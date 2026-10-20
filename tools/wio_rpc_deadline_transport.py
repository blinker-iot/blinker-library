"""Exact-anchor transport projection for the internal Wio RPC boundary.

This module does not read or install an SDK. The owning candidate generator pins
the inputs, calls transform(), and records the final source and output hashes.
"""


def _replace(text: str, old: str, new: str, label: str) -> str:
    if text.count(old) != 1:
        raise ValueError(f"transport anchor must occur exactly once: {label}")
    return text.replace(old, new, 1)


def _function(text: str, signature: str, next_signature: str, body: str) -> str:
    if text.count(signature) != 1 or text.count(next_signature) != 1:
        raise ValueError(f"transport function anchors must occur exactly once: {signature}")
    start, end = text.index(signature), text.index(next_signature)
    if end <= start:
        raise ValueError(f"transport function anchor order: {signature}")
    return text[:start] + body.strip() + "\n\n" + text[end:]


UART_NAMESPACE = "namespace wio_boundary = blinker::wio_rpc;\n"

IRQ = r"""
void EUart::IrqHandler()
{
  wio_boundary::Guard guard;
  if (!wio_boundary::boundary.ready()) return;
  if (sercom->isFrameErrorUART() || sercom->isUARTError())
  {
    wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
    return;
  }
  if (sercom->availableDataUART())
  {
    if (rxBuffer.isFull())
    {
      wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
      return;
    }
    rxBuffer.store_char(sercom->readDataUART());
  }
  if (sercom->isDataRegisterEmptyUART())
  {
    if (txBuffer.available())
    {
      // DRE is known true and IRQs are held; never enter SERCOM's busy wait.
      SERCOM0->USART.DATA.reg = static_cast<uint16_t>(txBuffer.read_char());
    }
    else
    {
      sercom->disableDataRegisterEmptyInterruptUART();
    }
  }
}
"""

QUARANTINE = r"""
void EUart::quarantine()
{
  // Called only by the shared boundary under its short, saved-PRIMASK Guard.
  // Do not re-enter the boundary, reset/flush UART, or wait for SYNCBUSY/TXC.
  SERCOM0->USART.INTENCLR.reg = SERCOM_USART_INTENCLR_MASK;
  NVIC_DisableIRQ(SERCOM0_0_IRQn);
  NVIC_DisableIRQ(SERCOM0_1_IRQn);
  NVIC_DisableIRQ(SERCOM0_2_IRQn);
  NVIC_DisableIRQ(SERCOM0_3_IRQn);

  // Fixed, exclusive Wio RPC UART: pin 85 PB24 TX, pin 84 PC24 RX.
  // Disconnect pin mux even if a peripheral synchronization bit is stuck.
  PORT->Group[PORTB].OUTSET.reg = (1ul << 24);
  PORT->Group[PORTB].DIRSET.reg = (1ul << 24);
  PORT->Group[PORTB].PINCFG[24].reg &= ~PORT_PINCFG_PMUXEN;
  PORT->Group[PORTC].PINCFG[24].reg &= ~PORT_PINCFG_PMUXEN;
  rxBuffer.clear();
  txBuffer.clear();
  NVIC_ClearPendingIRQ(SERCOM0_0_IRQn);
  NVIC_ClearPendingIRQ(SERCOM0_1_IRQn);
  NVIC_ClearPendingIRQ(SERCOM0_2_IRQn);
  NVIC_ClearPendingIRQ(SERCOM0_3_IRQn);
}
"""

WRITE = r"""
size_t EUart::write(const uint8_t data)
{
  while (wio_boundary::boundary.checkFrame(wio_boundary::Direction::Tx))
  {
    {
      wio_boundary::Guard guard;
      if (!wio_boundary::boundary.ready()) return 0;
      if (sercom->isDataRegisterEmptyUART() && txBuffer.available() == 0)
      {
        wio_boundary::boundary.markSend();
        if (!wio_boundary::boundary.ready()) return 0;
        SERCOM0->USART.DATA.reg = static_cast<uint16_t>(data);
        return 1;
      }
      if (!txBuffer.isFull())
      {
        wio_boundary::boundary.markSend();
        if (!wio_boundary::boundary.ready()) return 0;
        txBuffer.store_char(data);
        sercom->enableDataRegisterEmptyInterruptUART();
        return 1;
      }
    }
    blinker_wio_rpc_pause();
  }
  return 0;
}
"""

