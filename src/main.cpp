#include "OneButton.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <PicoMQTT.h>
#include <PicoWebsocket.h>
#include <WiFi.h>
#include "ESP_HostedOTA.h" // ESP-Hosted OTA update functionality
#include "BLEScanner.h"

#define MQTT_PORT 1883
#define MQTTWS_PORT 8883
const char *hostname = "picomqtt";

void setup_ble(void);
void process_ble(void);

wl_status_t wifi_status = WL_STOPPED;
WiFiServer tcp_server(MQTT_PORT);
WiFiServer websocket_underlying_server(MQTTWS_PORT);
PicoWebsocket::Server<::WiFiServer>
websocket_server(websocket_underlying_server);
static auto &bleScanner = BLEScanner::instance();

class CustomMQTTServer : public PicoMQTT::Server {
    using PicoMQTT::Server::Server;

  public:
    int32_t connected, subscribed, messages;

  protected:
    void on_connected(const char *client_id) override {
        log_i("client %s connected", client_id);
        connected++;
    }
    virtual void on_disconnected(const char *client_id) override {
        log_i("client %s disconnected", client_id);
        connected--;
    }
    virtual void on_subscribe(const char *client_id, const char *topic) override {
        log_i("client %s subscribed %s", client_id, topic);
        subscribed++;
    }
    virtual void on_unsubscribe(const char *client_id,
                                const char *topic) override {
        log_i("client %s unsubscribed %s", client_id, topic);
        subscribed--;
    }
    virtual void on_message(const char *topic,
                            PicoMQTT::IncomingPacket &packet) override {
        log_i("message topic=%s", topic);
        PicoMQTT::Server::Server::on_message(topic, packet);
        messages++;
    }
};

CustomMQTTServer mqtt(tcp_server, websocket_server);

int numClicks = 0;

#ifdef BUTTON_PIN
OneButton button(BUTTON_PIN, true,
                 true); // Button pin, active low, pullup enabled

void singleClick() {
    log_i("singleClick() detected.");
    numClicks = 1;
}

void doubleClick() {
    log_i("doubleClick() detected.");
    numClicks = 2;
}

void multiClick() {
    int n = button.getNumberClicks();
    log_i("multiClick clicks = %d", n);
    numClicks = n;
}
#endif

void setup() {
    Serial.begin(115200);
    delay(3000);
#ifdef BUTTON_PIN
    button.attachClick(singleClick);
    button.attachDoubleClick(doubleClick);
    button.attachMultiClick(multiClick);
#endif

    // WiFi.begin(WIFI_SSID, WIFI_PASS);
    // variant: ESP32C6
    // # SDIO pins for ESP32-C6 communication
    // clk_pin: GPIO18
    // cmd_pin: GPIO19
    // d0_pin: GPIO14
    // d1_pin: GPIO15
    // d2_pin: GPIO16
    // d3_pin: GPIO17
    // # Reset pin
    // reset_pin: GPIO54
    // active_high: true

    // if (WiFi.setPins(18, 19, 14, 15, 16, 17, 54)) {  // Example: CLK=18,   CMD=19, D0=14, D1=15, D2=16, D3=17, RST=54
    //     Serial.println("SDIO pins set");
    //   }
    WiFi.STA.begin();

    // Step 4: Attempt to connect to the specified WiFi network
    WiFi.STA.connect(WIFI_SSID, WIFI_PASS);

    // Step 5: Wait for WiFi connection to be established
    // Display progress dots while connecting
    while (WiFi.STA.status() != WL_CONNECTED) {
        delay(500);        // Wait 500ms between connection attempts
        Serial.print("."); // Show connection progress
    }
    Serial.println();

    // Step 6: Display successful connection information
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.STA.localIP());
    WiFi.printDiag(Serial);

    // Step 7: Attempt to update the ESP-Hosted co-processor firmware
    // This function will:
    // - Check if ESP-Hosted is initialized
    // - Verify if an update is available
    // - Download and install the firmware update if needed
    if (updateEspHostedSlave()) {
        // Step 8: Restart the host ESP32 after successful update
        // This is currently required to properly activate the new firmware
        // on the ESP-Hosted co-processor
        ESP.restart();
    }
    bleScanner.begin(4096, 15000, 100, 99, 4096, 1, MALLOC_CAP_DEFAULT);
}

void loop() {
    static unsigned long last_report = 0; // last report time
    wl_status_t ws = WiFi.status();
    if (ws ^ wifi_status) {
        wifi_status = ws; // track changes
        switch (ws) {
            case WL_CONNECTED:
                log_i("WiFi: Connected, IP: %s", WiFi.localIP().toString().c_str());
                // if (esp_read_mac(mac, mtype) != ESP_OK) {
                //   log_e("Failed to read the MAC address");
                //   return;
                // }
                // sprintf(winstance, "%s [%02x:%02x:%02x:%02x:%02x:%02x]",
                // _hostname.c_str(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

                if (MDNS.begin(hostname)) {
                    MDNS.addService("mqtt", "tcp", MQTT_PORT);
                    MDNS.addService("mqtt-ws", "tcp", MQTTWS_PORT);
                    MDNS.addServiceTxt("mqtt-ws", "tcp", "path", "/mqtt");
                    mdns_service_instance_name_set("_mqtt", "_tcp", "PicoMQTT TCP broker");
                    mdns_service_instance_name_set("_mqtt-ws", "_tcp",
                                                   "PicoMQTT Websockets broker");
                }
                mqtt.begin();
                break;
            case WL_NO_SSID_AVAIL:
                log_i("WiFi: SSID %s not found", WIFI_SSID);
                break;
            case WL_DISCONNECTED:
                log_i("WiFi: disconnected");
                break;
            default:
                log_i("WiFi status: %d", ws);
                break;
        }
        delay(300);
    }
#ifdef BUTTON_PIN
    button.tick();
#endif
    if (millis() - last_report > 500) {
        if (numClicks > 0) {
            JsonDocument output;
            output["clicks"] = numClicks;
            auto publish = mqtt.begin_publish("button", measureJson(output));
            serializeJson(output, publish);
            publish.send();
            log_w("numClicks: %d", numClicks);
            numClicks = 0;
        }
        last_report = millis();
    }
    {
        JsonDocument doc;
        char mac[16];
        if (bleScanner.process(doc, mac, sizeof(mac))) {
            String topic = String("ble/") + mac;
            auto publish = mqtt.begin_publish(topic.c_str(), measureJson(doc));
            serializeJson(doc, publish);
            publish.send();
        }
    }
    {
        static BLEScanner::Stats lastStats = {};
        auto st = bleScanner.stats();
        if (st.hwmBytes > lastStats.hwmBytes ||
                st.queueFull > lastStats.queueFull ||
                st.acquireFail > lastStats.acquireFail ||
                st.received > lastStats.received ||
                st.decoded > lastStats.decoded) {
            lastStats = st;
            JsonDocument sdoc;
            sdoc["hwm"] = st.hwmPercent;
            sdoc["qfull"] = st.queueFull;
            sdoc["afail"] = st.acquireFail;
            sdoc["rx"] = st.received;
            sdoc["dec"] = st.decoded;
            auto publish = mqtt.begin_publish("ble/$stats", measureJson(sdoc));
            serializeJson(sdoc, publish);
            publish.send();
        }
    }

    mqtt.loop();
    delay(10);
}
