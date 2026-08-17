# Blinker Arduino Client HTTP adapter

This optional adapter implements the `IHttpClient` contract on an
application-owned Arduino `Client` compatible object.

It does not bundle `HTTPClient`, `ArduinoHttpClient`, `ArduinoJson`, WiFi, or a
TLS implementation. The sketch chooses and configures the actual network
client:

~~~cpp
WiFiClientSecure tls;
blinker::ArduinoClientHttpAdapter<WiFiClientSecure> http(
    tls,
    blinker::HttpSecurity::Tls);
~~~

UNO R4 uses the same adapter with `WiFiSSLClient`. Plain HTTP uses
`WiFiClient` plus `HttpSecurity::PlainTcp`. The adapter rejects a request whose
declared security differs from the constructor, so a TLS request cannot
silently fall back to a plain socket.

Current response support is HTTP/1.0/1.1 with `Content-Length` or
connection-close framing. Chunked transfer encoding is deliberately rejected;
the login service should return a bounded `Content-Length` response.

The underlying Arduino `Client::connect()` may block according to that
client's configured timeout. All request writes and response reads after the
connect step are incremental and bounded by `poll()` budget.