READ = r"""
int EUart::read()
{
  wio_boundary::Guard guard;
  if (!wio_boundary::boundary.ready()) return -1;
  const int c = rxBuffer.read_char();
  if (c >= 0) wio_boundary::boundary.progressRx();
  return wio_boundary::boundary.ready() ? c : -1;
}
"""

WAIT_READ = r"""
void EUart::waitForRead()
{
  while (wio_boundary::boundary.checkFrame(wio_boundary::Direction::Rx))
  {
    if (available() > 0) return;
    blinker_wio_rpc_pause();
  }
}
"""

WAIT_WRITE = r"""
void EUart::waitForWrite(size_t n)
{
  while (wio_boundary::boundary.checkFrame(wio_boundary::Direction::Tx))
  {
    if (n <= static_cast<size_t>(availableForWrite())) return;
    blinker_wio_rpc_pause();
  }
}
"""

RECEIVE = r"""
erpc_status_t UartTransport::underlyingReceive(uint8_t *data, uint32_t size)
{
  uint32_t bytesRead = 0;
  while (bytesRead < size)
  {
    if (!wio_boundary::boundary.checkFrame(wio_boundary::Direction::Rx))
      return kErpcStatus_ReceiveFailed;
    waitMessage();
    if (!wio_boundary::boundary.checkFrame(wio_boundary::Direction::Rx))
      return kErpcStatus_ReceiveFailed;
    const int c = m_uartDrv->read();
    if (c < 0)
    {
      if (!wio_boundary::boundary.ready()) return kErpcStatus_ReceiveFailed;
      blinker_wio_rpc_pause();
      continue;
    }
    data[bytesRead++] = static_cast<uint8_t>(c);
  }
  return wio_boundary::boundary.checkFrame(wio_boundary::Direction::Rx)
      ? kErpcStatus_Success : kErpcStatus_ReceiveFailed;
}
"""

SEND = r"""
erpc_status_t UartTransport::underlyingSend(const uint8_t *data, uint32_t size)
{
  uint32_t sentSize = 0;
  while (sentSize < size)
  {
    if (!wio_boundary::boundary.checkFrame(wio_boundary::Direction::Tx))
      return kErpcStatus_SendFailed;
    const uint32_t sendSize = min(size - sentSize, static_cast<uint32_t>(255));
    m_uartDrv->waitForWrite(sendSize);
    if (!wio_boundary::boundary.checkFrame(wio_boundary::Direction::Tx))
      return kErpcStatus_SendFailed;
    const size_t written = m_uartDrv->write(&data[sentSize], sendSize);
    if (written != sendSize)
    {
      wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
      return kErpcStatus_SendFailed;
    }
    sentSize += static_cast<uint32_t>(written);
  }
  return wio_boundary::boundary.checkFrame(wio_boundary::Direction::Tx)
      ? kErpcStatus_Success : kErpcStatus_SendFailed;
}
"""

