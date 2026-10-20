# R4 production cleanup guard differences

This isolated Host test requires the unchanged Arduino Renesas UNO core **1.5.3**
and **PubSubClient 2.8**. Supply their source paths explicitly:

```text
cmake -S tests/internal/r4_cleanup_guard -B <private-host-build> -DR4_CORE=<renesas_uno/1.5.3> -DPUBSUB_SOURCE=<PubSubClient/src>
cmake --build <private-host-build> --config Release --parallel 1
ctest --test-dir <private-host-build> -C Release --output-on-failure
```

The complete original `WiFiClient.cpp`, `WiFi.cpp` and `PubSubClient.cpp` are
compiled, together with the production modem Client, Station, HTTP and MQTT
adapters. The original Modem declaration, WiFi commands/types and FifoBuffer
remain in use. The Modem **implementation** is a deliberately permissive command
recording seam: it does not read the shared owner or hide a missing guard.

Eight independent cases compare valid sockets/nonempty RX and active Station
states with shared-fault cleanup. Arduino API and synchronization are
single-threaded Host seams, not UART, IRQ, physical isolation, timing, allocation
failure, native ARM ABI or real-device evidence. SDK source is untouched; this
suite compiles the production wrapper under test. Raw SDK WiFi/Modem APIs outside
the production wrappers are not admitted.

Four separate `r4_connect_` cases cover the bounded IPv4 host conversion,
unchanged hostname and IPv6 routes, and rejection before IP inspection when the
shared owner is busy or faulted. To run only this difference (not repeat the eight
cleanup cases), use `ctest --test-dir <private-host-build> -C Release -R ^r4_connect_ --output-on-failure`.
The IPv4 formatter is exercised through `Client&` against complete original SDK
bodies with zero, maximum and ordinary octets/ports. The IPAddress compatibility
seam counts type/index/toString calls and can return an empty IPv4 String to make
accidental use observable; this is not native String allocation-failure evidence.
Its IPv6 text is a fixed seam value, so that case proves fallback dispatch only,
not native IPv6 formatting or network support. No whole-client allocation-free,
TLS, cold-start, IRQ or actual out-of-memory support is established by these cases.
