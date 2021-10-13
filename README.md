# VRPC Arduino Agent

Asynchronous RPC library for Arduino-based embedded C++ applications.

This library allows to adapt existing embedded functions by simply naming them
using a special macro. Once adapted, those functions may be called from any
remote location connected to the internet and by using any programming language
(e.g. Node.js).

See [here](https://vrpc.io) for more details on the VRPC eco-system.
## Required dependencies

- [ArduinoSTL](https://github.com/mike-matera/ArduinoSTL) by Mike Matera
- [ArduinoJSON](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties) by Benoit Blanchon
- [MQTT](https://github.com/256dpi/arduino-mqtt) by Joël Gähwiler

## Example

```c++
#include <SPI.h>
#include <WiFiNINA.h>
#include <vrpc.h>

const char ssid[] = "<WLAN-SSID>";
const char pass[] = "<WLAN-PASSWORD>";

WiFiClient wifi;
VrpcAgent agent;

void ledOn() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOff() {
  digitalWrite(LED_BUILTIN, LOW);
}

void connect() {
  Serial.print("Connecting WiFi...");
  while (WiFi.status() != WL_CONNECTED) delay(1000);
  Serial.println("[OK]");
  agent.connect();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(ssid, pass);
  agent.begin(wifi);

  connect();

}

void loop() {
  agent.loop();
  if (!agent.connected()) connect();
}

VRPC_GLOBAL_FUNCTION(void, ledOn);
VRPC_GLOBAL_FUNCTION(void, ledOff);
```

> **NOTE**
> Depending on the board you are using it may be necessary to include parts of
> the STL as external library. In this case add:
> `#define WITH_STL 1`
> before including the VRPC

## API

### Function adaptation macros

Use the macros described below to add remote access to regular functions.

#### 1. Global Functions

```c++
VRPC_GLOBAL_FUNCTION(<returnType>, <functionName>[, <argTypes>])
```

Example:

```c++

int foo () {
  // [...]
}

void bar (String& s, bool b) {
  // [...]
}

VRPC_GLOBAL_FUNCTION(int, foo)
VRPC_GLOBAL_FUNCTION(void, bar, String&, bool)
```

### class `VrpcAgent`

Agent that transforms existing code to be operated from a remote location.

### Summary

 Members                        | Descriptions
--------------------------------|---------------------------------------------
`public inline  `[`VrpcAgent`](#classVrpcAgent_1ace51d7fc67e6cca3db088b229292ded7)`(int maxBytesPerMessage)` | Constructs an agent.
`public template<>`  <br/>`inline void `[`begin`](#classVrpcAgent_1a5bcc3d82db137a8d4dd37f55ce83d53e)`(T & wifiClient,const String & domain,const String & token)` | Initializes the object using a client class for network transport.
`public inline bool `[`connected`](#classVrpcAgent_1aef4609a41a89bf7602011cca1fff5057)`()` | Reports the current connectivity status.
`public inline void `[`connect`](#classVrpcAgent_1afa4e6b81fcb0a990d5747b986adeecdb)`()` | Connect the agent to the broker.
`public inline void `[`loop`](#classVrpcAgent_1a89c5b7c6a84bccc8470b4bb8ff29a4ff)`()` | This function will send and receive VRPC packets.

### Members

#### `public inline  `[`VrpcAgent`](#classVrpcAgent_1ace51d7fc67e6cca3db088b229292ded7)`(int maxBytesPerMessage)`

Constructs an agent.

##### Parameters
* `maxBytesPerMessage` [optional, default: `1024`] Specifies the maximum size a single MQTT message may have

- - -
#### `public template<>`  <br/>`inline void `[`begin`](#classVrpcAgent_1a5bcc3d82db137a8d4dd37f55ce83d53e)`(T & wifiClient,const String & domain,const String & token)`

Initializes the object using a client class for network transport.

##### Parameters
* `T` Type of the client class

##### Parameters
* `wifiClient` A client class following the interface as described [here](https://www.arduino.cc/en/Reference/ClientConstructor)

* `domain` [optional, default: `"public.vrpc"`] The domain under which the agent-provided code is reachable

* `token` [optional, default: `""`] Access token as generated using the [VRPC App](https://app.vrpc.io), or password if own broker is used

* `broker` [optional, default: `"vrpc.io"`] Address of the MQTT broker

* `username` [optional] MQTT username (not needed when using the vrpc.io broker)

- - -

#### `public inline bool `[`connected`](#classVrpcAgent_1aef4609a41a89bf7602011cca1fff5057)`()`

Reports the current connectivity status.

##### Returns
true when connected, false otherwise

- - -

#### `public inline void `[`connect`](#classVrpcAgent_1afa4e6b81fcb0a990d5747b986adeecdb)`()`

Connect the agent to the broker.

The function will try to connect forever. Inspect the serial monitor to see the connectivity progress.

- - -

#### `public inline void `[`loop`](#classVrpcAgent_1a89c5b7c6a84bccc8470b4bb8ff29a4ff)`()`

Send and receive VRPC packets.

NOTE: This function should be called in every `loop`
