"""Exact-anchor internal STA/Plain client hygiene; never writes the SDK.

The master operator pins original inputs and combines this module with the
pending-lifecycle and UART transforms. This is not an ordinary SDK replacement
or support for the generated BLE/AP/scan/raw-TCP API surface.
"""

import re


INPUTS = (
    "erpc/erpc_client_manager.h",
    "erpc/erpc_client_manager.cpp",
    "erpc/rpc_wifi_api_client.cpp",
    "erpc/rpc_system_client.cpp",
    "lwip/sockets.c",
    "lwip/netdb.c",
    "rtl_wifi/rpc_wifi_api_hal.cpp",
)

_INCLUDE = '#include "erpc/blinker_wio_rpc_boundary.h"\n'


def _replace(text, old, new, count=1):
    if text.count(old) != count:
        raise ValueError("client hygiene anchor/count mismatch")
    return text.replace(old, new)


def _function(text, name, edit):
    """Restrict an edit to one pinned function, including nested scopes."""
    matches = list(re.finditer(r"^[\w *]+\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{", text, re.M))
    if len(matches) != 1:
        raise ValueError("client hygiene function anchor mismatch: " + name)
    start = matches[0].start()
    opening = text.find("{", matches[0].start(), matches[0].end())
    depth = 0
    tokens = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]'
    for token in re.finditer(tokens, text[opening:]):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                end = opening + token.end()
                return text[:start] + edit(text[start:end]) + text[end:]
    raise ValueError("unterminated client hygiene function: " + name)


def _binary_locals(text, count):
    pattern = r"\bbinary_t ([A-Za-z_][A-Za-z_0-9]*);"
    if len(re.findall(pattern, text)) != count:
        raise ValueError("binary local declaration count mismatch")
    return re.sub(pattern, r"binary_t \1 = {0};", text)


_COPY_HELPER = """
// Only validated, owned decoded buffers may reach an ordinary SDK caller.
static int blinker_rpc_copy_output(void *destination, size_t capacity,
                                   const binary_t *source, size_t amount)
{
    if (blinker_wio_rpc_faulted() || !source || amount > capacity ||
        source->dataLength < amount || source->dataLength > capacity ||
        (amount && (!destination || !source->data)))
    {
        blinker_wio_rpc_invalid_output();
        return 0;
    }
    if (amount) memcpy(destination, source->data, amount);
    return 1;
}
"""


def _bridge_include(text):
    return _replace(text, '#include "erpc/erpc_port.h"\n',
                    '#include "erpc/erpc_port.h"\n' + _INCLUDE)


def _client(text):
    text = _replace(text, '#include "erpc_client_manager.h"\n',
                    '#include "erpc_client_manager.h"\n' + _INCLUDE)
    text = _replace(text, """    // Create codec to read and write the request.
    Codec *codec = createBufferAndCodec();

    return RequestContext(++m_sequence, codec, isOneway);""", """    // Sample caller/IRQ permission before the short PRIMASK guard.
    const bool allowed = blinker_wio_rpc_caller_allowed();
    if (!allowed)
    {
        blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Unsupported);
        return RequestContext(0U, NULL, isOneway);
    }
    uint32_t sequence = 0U;
    {
        blinker::wio_rpc::Guard guard;
        sequence = m_sequence + 1U;
        if (!blinker::wio_rpc::boundary.begin(sequence, allowed))
            return RequestContext(0U, NULL, isOneway);
        m_sequence = sequence;
    }
    // A denied or quarantined call never allocates a message or codec.
    Codec *codec = createBufferAndCodec();
    if (!codec) blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Codec);
    return RequestContext(sequence, codec, isOneway);""")
    text = _replace(text, """void ClientManager::performRequest(RequestContext &request)
{
    // Check the codec status""", """void ClientManager::performRequest(RequestContext &request)
{
    if (!request.getCodec())
    {
        blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Codec);
        return;
    }
    if (!blinker::wio_rpc::boundary.activeFor(request.getSequence()))
    {
        request.getCodec()->updateStatus(kErpcStatus_Fail);
        return;
    }
    // Check the codec status""")
    text = _replace(text, """        // Do not perform the request
        return;""", """        // Do not perform the request or retain an unusable transport.
        blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Codec);
        return;""")
    text = _replace(text, """    assert(m_serverThreadId && "server thread id was not set");
    if (Thread::getCurrentThreadId() == m_serverThreadId)
    {
        return performNestedClientRequest(request);
    }""", """    if (Thread::getCurrentThreadId() == m_serverThreadId)
    {
        blinker::wio_rpc::boundary.trip(blinker::wio_rpc::Fault::Unsupported);
        request.getCodec()->updateStatus(kErpcStatus_Fail);
        return;
    }""")
    text = _replace(text, """    m_messageFactory->dispose(request.getCodec()->getBuffer());
    m_codecFactory->dispose(request.getCodec());""", """    // Cleanup is local, including a failed allocation or a denied call.
    if (request.getCodec())
    {
        m_messageFactory->dispose(request.getCodec()->getBuffer());
        m_codecFactory->dispose(request.getCodec());
    }
    blinker::wio_rpc::boundary.release(request.getSequence());""")
    return text


