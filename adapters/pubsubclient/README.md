# Blinker PubSubClient Adapter

This optional Arduino library adapts an application-owned PubSubClient object
to BlinkerDeviceV2 IMqttClient.

Important constraints:

- publish QoS is 0 because PubSubClient 2.8 only publishes QoS 0;
- subscribe QoS 0 and 1 are accepted;
- packetBufferSize must contain MQTT header, topic, and one complete BBP/2 frame;
- MqttFrameTransport validates both publish and subscribe packet capacity during
  configure(), so an undersized buffer fails explicitly instead of silently
  dropping an inbound packet;
- the constructor's optional fourth argument is the PubSubClient socket timeout
  in seconds and defaults to 2 instead of PubSubClient's 15-second default;
- the application must pass the actual security type of its underlying
  Arduino Client;
- TLS never falls back to plain TCP;
- the adapter does not own or delete PubSubClient or its Arduino Client;
- PubSubClient allocates its packet buffer internally;
- PubSubClient 2.8 does not expose inbound QoS/retain metadata to its callback,
  so the adapter reports those fields as unknown;
- one adapter exclusively owns one PubSubClient object's callback;
- the broker/server must never retain device command messages; retained status
  cannot be inspected through the PubSubClient 2.8 callback;
- on platforms whose PubSubClient callback has no context pointer, each live
  adapter must use a distinct CallbackSlot template value.

`poll(budgetMicros)` cannot preempt a synchronous Arduino `Client` operation.
The budget is therefore cooperative; the short socket timeout bounds PubSubClient
read/CONNACK waits, while DNS, TCP, and TLS connect timeouts still depend on the
underlying Arduino Client implementation.

Example:

~~~cpp
WiFiClientSecure network;
PubSubClient mqtt(network);

blinker::PubSubClientAdapter<0> mqttAdapter(
    mqtt,
    blinker::MqttSecurity::Tls,
    1200,
    2);
~~~
