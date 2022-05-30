# VRPC - Variadic Remote Procedure Calls

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/heisenware/vrpc-arduino/master/LICENSE)
[![Semver](https://img.shields.io/badge/semver-2.0.0-blue)](https://semver.org/spec/v2.0.0.html)
[![GitHub Releases](https://img.shields.io/github/tag/heisenware/vrpc-arduino.svg)](https://github.com/heisenware/vrpc-arduino/tag)
[![GitHub Issues](https://img.shields.io/github/issues/heisenware/vrpc-arduino.svg)](http://github.com/heisenware/vrpc-arduino/issues)

## Visit our website: [vrpc.io](https://vrpc.io)

## What is VRPC?

VRPC - Variadic Remote Procedure Calls - is an enhancement of the old RPC
(remote procedure calls) idea. Like RPC, it allows to directly call functions
written in any programming language by functions written in any other (or the
same) programming language. Unlike RPC, VRPC furthermore supports:

- non-intrusive adaption of existing code, making it callable remotely
- remote function calls on many distributed receivers at the same time (one
  client - multiple agents)
- service discovery
- outbound-connection-only network architecture (using MQTT technology)
- isolated (multi-tenant) and shared access modes to remote resources
- asynchronous language constructs (callbacks, promises, event-loops)
- OOP (classes, objects, member functions) and functional (lambda) patterns
- exception forwarding

VRPC is available for an entire spectrum of programming technologies including
embedded, data-science, and web technologies.

As a robust and highly performing communication system, it can build the
foundation of complex digitization projects in the area of (I)IoT or
Cloud-Computing.

## This is VRPC for Arduino

This library allows to adapt existing embedded functions by simply naming them
using a special macro. Once adapted, those functions may be called from any
remote location connected to the internet and by using any programming language
(e.g. Node.js).

Understand how to use it by looking at the examples:

- [Blink](examples/Blink_WiFi/Blink_WiFi.ino)
- [LCD Display](examples/LCD_Display_WiFi/LCD_Display_WiFi.ino)
- [Temperature](examples/Temperature_GSM/Temperature_GSM.ino)
- [GSM IR-Temperature](examples/Temperature_GSM/Temperature_GSM.ino)

Get all the details by reading the [documentation](docs/api.md).

> This open-source project is professionally managed and supported by
> [Heisenware GmbH](https://heisenware.com).
