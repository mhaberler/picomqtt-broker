# PicoMQTT Broker - AI Assistant Context

## Project Overview

This is a lightweight MQTT broker implementation for ESP32 microcontrollers (S2, S3, C6, P4 variants) that combines MQTT message brokering with BLE scanning capabilities. The project publishes BLE advertisement data from nearby devices to MQTT topics, enabling IoT sensor integration and monitoring.

**Primary Use Case**: Act as an MQTT broker while simultaneously scanning for BLE advertisements from smart sensors (Ruuvi tags, Mopeka tank sensors, TPMS sensors, BTHome devices, etc.) and publishing their data to MQTT topics.

## Architecture

### Core Components

1. **MQTT Broker** (`CustomMQTTServer`)
   - Dual-protocol support: TCP (port 1883) and WebSocket (port 8883)
   - Custom server class extending `PicoMQTT::Server`
   - Tracks connection statistics (connected, subscribed, messages)
   - mDNS service advertisement for auto-discovery

2. **BLE Scanner** (`BLEScanner`)
   - Singleton pattern with dedicated FreeRTOS task
   - Ring buffer-based message queue for BLE advertisements
   - Built-in decoders for multiple sensor types
   - Stats tracking (high water mark, queue full events, decoded messages)

3. **Button Handler** (`OneButton`)
   - GPIO button input with debouncing
   - Single, double, and multi-click detection
   - Publishes click events as JSON to `button` topic

4. **WiFi Management**
   - ESP-Hosted SDIO support for WiFi connectivity
   - Automatic OTA firmware updates for ESP-Hosted slave
   - Connection state tracking and reporting

### Data Flow

```
BLE Devices → BLE Scanner Task → Ring Buffer → Main Loop → JSON Encoder → MQTT Publish
                                                                              ↓
Button Input → Event Handler → JSON Encoder → MQTT Publish              MQTT Clients
```

### Threading Model

- **Main Loop (Arduino core 0)**: WiFi management, MQTT broker loop, BLE message processing, button polling
- **BLE Scanner Task (FreeRTOS)**: Dedicated task for BLE scanning, queues raw advertisement data

## File Structure

### Source Files (`src/`)

- **main.cpp** - Main entry point, WiFi setup, MQTT initialization, loop logic
- **BLEScanner.h/cpp** - BLE scanning singleton with built-in device decoders
- **broker.hpp** - MQTT broker instance declaration (legacy, mostly unused)
- **util.h/cpp** - Utility functions
- **ringbuffer.hpp** - Ring buffer implementation for BLE message queue
- **tickers.hpp** - Timer/ticker utilities
- **fmicro.h** - Helper macros and functions

### Untracked Files (`untracked/`)

Development/testing code not included in main build.

### Board Definitions (`boards/`)

Custom board definitions for:
- `esp32-c5-devkitc1-n16r8.json` - ESP32-C5 dev kit
- `m5stack_nanoc6.json` - M5Stack Nano C6

### Configuration

- **platformio.ini** - Build configuration, board definitions, environment variables, dependencies

## Dependencies

All dependencies managed via PlatformIO `lib_deps`:

- **PicoMQTT** - Lightweight MQTT broker implementation
- **PicoWebsocket** - WebSocket transport for MQTT
- **ArduinoJson** - JSON serialization/deserialization (v7+ with long long support)
- **OneButton** - Button event detection library
- **BTHomeDecoder** - BTHome v2 protocol decoder with AES decryption support

## Build System (PlatformIO)

### Environment Variables

Required credentials in shell environment:
```bash
export WIFI_SSID="your-ssid"
export WIFI_PASSWORD="your-password"
```

### Build Targets

- `atom-s3u` - M5Stack Atom S3U (ESP32-S3, button on GPIO41)
- `seeed_xiao_esp32c6` - Seeed XIAO ESP32-C6
- `m5stack_nanoc6` - M5Stack Nano C6 (button on GPIO9)
- `m5stick-c` - M5Stick C (ESP32, button on GPIO37)
- `m5stamp-c3u` - M5Stamp C3U (button on GPIO9, RGB LED on GPIO2)
- `esp32p4_waveshare_devkit` - Waveshare ESP32-P4 dev kit

### Common Build Flags

```ini
-g -O2
-DCORE_DEBUG_LEVEL=2
-DCUSTOM_ARDUINO_LOOP_STACK_SIZE=32768
-DPICOWEBSOCKET_MAX_HTTP_LINE_LENGTH=512
```

### Build Commands

```bash
# Build for specific environment
pio run -e atom-s3u

# Upload firmware
pio run -e atom-s3u -t upload

# Monitor serial output
pio device monitor -e atom-s3u
```

## Hardware Support

### Tested Boards

