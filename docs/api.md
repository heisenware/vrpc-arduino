# API Reference

### Required dependencies

- [ArduinoJSON](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties) by Benoit Blanchon
- [MQTT](https://github.com/256dpi/arduino-mqtt) by Joël Gähwiler

> **NOTE**
> Depending on the board you are using it may be necessary to include parts of
> the STL as external library. In this case add:
> `#define WITH_STL 1`
> before including the VRPC and install:
>
> [ArduinoSTL](https://github.com/mike-matera/ArduinoSTL) by Mike Matera

## Function adaptation macros

Use the macros described below to add remote access to regular functions.

### 1. Global Functions

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

## class `VrpcAgent`

The agent allows existing code to be called from remote.

## Summary

 Members                        | Descriptions
--------------------------------|---------------------------------------------
`public inline  `[`VrpcAgent`](#classVrpcAgent_1ace51d7fc67e6cca3db088b229292ded7)`(int maxBytesPerMessage)` | Constructs an agent.
`public template<>`  <br/>`inline void `[`begin`](#classVrpcAgent_1a5bcc3d82db137a8d4dd37f55ce83d53e)`(T & wifiClient,const String & domain,const String & token)` | Initializes the object using a client class for network transport.
`public inline bool `[`connected`](#classVrpcAgent_1aef4609a41a89bf7602011cca1fff5057)`()` | Reports the current connectivity status.
`public inline void `[`connect`](#classVrpcAgent_1afa4e6b81fcb0a990d5747b986adeecdb)`()` | Connect the agent to the broker.
`public inline void `[`loop`](#classVrpcAgent_1a89c5b7c6a84bccc8470b4bb8ff29a4ff)`()` | This function will send and receive VRPC packets.

## Members

### `public inline  `[`VrpcAgent`](#classVrpcAgent_1ace51d7fc67e6cca3db088b229292ded7)`(int maxBytesPerMessage)`

Constructs an agent.

#### Parameter

* `maxBytesPerMessage` [optional, default: `1024`] Specifies the maximum size a single MQTT message may have

- - -

### `public template<>`  <br/>`inline void `[`begin`](#classVrpcAgent_1a5bcc3d82db137a8d4dd37f55ce83d53e)`(T& wifiClient, const String& domain, const String& token)`

Initializes the object using a client class for network transport.

#### Parameter

* `wifiClient` A client class following the interface as described [here](https://www.arduino.cc/en/Reference/ClientConstructor)

* `domain` [optional, default: `"public.vrpc"`] The domain under which the agent-provided code is reachable

* `token` [optional, default: `""`] Access token as generated using the [VRPC App](https://app.vrpc.io), or password if own broker is used

* `broker` [optional, default: `"vrpc.io"`] Address of the MQTT broker

* `username` [optional] MQTT username (not needed when using the vrpc.io broker)

- - -

### `public inline bool `[`connected`](#classVrpcAgent_1aef4609a41a89bf7602011cca1fff5057)`()`

Reports the current connectivity status.

#### Returns

true when connected, false otherwise

- - -

### `public inline void `[`connect`](#classVrpcAgent_1afa4e6b81fcb0a990d5747b986adeecdb)`()`

Connect the agent to the broker.

The function will try to connect forever. Inspect the serial monitor to see the connectivity progress.

- - -

### `public inline void `[`loop`](#classVrpcAgent_1a89c5b7c6a84bccc8470b4bb8ff29a4ff)`()`

Send and receive VRPC packets.

NOTE: This function should be called in every `loop`