def _generated(text, count, binary):
    text = _replace(text, '#include "erpc_client_manager.h"\n',
                    '#include "erpc_client_manager.h"\n' + _INCLUDE)
    for anchor in ("RequestContext request = g_client->createRequest(false);",
                   "g_client->performRequest(request);", "g_client->releaseRequest(request);"):
        if text.count(anchor) != count:
            raise ValueError("generated client lifecycle count mismatch")
    text = _replace(text, "err = codec->getStatus();",
                    "err = (erpc_status_t)blinker_wio_rpc_finalize(request.getSequence(), codec->getStatus());", count)
    if binary:
        text = _replace(text, """    uint8_t * data_local;
    codec->readBinary(&data->dataLength, &data_local);
    data->data = (uint8_t *) erpc_malloc(data->dataLength * sizeof(uint8_t));
    if (data->data == NULL)
    {
        codec->updateStatus(kErpcStatus_MemoryError);
    }
    else
    {
        memcpy(data->data, data_local, data->dataLength);
    }""", """    data->data = NULL;
    data->dataLength = 0U;
    if (codec->getStatus() != kErpcStatus_Success) return;
    uint8_t * data_local = NULL;
    uint32_t length = 0U;
    codec->readBinary(&length, &data_local);
    if (codec->getStatus() != kErpcStatus_Success || !length) return;
    if (!data_local)
    {
        codec->updateStatus(kErpcStatus_Fail);
        return;
    }
    data->data = (uint8_t *)erpc_malloc(length);
    if (!data->data) codec->updateStatus(kErpcStatus_MemoryError);
    else
    {
        memcpy(data->data, data_local, length);
        data->dataLength = length;
    }""")
    text = _replace(text, """        uint32_t return_len;
        char * return_local;
        codec->readString(&return_len, &return_local);
        result = (char *) erpc_malloc((return_len + 1) * sizeof(char));
        if (result == NULL)
        {
            codec->updateStatus(kErpcStatus_MemoryError);
        }
        else
        {
            memcpy(result, return_local, return_len);
            (result)[return_len] = 0;
        }""", """        uint32_t return_len = 0U;
        char * return_local = NULL;
        codec->readString(&return_len, &return_local);
        if (codec->getStatus() == kErpcStatus_Success)
        {
            if ((return_len && !return_local) || return_len == UINT32_MAX)
                codec->updateStatus(kErpcStatus_Fail);
            else
            {
                result = (char *)erpc_malloc(return_len + 1U);
                if (!result) codec->updateStatus(kErpcStatus_MemoryError);
                else
                {
                    if (return_len) memcpy(result, return_local, return_len);
                    result[return_len] = 0;
                }
            }
        }""")
    name = "rpc_ip4addr_ntoa" if binary else "rpc_system_version"
    return _function(text, name, lambda body: _replace(body, "    return result;", """    if (err != kErpcStatus_Success && result)
    {
        erpc_free(result);
        result = NULL;
    }
    return result;"""))


