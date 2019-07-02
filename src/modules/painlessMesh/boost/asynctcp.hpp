#ifndef _BOOST_ASYNCTCP_HPP_
#define _BOOST_ASYNCTCP_HPP_

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <iostream>

#ifndef TCP_MSS
#define TCP_MSS 1024
#endif

using boost::asio::ip::tcp;

#define ASYNC_WRITE_FLAG_COPY 0x01

class AsyncClient;

typedef std::function<void(void*, AsyncClient*)> AcConnectHandler;
typedef std::function<void(void*, AsyncClient*, size_t len, uint32_t time)>
    AcAckHandler;
typedef std::function<void(void*, AsyncClient*, int8_t error)> AcErrorHandler;
typedef std::function<void(void*, AsyncClient*, void* data, size_t len)>
    AcDataHandler;
// typedef std::function<void(void*, AsyncClient*, struct pbuf* pb)>
//    AcPacketHandler;
typedef std::function<void(void*, AsyncClient*, uint32_t time)>
    AcTimeoutHandler;

typedef boost::asio::ip::address IPAddress;

class AsyncClient {
 public:
  AsyncClient(boost::asio::io_service& io_service)
      : _io_service(io_service), mSocket(_io_service) {}

  bool connect(IPAddress ipaddress, uint16_t port) {
    namespace ip = boost::asio::ip;
    auto endpoint = ip::tcp::endpoint(ipaddress, port);

    mSocket.async_connect(endpoint, [&](auto& ec) { this->handleConnect(ec); });
    return true;
  }

  void initRead() {
    mSocket.async_read_some(
        boost::asio::buffer(mInputBuffer, TCP_MSS),
        [&](auto& ec, auto len) { this->handleData(ec, len); });
  }

  size_t write(const void* data, size_t len,
               size_t copy = ASYNC_WRITE_FLAG_COPY) {
    if (writing) return 0;
    writing = true;
    if (copy == ASYNC_WRITE_FLAG_COPY) {
      memcpy(mWriteBuffer, data, len);
      mSocket.async_send(
          boost::asio::buffer(mWriteBuffer, len),
          [&](auto& ec, auto len) { this->handleWrite(ec, len); });
    } else {
      mSocket.async_send(
          boost::asio::buffer(data, len),
          [&](auto& ec, auto len) { this->handleWrite(ec, len); });
    }
    return len;
  }

  // Dummy functions for compatibility with ESPAsycnTCP
  void send() {}
  void setNoDelay(bool value = true) {}
  void setRxTimeout(uint32_t timeout) {}
  const char* errorToString(int8_t error) { return ""; }

  // TODO: check if there is a better one
  void abort() { this->close(true); }

  void onConnect(AcConnectHandler cb, void* arg = 0) {
    _connect_cb = cb;
    _connect_cb_arg = arg;
  }

  void onDisconnect(AcConnectHandler cb, void* arg = 0) {
    _discard_cb = cb;
    _discard_cb_arg = arg;
  }

  void onAck(AcAckHandler cb, void* arg = 0) {
    _sent_cb = cb;
    _sent_cb_arg = arg;
  }

  void onError(AcErrorHandler cb, void* arg = 0) {
    _error_cb = cb;
    _error_cb_arg = arg;
  }

  void onData(AcDataHandler cb, void* arg = 0) {
    _recv_cb = cb;
    _recv_cb_arg = arg;
  }

  void onTimeout(AcTimeoutHandler cb, void* arg = 0) {
    _timeout_cb = cb;
    _timeout_cb_arg = arg;
  }

  void onPoll(AcConnectHandler cb, void* arg = 0) {
    _poll_cb = cb;
    _poll_cb_arg = arg;
  }

  bool connected() { return mSocket.is_open(); }

  bool freeable() { return !this->connected(); }

  void close(bool now = true) {
    if (this->connected()) {
      mSocket.close();
    }
    if (!disconnectCalled) {
      disconnectCalled = true;
      if (_discard_cb) {
        _discard_cb(_discard_cb_arg, this);
      }
    }
  }

