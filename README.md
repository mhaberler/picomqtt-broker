# PicoMQTT Broker example

A lightweight MQTT broker implementation for ESP32-S3 and ESP32-C6 devices using the PicoMQTT library. This project provides both TCP and WebSocket MQTT connectivity with button input functionality.

## Features

- **Dual Protocol Support**: MQTT over TCP (port 1883) and WebSocket (port 8883)
- **mDNS Discovery**: Automatic service discovery for both TCP and WebSocket endpoints
- **Button Input**: Physical button with single, double, and multi-click detection
- **Real-time Monitoring**: Connection, subscription, and message statistics
- **JSON Message Publishing**: Button clicks are published as JSON messages to the `button` topic
- **Debug Logging**: Comprehensive logging for debugging and monitoring

## Hardware Requirements

- ESP32-S3 development board (tested with M5Stack Atom-S3U)
- ESP32-C6 development board (tested with M5Stack Nano ESP32-C6)
- Physical button connected to GPIO (configurable)
- WiFi connectivity

**Note**: The Arduino Nano ESP32-C6 is a compact form factor board that works well for this project with minimal modifications.

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

## License

This project is part of the BalloonWare networking suite.

## Contributing

Please ensure all commits include proper testing on the target hardware and maintain compatibility with the existing API.