FRAME_RECEIVE = r"""
erpc_status_t FramedTransport::receive(MessageBuffer *message)
{
    assert(m_crcImpl && "Uninitialized Crc16 object.");
#if ERPC_THREADS
    wio_boundary::BoundedLock<Mutex> lock(m_receiveLock);
    if (!lock) return kErpcStatus_ReceiveFailed;
#endif
    wio_boundary::FrameScope frame(wio_boundary::Direction::Rx);
    if (!frame) return kErpcStatus_ReceiveFailed;
    if (!wio_boundary::boundary.checkFrame(wio_boundary::Direction::Rx))
        return kErpcStatus_ReceiveFailed;
    Header h;
    erpc_status_t ret = underlyingReceive((uint8_t *)&h, sizeof(h));
    if (ret != kErpcStatus_Success)
    {
        wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
        return ret;
    }
    if (h.m_messageSize > message->getLength())
    {
        wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
        return kErpcStatus_ReceiveFailed;
    }
    // Header and body share one FrameScope: partial RX never gets a new budget.
    ret = underlyingReceive(message->get(), h.m_messageSize);
    if (ret != kErpcStatus_Success)
    {
        wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
        return ret;
    }
    const uint16_t computedCrc = m_crcImpl->computeCRC16(message->get(), h.m_messageSize);
    if (computedCrc != h.m_crc)
    {
        wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
        return kErpcStatus_CrcCheckFailed;
    }
    if (!wio_boundary::boundary.checkFrame(wio_boundary::Direction::Rx))
        return kErpcStatus_ReceiveFailed;
    message->setUsed(h.m_messageSize);
    return kErpcStatus_Success;
}
"""

FRAME_SEND = r"""
erpc_status_t FramedTransport::send(MessageBuffer *message)
{
    assert(m_crcImpl && "Uninitialized Crc16 object.");
#if ERPC_THREADS
    wio_boundary::BoundedLock<Mutex> lock(m_sendLock);
    if (!lock) return kErpcStatus_SendFailed;
#endif
    wio_boundary::FrameScope frame(wio_boundary::Direction::Tx);
    if (!frame) return kErpcStatus_SendFailed;
    if (!wio_boundary::boundary.checkFrame(wio_boundary::Direction::Tx))
        return kErpcStatus_SendFailed;
    const uint16_t messageLength = message->getUsed();
    Header h;
    h.m_messageSize = messageLength;
    h.m_crc = m_crcImpl->computeCRC16(message->get(), messageLength);
    erpc_status_t ret = underlyingSend((uint8_t *)&h, sizeof(h));
    if (ret == kErpcStatus_Success)
        ret = underlyingSend(message->get(), messageLength);
    if (ret != kErpcStatus_Success)
    {
        wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
        return ret;
    }
    return wio_boundary::boundary.checkFrame(wio_boundary::Direction::Tx)
        ? kErpcStatus_Success : kErpcStatus_SendFailed;
}
"""