- ESP32-S2 (M5Stick C Plus 1.1)
- ESP32-S3 (M5Stack Atom-S3U)
- ESP32-C3 (M5Stamp C3U)
- ESP32-C6 (M5Stack Nano C6, Seeed XIAO)
- ESP32-P4 (Waveshare dev kit)

### GPIO Configuration

Button pins are per-environment:
- Atom S3U: GPIO41
- M5Stack Nano C6: GPIO9
- M5Stick C: GPIO37
- M5Stamp C3U: GPIO9

### WiFi Connectivity

- Standard WiFi for most boards
- ESP-Hosted SDIO support for ESP32-C6 (pins: CLK=18, CMD=19, D0=14, D1=15, D2=16, D3=17, RST=54)

## Key Code Patterns

### Custom MQTT Server

The `CustomMQTTServer` class extends `PicoMQTT::Server` to add statistics tracking:

```cpp
class CustomMQTTServer : public PicoMQTT::Server {
    int32_t connected, subscribed, messages;

    void on_connected(const char *client_id) override;
    void on_disconnected(const char *client_id) override;
    void on_subscribe(const char *client_id, const char *topic) override;
    void on_message(const char *topic, PicoMQTT::IncomingPacket &packet) override;
};
```

### BLE Message Processing

Main loop drains BLE scanner queue and publishes to MQTT:

```cpp
JsonDocument doc;
char mac[16];
if (bleScanner.process(doc, mac, sizeof(mac))) {
    String topic = String("ble/") + mac;
    auto publish = mqtt.begin_publish(topic.c_str(), measureJson(doc));
    serializeJson(doc, publish);
    publish.send();
}
```

### Button Click Publishing

Button clicks are published as JSON with retry on state change:

```cpp
JsonDocument output;
output["clicks"] = numClicks;
auto publish = mqtt.begin_publish("button", measureJson(output));
serializeJson(output, publish);
publish.send();
```

## MQTT Topics

### Published Topics

- `button` - Button click events: `{"clicks": n}`
- `ble/{MAC}` - BLE device data keyed by MAC address (colon-stripped uppercase)
- `ble/$stats` - Scanner statistics: `{"hwm": %, "qfull": n, "afail": n, "rx": n, "dec": n}`

### Topic Naming

- BLE device topics use MAC addresses without colons (e.g., `ble/AABBCCDDEEFF`)
- All BLE data includes device-specific fields from decoders

## BLE Scanner Details

### Supported Device Types

The BLE scanner has built-in decoders for:

1. **Ruuvi Tag** (V5 format) - Temperature, humidity, pressure sensors
2. **Mopeka** - Propane tank level sensors
3. **TPMS** (0x0100, 0x00AC) - Tire pressure monitoring systems
4. **Otodata** - Tank monitors
5. **Rotarex ELG** - Level gauges
6. **BTHome v2** - Generic BTHome protocol with optional AES decryption

### Configuration

```cpp
bleScanner.begin(
    4096,   // ringBufSize - ring buffer size in bytes
    15000,  // scanTimeMs - scan window duration
    100,    // scanInterval - scan interval (BLE units)
    99,     // scanWindow - scan window (BLE units)
    4096,   // taskStackSize - FreeRTOS task stack
    1,      // taskPriority - FreeRTOS priority
    MALLOC_CAP_DEFAULT // memory allocation caps
);
```

### Statistics

Scanner provides real-time stats:
- **hwmPercent** - Peak buffer usage percentage
- **queueFull** - Number of dropped messages (queue overflow)
- **acquireFail** - Failed buffer acquisitions
- **received** - Total messages processed
- **decoded** - Messages successfully decoded

## Development Guidelines

### Adding New BLE Decoders

1. Add decoder logic to `BLEScanner.cpp` in the `deliver()` method
2. Match on service UUID or manufacturer data
3. Populate JsonDocument with decoded fields
4. Return true to mark as decoded

### Memory Considerations

- Stack size increased to 32KB (`CUSTOM_ARDUINO_LOOP_STACK_SIZE`)
- Ring buffer sizes tunable per deployment
- JSON documents use dynamic allocation (heap)

### Debug Logging

Debug level controlled via `CORE_DEBUG_LEVEL`:
- `2` - Warnings and errors (default)
- `4` - Verbose debug output

Use ESP-IDF logging macros:
- `log_i()` - Info
- `log_w()` - Warning
- `log_e()` - Error

### Error Handling

The project uses ESP-IDF error checking patterns:
- WiFi connection status monitoring with state change detection
- BLE scanner queue overflow tracking
- MQTT connection state callbacks

## Network Services

### mDNS Advertisement

Broker advertises two services:
- `_mqtt._tcp` on port 1883 - "PicoMQTT TCP broker"
- `_mqtt-ws._tcp` on port 8883 - "PicoMQTT Websockets broker" with TXT record `path=/mqtt`