def _sockets(text):
    text = _bridge_include(text)
    text = _replace(text, "int lwip_errno()", _COPY_HELPER + "\nint lwip_errno()")
    text = _binary_locals(text, 20)
    text = _replace(text, "binary_t b_in_optval, b_out_optval;", "binary_t b_in_optval = {0}, b_out_optval = {0};")
    text = _replace(text, "    int so_error = rpc_lwip_errno();", "    if (blinker_wio_rpc_faulted()) return ENOTCONN;\n    int so_error = rpc_lwip_errno();\n    if (blinker_wio_rpc_faulted()) return ENOTCONN;")
    for name in ("lwip_getpeername", "lwip_getsockname"):
        def address(body):
            body = _replace(body, "    FUNC_ENTRY;", """    FUNC_ENTRY;
    if (!name || !namelen) { blinker_wio_rpc_invalid_output(); return -1; }
    const size_t capacity = *namelen;
    memset(name, 0, capacity);""")
            return _replace(body, "    memcpy(name, b_name.data, sizeof(struct sockaddr));", """    if (ret >= 0 && (!blinker_rpc_copy_output(name, capacity, &b_name, sizeof(struct sockaddr)) || *namelen > capacity))
    { blinker_wio_rpc_invalid_output(); ret = -1; }
    if (ret < 0) { memset(name, 0, capacity); *namelen = 0U; }
    if (b_name.data) erpc_free(b_name.data);""")
        text = _function(text, name, address)
    def sockopt(body):
        body = _replace(body, "    FUNC_ENTRY;", """    FUNC_ENTRY;
    if (!optval || !optlen) { blinker_wio_rpc_invalid_output(); return -1; }
    const size_t capacity = *optlen;""")
        return _replace(body, "    memcpy(optval, b_out_optval.data, b_out_optval.dataLength);", """    if (ret >= 0 && (!blinker_rpc_copy_output(optval, capacity, &b_out_optval, *optlen) || *optlen > capacity))
    { blinker_wio_rpc_invalid_output(); ret = -1; }
    if (ret < 0) { memset(optval, 0, capacity); *optlen = 0U; }""")
    text = _function(text, "lwip_getsockopt", sockopt)
    for name in ("lwip_recv", "lwip_read"):
        text = _function(text, name, lambda body: _replace(body,
            "        memcpy(mem, b_mem.data, b_mem.dataLength);",
            "        if (!blinker_rpc_copy_output(mem, len, &b_mem, (size_t)ret)) ret = -1;"))
    text = _function(text, "lwip_ioctl", lambda body: _replace(body,
        "    memcpy(argp, b_out_argp.data, b_in_argp.dataLength);", """    if (ret >= 0 && !blinker_rpc_copy_output(argp, b_in_argp.dataLength, &b_out_argp, b_in_argp.dataLength)) ret = -1;
    if (ret < 0 && argp) memset(argp, 0, b_in_argp.dataLength);"""))
    # Descriptor allocation is outside the generated call, but its failure must
    # not be dereferenced or leak earlier descriptors. Keep the native layout.
    previous = []
    for name in ("b_readset", "b_writeset", "b_exceptset", "b_timeout"):
        cleanup = " ".join("erpc_free(" + prior + ");" for prior in previous)
        old = name + " = (binary_t *)erpc_malloc(sizeof(binary_t));"
        text = _replace(text, old, old + "\n        if (!" + name + ") { " + cleanup + " blinker_wio_rpc_invalid_output(); return -1; }")
        previous.append(name)
    return text


def _dns(text):
    text = _bridge_include(text)
    text = _binary_locals(text, 1)
    text = _replace(text, "err_t netconn_gethostbyname", _COPY_HELPER + "\nerr_t netconn_gethostbyname")
    text = _replace(text, "    binary_t ip_addr = {0};", """    if (!addr) { blinker_wio_rpc_invalid_output(); return ERR_ARG; }
    memset(addr, 0, sizeof(*addr));
    binary_t ip_addr = {0};""")
    return _replace(text, "    memcpy(addr, ip_addr.data, sizeof(ip_addr_t));", "    if (ret == ERR_OK && !blinker_rpc_copy_output(addr, sizeof(*addr), &ip_addr, sizeof(*addr))) ret = ERR_VAL;")