def transform(inputs: dict[str, str], port_text: str) -> dict[str, str]:
    """Project only this module's files; input keys are relative to RPC src/."""
    required = (
        "erpc/erpc_arduino_uart_transport.h", "erpc/erpc_arduino_uart_transport.cpp",
        "erpc/erpc_framed_transport.cpp", "erpc_Unified_init.cpp",
    )
    missing = set(required) - inputs.keys()
    if missing or not port_text.strip():
        raise ValueError("missing pinned transport input or native port template")
    out = {}
    path = required[0]
    text = _replace(inputs[path], '#include "erpc_framed_transport.h"',
                    '#include "erpc_framed_transport.h"\n#include "blinker_wio_rpc_boundary.h"', path)
    text = _replace(text, "    void IrqHandler();", "    void IrqHandler();\n    void quarantine();", path)
    text = _replace(text, "    volatile bool is_waiting_for_read;\n    Semaphore sem_read;\n    volatile size_t request_size;\n    Semaphore sem_write;\n", "", path)
    out[path] = text

    path = required[1]
    text = _replace(inputs[path], "using namespace erpc;", "using namespace erpc;\n" + UART_NAMESPACE, path)
    text = _replace(text, "  is_waiting_for_read{ false }, sem_read{ 0 }, request_size{ 0 }, sem_write{ 0 }", "  sercom{ _s }", path)
    text = _replace(text, "  sercom = _s;\n", "", path)
    text = _replace(text, "void EUart::begin(unsigned long baudrate, uint16_t config)\n{",
        "void EUart::begin(unsigned long baudrate, uint16_t config)\n{\n"
        "  // Cold SDK clock/reset synchronization is outside the single-RPC budget.\n"
        "  // A faulted runtime must never re-enable or re-mux this UART.\n"
        "  if (!wio_boundary::boundary.ready()) return;", path)
    text = _function(text, "void EUart::end()", "void EUart::flush()", """
void EUart::end()
{
  wio_boundary::boundary.trip(wio_boundary::Fault::Transport);
}
""")
    text = _function(text, "void EUart::flush()", "void EUart::IrqHandler()", """
void EUart::flush()
{
  // The internal RPC graph never flushes. Refuse rather than busy-wait TXC.
  wio_boundary::boundary.trip(wio_boundary::Fault::Unsupported);
}
""")
    text = _function(text, "void EUart::IrqHandler()", "int EUart::available()", IRQ + "\n" + QUARANTINE)
    for signature, next_sig, name, expr in (
        ("int EUart::available()", "int EUart::availableForWrite()", "available", "rxBuffer.available()"),
        ("int EUart::availableForWrite()", "int EUart::peek()", "availableForWrite", "txBuffer.availableForStore()"),
        ("int EUart::peek()", "int EUart::read()", "peek", "rxBuffer.peek()"),
    ):
        fallback = "-1" if name == "peek" else "0"
        text = _function(text, signature, next_sig, f"""
int EUart::{name}()
{{
  wio_boundary::Guard guard;
  return wio_boundary::boundary.ready() ? {expr} : {fallback};
}}
""")
    text = _function(text, "int EUart::read()", "size_t EUart::write(const uint8_t data)", READ)
    text = _function(text, "size_t EUart::write(const uint8_t data)", "SercomNumberStopBit EUart::extractNbStopBit", WRITE)
    text = _function(text, "void EUart::waitForRead()", "void EUart::waitForWrite(size_t n)", WAIT_READ)
    start = "void EUart::waitForWrite(size_t n)"
    next_start = "////////////////////////////////////////////////////////////////////////////////\n// Variables"
    text = _function(text, start, next_start, WAIT_WRITE)
    text = _function(text, "erpc_status_t UartTransport::underlyingReceive", "erpc_status_t UartTransport::underlyingSend", RECEIVE)
    text = _function(text, "erpc_status_t UartTransport::underlyingSend", "bool UartTransport::hasMessage()", SEND)
    text = _replace(text, "  m_uartDrv->begin(m_baudrate);\n  return kErpcStatus_Success;",
        "  if (!wio_boundary::boundary.ready()) return kErpcStatus_InitFailed;\n"
        "  m_uartDrv->begin(m_baudrate);\n"
        "  return wio_boundary::boundary.ready() ? kErpcStatus_Success : kErpcStatus_InitFailed;", path)
    out[path] = text

    path = required[2]
    text = _replace(inputs[path], '#include "erpc_message_buffer.h"',
                    '#include "erpc_message_buffer.h"\n#include "blinker_wio_rpc_boundary.h"', path)
    text = _replace(text, "using namespace erpc;", "using namespace erpc;\n" + UART_NAMESPACE, path)
    text = _function(text, "erpc_status_t FramedTransport::receive", "erpc_status_t FramedTransport::send", FRAME_RECEIVE)
    old_send = text[text.index("erpc_status_t FramedTransport::send"):]
    text = _replace(text, old_send, FRAME_SEND.strip() + "\n", path)
    out[path] = text

    path = required[3]
    text = _replace(inputs[path], 'Thread clientThread(&runClient, configMAX_PRIORITIES - 2, 20480, "runClient");',
        'Thread clientThread(&runClient, configMAX_PRIORITIES - 2, 20480, "runClient");\n\n'
        '#include "erpc/blinker_wio_rpc_port.inc"', path)
    text = _replace(text, "        if (g_server.poll() != kErpcStatus_Success)",
        "        if (!blinker::wio_rpc::boundary.ready()) { delay(20); continue; }\n"
        "        if (g_server.poll() != kErpcStatus_Success)", path)
    out[path] = text
    out["erpc/blinker_wio_rpc_port.inc"] = port_text
    return out
