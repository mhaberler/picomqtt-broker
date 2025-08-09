# PicoMQTT Broker example

A lightweight MQTT broker implementation for ESP32-S2, ESP32-S3 and ESP32-C6 devices using the PicoMQTT library. This project provides both TCP and WebSocket MQTT connectivity with button input functionality.

## Features

- **Dual Protocol Support**: MQTT over TCP (port 1883) and WebSocket (port 8883)
- **mDNS Discovery**: Automatic service discovery for both TCP and WebSocket endpoints
- **Button Input**: Physical button with single, double, and multi-click detection
- **Real-time Monitoring**: Connection, subscription, and message statistics
- **JSON Message Publishing**: Button clicks are published as JSON messages to the `button` topic
- **Debug Logging**: Comprehensive logging for debugging and monitoring

## Hardware Requirements

- ESP32-S2 development board (tested with M5Stick Cplus 1.1)
- ESP32-S3 development board (tested with M5Stack Atom-S3U)
- ESP32-C6 development board (tested with M5Stack Nano ESP32-C6)
- Physical button connected to GPIO (configurable)
- WiFi connectivity

## Dependencies

This project uses the following libraries:

- [PicoMQTT](https://github.com/mlesniew/PicoMQTT.git) - Lightweight MQTT implementation
- [PicoWebsocket](https://github.com/mlesniew/PicoWebsocket.git) - WebSocket support
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson.git) - JSON serialization
- [OneButton](https://github.com/mathertel/OneButton) - Button handling library

## Setup

### Environment Variables

Before building, set the following environment variables for WiFi credentials:

```bash
export WIFI_SSID5="your-wifi-ssid"
export WIFI_PASSWORD5="your-wifi-password"
```

### Building and Flashing

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repository
3. Set your WiFi credentials as environment variables
4. Build and upload:

```bash
pio run -e atom-s3u -t upload
```

### Monitoring

To view serial output:

```bash
pio device monitor -e atom-s3u
```

## Usage

### MQTT Connection

Once the device connects to WiFi, it will start the MQTT broker services:

- **TCP MQTT**: `mqtt://[device-ip]:1883`
- **WebSocket MQTT**: `ws://[device-ip]:8883/mqtt`

### mDNS Discovery

The broker advertises itself via mDNS as:
- TCP service: `picomqtt.local:1883`
- WebSocket service: `picomqtt.local:8883`

### Button Functionality

The physical button supports:
- **Single Click**: Publishes `{"clicks": 1}` to the `button` topic
- **Double Click**: Publishes `{"clicks": 2}` to the `button` topic
- **Multi Click**: Publishes `{"clicks": n}` to the `button` topic (where n is the number of clicks)

### MQTT Topics

- `button` - Receives button click events as JSON messages

## Configuration

### Pin Configuration

The button pin is configured in `platformio.ini`:

```ini
-DBUTTON_PIN=GPIO_NUM_41
```

### Port Configuration

Default ports can be modified in `main.cpp`:

```cpp
#define MQTT_PORT 1883
#define MQTTWS_PORT 8883
```

### Hostname

The mDNS hostname can be changed:

```cpp
const char *hostname = "picomqtt";
```

## Monitoring and Statistics

The broker tracks and logs:
- Client connections/disconnections
- Topic subscriptions/unsubscriptions
- Message routing
- WiFi connection status

## Development

### Debug Build

The project is configured for debug builds with:
- Debug symbols enabled (`-g`)
- Optimization level 2 (`-O2`)
- Core debug level 4 for detailed ESP32 logging
- Exception decoder for crash analysis

### Board Configuration

Currently configured for ESP32-S3-DevKitC-1 with:
- USB CDC on boot enabled
- Built-in debugging support
- Exception decoder for better error reporting

## NanoC6 boot log
```
-----------
Flash Info:
------------------------------------------
  Chip Size         :  4194304 B (4 MB)
  Block Size        :    65536 B (  64.0 KB)
  Sector Size       :     4096 B (   4.0 KB)
  Page Size         :      256 B (   0.2 KB)
  Bus Speed         : 40 MHz
  Bus Mode          : QIO
------------------------------------------
Partitions Info:
------------------------------------------
                nvs : addr: 0x00009000, size:    20.0 KB, type: DATA, subtype: NVS
            otadata : addr: 0x0000E000, size:     8.0 KB, type: DATA, subtype: OTA
               app0 : addr: 0x00010000, size:  1280.0 KB, type:  APP, subtype: OTA_0
               app1 : addr: 0x00150000, size:  1280.0 KB, type:  APP, subtype: OTA_1
             spiffs : addr: 0x00290000, size:  1408.0 KB, type: DATA, subtype: SPIFFS
           coredump : addr: 0x003F0000, size:    64.0 KB, type: DATA, subtype: COREDUMP
------------------------------------------
Software Info:
------------------------------------------
  Compile Date/Time : Aug  9 2025 11:35:49
  ESP-IDF Version   : v5.4.2-25-g858a988d6e
  Arduino Version   : 3.2.1
------------------------------------------
Board Info:
------------------------------------------
  Arduino Board     : M5stack NanoC6 ESP32C6
  Arduino Variant   : M5STACK_NANOC6
  Core Debug Level  : 4
  Arduino Runs Core : 0
  Arduino Events on : 0
  Arduino USB Mode  : 1
  CDC On Boot       : 1
============ Before Setup End ============
[  2342][I][esp32-hal-periman.c:141] perimanSetPinBus(): Pin 12 already has type USB_DM (38) with bus 0x408160a4
  #0  0x408160a4 in ?? at /Users/mah/.platformio/packages/framework-arduinoespressif32/cores/esp32/HWCDC.cpp:611

[  2343][I][esp32-hal-periman.c:141] perimanSetPinBus(): Pin 13 already has type USB_DP (39) with bus 0x408160a4
  #0  0x408160a4 in ?? at /Users/mah/.platformio/packages/framework-arduinoespressif32/cores/esp32/HWCDC.cpp:611

=========== After Setup Start ============
INTERNAL Memory Info:
------------------------------------------
  Total Size        :   418668 B ( 408.9 KB)
  Free Bytes        :   331700 B ( 323.9 KB)
  Allocated Bytes   :    77856 B (  76.0 KB)
  Minimum Free Bytes:   331660 B ( 323.9 KB)
  Largest Free Block:   303092 B ( 296.0 KB)
------------------------------------------
GPIO Info:
------------------------------------------
  GPIO : BUS_TYPE[bus/unit][chan]
  --------------------------------------
     9 : GPIO
    12 : USB_DM
    13 : USB_DP
    16 : UART_TX[0]
    17 : UART_RX[0]
============ After Setup End =============
[  5444][I][main.cpp:114] loop(): WiFi: disconnected
[  6570][W][STA.cpp:137] _onStaArduinoEvent(): Reason: 203 - ASSOC_FAIL
[  6571][D][STA.cpp:155] _onStaArduinoEvent(): WiFi Reconnect Running
[  6571][W][STA.cpp:543] disconnect(): STA already disconnected.
[  6712][I][main.cpp:117] loop(): WiFi status: 0
[  7784][I][main.cpp:99] loop(): WiFi: Connected, IP: 172.16.0.246
```




## esp32-p4 boot log

board is a https://www.waveshare.com/wiki/ESP32-P4-Module-DEV-KIT-StartPage#Overview

```
 Minimum Free Bytes:   552900 B ( 539.9 KB)
  Largest Free Block:   385012 B ( 376.0 KB)
------------------------------------------
SPIRAM Memory Info:
------------------------------------------
  Total Size        : 33554432 B (32768.0 KB)
  Free Bytes        : 33551856 B (32765.5 KB)
  Allocated Bytes   :        0 B (   0.0 KB)
  Minimum Free Bytes: 33551856 B (32765.5 KB)
  Largest Free Block: 33030132 B (32256.0 KB)
  Bus Mode          : QSPI
------------------------------------------
Flash Info:
------------------------------------------
  Chip Size         : 16777216 B (16 MB)
  Block Size        :    65536 B (  64.0 KB)
  Sector Size       :     4096 B (   4.0 KB)
  Page Size         :      256 B (   0.2 KB)
  Bus Speed         : 80 MHz
  Bus Mode          : QIO
------------------------------------------
Partitions Info:
------------------------------------------
                nvs : addr: 0x00009000, size:    20.0 KB, type: DATA, subtype: NVS
            otadata : addr: 0x0000E000, size:     8.0 KB, type: DATA, subtype: OTA
               app0 : addr: 0x00010000, size:  1280.0 KB, type:  APP, subtype: OTA_0
               app1 : addr: 0x00150000, size:  1280.0 KB, type:  APP, subtype: OTA_1
             spiffs : addr: 0x00290000, size:  1408.0 KB, type: DATA, subtype: SPIFFS
           coredump : addr: 0x003F0000, size:    64.0 KB, type: DATA, subtype: COREDUMP
------------------------------------------
Software Info:
------------------------------------------
  Compile Date/Time : Aug  9 2025 11:39:12
  ESP-IDF Version   : v5.4.2-25-g858a988d6e
  Arduino Version   : 3.2.1
------------------------------------------
Board Info:
------------------------------------------
  Arduino Board     : Espressif ESP32-P4 Function EV Board
  Arduino Variant   : esp32p4
  Core Debug Level  : 5
  Arduino Runs Core : 1
  Arduino Events on : 1
  Arduino USB Mode  : 1
  CDC On Boot       : 1
============ Before Setup End ============
[  4021][I][esp32-hal-periman.c:141] perimanSetPinBus(): Pin 24 already has type USB_DM (45) with bus 0x4ff139f0
  #0  0x4ff139f0 in ?? at /Users/mah/.platformio/packages/framework-arduinoespressif32/cores/esp32/HWCDC.cpp:611

[  4022][I][esp32-hal-periman.c:141] perimanSetPinBus(): Pin 25 already has type USB_DP (46) with bus 0x4ff139f0
  #0  0x4ff139f0 in ?? at /Users/mah/.platformio/packages/framework-arduinoespressif32/cores/esp32/HWCDC.cpp:611

sdio_mempool_create free:34104436 min-free:34104436 lfb-def:33030132 lfb-8bit:33030132

[  7024][V][WiFiGeneric.cpp:316] wifiHostedInit(): ESP-HOSTED initialized!
[  9735][V][NetworkEvents.cpp:117] _checkForEvent(): Network Event: 101 - WIFI_READY
rted
[  9863][V][NetworkEvents.cpp:117] _checkForEvent(): Network Event: 110 - STA_START
[  9864][V][STA.cpp:110] _onStaArduinoEvent(): Arduino STA Event: 110 - STA_START
=========== After Setup Start ============
INTERNAL Memory Info:
------------------------------------------
  Total Size        :   594616 B ( 580.7 KB)
  Free Bytes        :   491700 B ( 480.2 KB)
  Allocated Bytes   :    94404 B (  92.2 KB)
  Minimum Free Bytes:   490220 B ( 478.7 KB)
  Largest Free Block:   385012 B ( 376.0 KB)
------------------------------------------
SPIRAM Memory Info:
------------------------------------------
  Total Size        : 33554432 B (32768.0 KB)
  Free Bytes        : 33551692 B (32765.3 KB)
  Allocated Bytes   :      116 B (   0.1 KB)
  Minimum Free Bytes: 33551692 B (32765.3 KB)
  Largest Free Block: 33030132 B (32256.0 KB)
------------------------------------------
GPIO Info:
------------------------------------------
  GPIO : BUS_TYPE[bus/unit][chan]
  --------------------------------------
    24 : USB_DM
    25 : USB_DP
    37 : UART_TX[0]
    38 : UART_RX[0]
============ After Setup End =============
[  9933][I][main.cpp:114] loop(): WiFi: disconnected
:ce:fe, Reason: 2
[ 13701][V][NetworkEvents.cpp:117] _checkForEvent(): Network Event: 113 - STA_DISCONNECTED
[ 13701][V][STA.cpp:110] _onStaArduinoEvent(): Arduino STA Event: 113 - STA_DISCONNECTED
[ 13702][W][STA.cpp:137] _onStaArduinoEvent(): Reason: 2 - AUTH_EXPIRE
[ 13702][D][STA.cpp:155] _onStaArduinoEvent(): WiFi Reconnect Running
[ 13702][W][STA.cpp:543] disconnect(): STA already disconnected.
e, Reason: 2
[ 17525][V][NetworkEvents.cpp:117] _checkForEvent(): Network Event: 113 - STA_DISCONNECTED
[ 17525][V][STA.cpp:110] _onStaArduinoEvent(): Arduino STA Event: 113 - STA_DISCONNECTED
[ 17525][W][STA.cpp:137] _onStaArduinoEvent(): Reason: 2 - AUTH_EXPIRE
[ 17526][D][STA.cpp:158] _onStaArduinoEvent(): WiFi AutoReconnect Running
[ 17526][W][STA.cpp:543] disconnect(): STA already disconnected.
 Reason: 2
[ 21314][V][NetworkEvents.cpp:117] _checkForEvent(): Network Event: 113 - STA_DISCONNECTED
[ 21315][V][STA.cpp:110] _onStaArduinoEvent(): Arduino STA Event: 113 - STA_DISCONNECTED
[ 21315][W][STA.cpp:137] _onStaArduinoEvent(): Reason: 2 - AUTH_EXPIRE
[ 21315][D][STA.cpp:158] _onStaArduinoEvent(): WiFi AutoReconnect Running
[ 21315][W][STA.cpp:543] disconnect(): STA already disconnected.
[ 25108][V][NetworkEvents.cpp:117] _checkForEvent(): Network Event: 113 - STA_DISCONNECTED
[ 25108][V][STA.cpp:110] _onStaArduinoEvent(): Arduino STA Event: 113 - STA_DISCONNECTED
[ 25108][W][STA.cpp:137] _onStaArduinoEvent(): Reason: 2 - AUTH_EXPIRE
[ 25109][D][STA.cpp:158] _onStaArduinoEvent(): WiFi AutoReconnect Running
[ 25109][W][STA.cpp:543] disconnect(): STA already disconnected.
```