def _hal(text):
    text = _bridge_include(text)
    text = _binary_locals(text, 62)
    text = _replace(text, "int wifi_mode = RTW_MODE_NONE;", _COPY_HELPER + "\nint wifi_mode = RTW_MODE_NONE;")
    copies = (
        ("wifi_get_associated_client_list", "client_list_buffer", "buffer_length", "b_client_list_buffer", "buffer_length"),
        ("wifi_get_ap_info", "ap_info", "sizeof(*ap_info)", "b_ap_info", "sizeof(*ap_info)"),
        ("wifi_get_setting", "pSetting", "sizeof(*pSetting)", "b_pSetting", "sizeof(*pSetting)"),
        ("tcpip_adapter_get_ip_info", "ip_info", "sizeof(*ip_info)", "_ip_info", "sizeof(*ip_info)"),
        ("tcpip_adapter_get_dns_info", "dns", "sizeof(*dns)", "_dns", "sizeof(*dns)"),
        ("tcpip_adapter_get_mac", "mac", "6U", "_mac", "6U"),
    )
    for name, destination, capacity, source, amount in copies:
        def copy(body, destination=destination, capacity=capacity, source=source, amount=amount):
            matches = list(re.finditer(r"        memcpy\(" + re.escape(destination) + r", " + re.escape(source) + r"\.data, [^;]+\);", body))
            if len(matches) != 1:
                raise ValueError("HAL output copy count mismatch")
            old = matches[0].group()
            body = _replace(body, old, "        if (!blinker_rpc_copy_output(" + destination + ", " + capacity + ", &" + source + ", " + amount + ")) ret = -1;")
            return _replace(body, "    FUNC_ENTRY;", "    FUNC_ENTRY;\n    if (" + destination + ") memset(" + destination + ", 0, " + capacity + ");")
        text = _function(text, name, copy)
    text = _function(text, "wifi_get_reconnect_data", lambda body: _replace(body,
        "    memcpy(wifi_info, data.data, sizeof(wlan_fast_reconnect_profile_t));",
        "    if (ret == RTW_SUCCESS && !blinker_rpc_copy_output(wifi_info, sizeof(*wifi_info), &data, sizeof(*wifi_info))) ret = RTW_ERROR;"))
    return text


def transform(inputs: dict[str, str]) -> dict[str, str]:
    """Return six modified sources; the pinned common header is read-only."""
    if any(key not in inputs or not isinstance(inputs[key], str) for key in INPUTS):
        raise ValueError("all seven pinned client inputs required")
    original = {key: inputs[key].replace("\r\n", "\n") for key in INPUTS}
    header = original["erpc/erpc_client_manager.h"]
    for declaration in ("virtual RequestContext createRequest(bool isOneway);",
                        "virtual void releaseRequest(RequestContext &request);",
                        "Codec *getCodec(void) { return m_codec; }"):
        if header.count(declaration) != 1:
            raise ValueError("pinned ClientManager header contract mismatch")
    return {
        "erpc/erpc_client_manager.cpp": _client(original["erpc/erpc_client_manager.cpp"]),
        "erpc/rpc_wifi_api_client.cpp": _generated(original["erpc/rpc_wifi_api_client.cpp"], 169, True),
        "erpc/rpc_system_client.cpp": _generated(original["erpc/rpc_system_client.cpp"], 2, False),
        "lwip/sockets.c": _sockets(original["lwip/sockets.c"]),
        "lwip/netdb.c": _dns(original["lwip/netdb.c"]),
        "rtl_wifi/rpc_wifi_api_hal.cpp": _hal(original["rtl_wifi/rpc_wifi_api_hal.cpp"]),
    }