Hostname: `picomqtt.local`

### Security

- No authentication/authorization on MQTT broker (suitable for trusted LAN only)
- BTHome devices support AES-128-CCM encryption (configure via `setBTHomeKey()`)
- WiFi credentials via environment variables (not committed to repo)

## Common Tasks

### Adding a New Board

1. Add environment in `platformio.ini`
2. Set correct board identifier
3. Configure `BUTTON_PIN` if applicable
4. Set USB CDC flags for USB-based boards

### Modifying MQTT Topics

Edit `main.cpp`:
- Button topic: line ~185 (`mqtt.begin_publish("button", ...)`)
- BLE device topic: line ~197 (`String topic = String("ble/") + mac`)
- BLE stats topic: line ~217 (`mqtt.begin_publish("ble/$stats", ...)`)

### Tuning BLE Scanner

Adjust parameters in `main.cpp` `setup()`:
```cpp
bleScanner.begin(
    bufferSize,    // Larger = more buffering, more RAM
    scanTimeMs,    // Longer = more devices found, higher latency
    scanInterval,  // BLE timing (consult BLE spec)
    scanWindow,    // BLE timing (consult BLE spec)
    stackSize,     // Task stack (increase if task crashes)
    priority,      // FreeRTOS priority (1 = low)
    memCaps        // Memory allocation flags
);
```

### Enabling BTHome Encryption

Add to `setup()`:
```cpp
bleScanner.setBTHomeKey("0123456789abcdef0123456789abcdef"); // 32 hex chars
```

## Known Issues & Limitations

1. **ESP-Hosted OTA**: Requires ESP.restart() after firmware update (not hot-swappable)
2. **Active Scanning**: Currently disabled (passive scan only) - call `setActiveScan(true)` before `begin()` to enable
3. **MQTT Security**: No TLS, authentication, or authorization
4. **Memory**: Large stack required for JSON processing (32KB)
5. **BLE Scanner**: Queue can overflow on high advertisement density (monitor `ble/$stats` topic)

## Testing

### Manual Testing

1. Connect ESP32 to WiFi network
2. Use MQTT client (mosquitto_sub, MQTT Explorer, etc.)
3. Subscribe to `ble/#` for all BLE devices
4. Subscribe to `button` for button clicks
5. Place BLE sensors nearby and observe publications

### Debug Output

Monitor serial port for:
- WiFi connection status
- mDNS registration
- MQTT client connections/disconnections
- BLE scan events
- Button click detection

## Future Enhancements

### Potential Improvements

1. Add MQTT authentication/authorization
2. Implement TLS for secure connections
3. Add configuration via MQTT topics or web interface
4. Support for additional BLE device types
5. Persistent configuration storage (NVS)
6. OTA firmware updates via MQTT
7. Energy monitoring and sleep modes
8. Buffering when WiFi disconnected

### Architecture Considerations

- Consider moving button handling to separate task for better responsiveness
- Implement backpressure mechanism when MQTT clients are slow
- Add telemetry topic for device health monitoring (uptime, free heap, WiFi RSSI)

## Troubleshooting

### WiFi Won't Connect

- Verify environment variables: `echo $WIFI_SSID`
- Check serial output for specific error codes
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)

### BLE Devices Not Detected

- Check `ble/$stats` for queue overflow
- Increase ring buffer size in `bleScanner.begin()`
- Verify device is advertising (use smartphone BLE scanner app)
- Enable active scanning: `bleScanner.setActiveScan(true)`

### MQTT Clients Can't Connect

- Verify mDNS working: `ping picomqtt.local`
- Check firewall rules (ports 1883, 8883)
- Test with IP address instead of hostname
- Monitor serial for connection attempts

### Out of Memory Crashes

- Reduce ring buffer size
- Decrease BLE scan window
- Lower task stack sizes
- Monitor free heap: `ESP.getFreeHeap()`

## Code Review Checklist

When modifying this project, ensure:

- [ ] WiFi credentials not hardcoded
- [ ] Memory allocations checked (heap, stack)
- [ ] JSON document sizes appropriate (`JsonDocument` capacity)
- [ ] MQTT publish calls check return values
- [ ] BLE decoder returns correct boolean
- [ ] GPIO pins match target board
- [ ] Debug logging at appropriate level
- [ ] FreeRTOS task stack sizes adequate
- [ ] No blocking operations in RTOS callbacks
- [ ] mDNS services registered after WiFi connect

## Related Documentation

- [PicoMQTT Library](https://github.com/mlesniew/PicoMQTT)
- [PicoWebsocket Library](https://github.com/mlesniew/PicoWebsocket)
- [ArduinoJson Documentation](https://arduinojson.org/)
- [BTHome Protocol](https://bthome.io/)
- [ESP-IDF WiFi Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [PlatformIO ESP32 Platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)
