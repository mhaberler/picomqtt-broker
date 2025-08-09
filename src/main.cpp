#include "OneButton.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <PicoMQTT.h>
#include <PicoWebsocket.h>
#include <WiFi.h>

#define MQTT_PORT 1883
#define MQTTWS_PORT 8883
const char *hostname = "picomqtt";

wl_status_t wifi_status = WL_STOPPED;
WiFiServer tcp_server(MQTT_PORT);
WiFiServer websocket_underlying_server(MQTTWS_PORT);
PicoWebsocket::Server<::WiFiServer>
    websocket_server(websocket_underlying_server);

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

  WiFi.begin(WIFI_SSID, WIFI_PASS);

}

void loop() {
  static unsigned long last_report = 0; // last report time
  wl_status_t ws = WiFi.status();
  if (ws ^ wifi_status) {
    wifi_status = ws; // track changes
    switch (ws) {
    case WL_CONNECTED:
      log_i("WiFi: Connected, IP: %s", WiFi.localIP().toString().c_str());
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
      numClicks = 0;
    }
    last_report = millis();
  }
  mqtt.loop();
  yield();
}