  ~AsyncClient() { close(true); }

  size_t space() {
    // This could be more intelligent, but simple and safe for now
    if (writing) return 0;
    return TCP_MSS;
  }

  bool canSend() { return this->space() > 0; }

  size_t ack(size_t len) {
    // Currently assumes that len is actually the size of the data that was sent
    // Otherwise it will break
    initRead();
    return len;
  }

  tcp::socket& socket() { return mSocket; }

 protected:
  boost::asio::io_service& _io_service;
  tcp::socket mSocket;

  char mInputBuffer[TCP_MSS];
  char mWriteBuffer[TCP_MSS];
  bool writing = false;

  bool disconnectCalled = false;

  AcConnectHandler _connect_cb = 0;
  void* _connect_cb_arg = 0;
  AcConnectHandler _discard_cb = 0;
  void* _discard_cb_arg = 0;
  AcAckHandler _sent_cb = 0;
  void* _sent_cb_arg = 0;
  AcErrorHandler _error_cb = 0;
  void* _error_cb_arg = 0;
  AcDataHandler _recv_cb = 0;
  void* _recv_cb_arg = 0;
  AcTimeoutHandler _timeout_cb = 0;
  void* _timeout_cb_arg = 0;
  AcConnectHandler _poll_cb = 0;
  void* _poll_cb_arg = 0;

  void handleConnect(const boost::system::error_code& ec) {
    if (!ec) {
      if (_connect_cb) _connect_cb(_connect_cb_arg, this);

      initRead();
    } else {
      handleError(ec);
      if (this->connected()) close(true);
    }
  }

  void handleData(const boost::system::error_code& ec, size_t len) {
    if (disconnectCalled) return;

    if (!ec) {
      if (_recv_cb) {
        _recv_cb(_recv_cb_arg, this, (void*)mInputBuffer, len);
      }
    } else {
      handleError(ec);
      close(true);
    }
  }

  void handleWrite(const boost::system::error_code& ec, size_t len) {
    if (disconnectCalled) return;

    if (!ec) {
      if (_sent_cb) {
        // TODO send actual time
        _sent_cb(_sent_cb_arg, this, len, 0);
      }
      writing = false;
    } else {
      handleError(ec);
      close(true);
    }
  }

  void handleError(const boost::system::error_code& ec) {
    if (ec != boost::asio::error::eof && _error_cb) {
      _error_cb(_error_cb_arg, this, ec.value());
    }
  }
};

class AsyncServer {
 public:
  AsyncServer(boost::asio::io_service& io_service, uint16_t port)
      : _io_service(io_service),
        // mSocket(io_service),
        _port(port),
        mAcceptor(io_service) {}

  ~AsyncServer() { this->end(); }

  void onClient(AcConnectHandler cb, void* arg = 0) {
    _connect_cb = cb;
    _connect_cb_arg = arg;
  }

  // begin() should do start_accept (see server2):w
  void begin() {
    mAcceptor.open(tcp::v4());
    int one = 1;
    setsockopt(mAcceptor.native_handle(), SOL_SOCKET,
               SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), _port);
    mAcceptor.bind(endpoint);
    mAcceptor.listen();
    initAccept();
  }

  // Acceptor stop?
  void end() { mAcceptor.close(); }

  // Dummy function for compatibility with ESPAsycnTCP
  void setNoDelay(bool value = true) {}

 protected:
  boost::asio::io_service& _io_service;
  uint16_t _port;
  tcp::acceptor mAcceptor;
  AcConnectHandler _connect_cb = 0;
  void* _connect_cb_arg = 0;

  void initAccept() {
    AsyncClient* client = new AsyncClient(mAcceptor.get_io_service());
    mAcceptor.async_accept(
        client->socket(), [this, client](const boost::system::error_code& e) {
          if (!e && this->_connect_cb) {
            this->_connect_cb(this->_connect_cb_arg, client);
            this->initAccept();
            client->initRead();
          } else
            std::cout << "Error: " << e.message() << std::endl;
        });
  }
};

#endif
