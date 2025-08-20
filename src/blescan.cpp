#include <Arduino.h>
#include "freertos/ringbuf.h"
#include "ringbuffer.hpp"
#include "NimBLEDevice.h"

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"
#include "broker.hpp"
#include "util.hpp"
#include "fmicro.h"
#include "tickers.hpp"

// snarfed from OpenMQTT Gateway
#ifndef BLEScanInterval
#define BLEScanInterval                                                        \
  52 // How often the scan occurs / switches channels; in milliseconds,
#endif
#ifndef BLEScanWindow
#define BLEScanWindow                                                          \
  30 // How long to scan during the interval; in milliseconds.
#endif
#ifndef Scan_duration
#define Scan_duration 10000 // define the duration for a scan; in milliseconds
#endif
#ifndef TimeBtwActive
// #define TimeBtwActive 55555 //define default time between two BLE active
// scans when general passive scanning is selected; in milliseconds
#define TimeBtwActive                                                          \
  1000 * 3600 // define default time between two BLE active scans when general
              // passive scanning is selected; in milliseconds
#endif

#ifndef BLE_ADV_QUEUELEN
#define BLE_ADV_QUEUELEN 2048
#endif
// move queue to PSRAM if possible
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 1) && defined(BOARD_HAS_PSRAM)
#define BLE_ADV_QUEUE_MEMTYPE MALLOC_CAP_SPIRAM
#else
#define BLE_ADV_QUEUE_MEMTYPE MALLOC_CAP_INTERNAL
#endif

static NimBLEScan *pBLEScan;
static espidf::RingBuffer *bleadv_queue;
static uint32_t queue_full, acquire_fail;

static TICKER(activeScan, TimeBtwActive);
static constexpr uint32_t scanTime = 15 * 1000; // 30 seconds scan time.
TICKER(ble, 1800 * 1000);

class scanCallbacks : public NimBLEScanCallbacks {

  void onResult(const NimBLEAdvertisedDevice *advertisedDevice) {
    // log_i("Discovered Device: %s", advertisedDevice->toString().c_str());
    onDiscovered(advertisedDevice);
  }

  void onDiscovered(const NimBLEAdvertisedDevice *advertisedDevice) {
    // log_i("Advertised Device: %s", advertisedDevice->toString().c_str());
    JsonDocument doc;
    JsonObject BLEdata = doc.to<JsonObject>();
    String mac_adress = advertisedDevice->getAddress().toString().c_str();
    mac_adress.toUpperCase();
    BLEdata["id"] = (char *)mac_adress.c_str();
    BLEdata["connectable"] = advertisedDevice->isConnectable();
    BLEdata["rssi"] = (int)advertisedDevice->getRSSI();
    BLEdata["payload"] = NimBLEUtils::dataToHexString(
        advertisedDevice->getPayload().data(),
        static_cast<uint8_t>(advertisedDevice->getPayload().size()));

    if (advertisedDevice->haveName())
      BLEdata["name"] = (char *)advertisedDevice->getName().c_str();
    if (advertisedDevice->haveAppearance())
      BLEdata["appearance"] = advertisedDevice->getAppearance();
    if (advertisedDevice->haveManufacturerData()) {
      BLEdata["manufacturerdata"] = NimBLEUtils::dataToHexString(
          (uint8_t *)advertisedDevice->getManufacturerData().data(),
          advertisedDevice->getManufacturerData().length());
    }

    if (advertisedDevice->haveTXPower())
      BLEdata["txpower"] = (int8_t)advertisedDevice->getTXPower();

    if (advertisedDevice->haveServiceData()) {
      JsonObject nested = BLEdata["serviceData"].to<JsonObject>();
      for (int j = 0; j < advertisedDevice->getServiceDataCount(); j++) {
        nested[advertisedDevice->getServiceDataUUID(j).to128().toString()] =
            NimBLEUtils::dataToHexString(
                (const uint8_t *)advertisedDevice->getServiceData(j).c_str(),
                advertisedDevice->getServiceData(j).length());
      }
    }

    BLEdata["time"] = fseconds();
    void *ble_adv = nullptr;
    size_t total = measureMsgPack(BLEdata);
    if (bleadv_queue->send_acquire((void **)&ble_adv, total, 0) != pdTRUE) {
      acquire_fail++;
      return;
    }
    size_t n = serializeMsgPack(BLEdata, ble_adv, total);
    if (n != total) {
      log_e("serializeMsgPack: expected %u got %u", total, n);
    } else {
      if (bleadv_queue->send_complete(ble_adv) != pdTRUE) {
        queue_full++;
      }
    }
  }
  void onScanEnd(const NimBLEScanResults &results, int reason) override {
    NimBLEScan *pBLEScan = NimBLEDevice::getScan();
    if (TIME_FOR(activeScan)) {
      log_i("active scan");
      pBLEScan->setActiveScan(true);
      DONE_WITH(activeScan);
    } else {
      pBLEScan->setActiveScan(false);
      log_i("passive scan");
    }
    log_i("Scan Ended, restarting");
    pBLEScan->start(scanTime, false, false);
  }
};

void setup_ble(void) {
  bleadv_queue = new espidf::RingBuffer();
  bleadv_queue->create(BLE_ADV_QUEUELEN, RINGBUF_TYPE_NOSPLIT);

  NimBLEDevice::init(
      ""); // Initialize the device, you can specify a device name if you want.
  NimBLEScan *pBLEScan = NimBLEDevice::getScan(); // Create the scan object.
  pBLEScan->setScanCallbacks(new scanCallbacks(),
                             true); // Set the callback for when devices are
                                    // discovered, no duplicates.
  pBLEScan->setActiveScan(true); // Set active scanning, this will get more data
                                 // from the advertiser.
  pBLEScan->setMaxResults(
      0); // Do not store the scan results, use callback only.
  pBLEScan->start(scanTime, false,
                  false); // duration, not a continuation of last scan, restart
                          // to get all devices again.
  RUNTICKER(ble);
  log_i("Scanning...");
}

void bleDeliver(JsonDocument &doc) {
  auto publish = mqtt.begin_publish("ble/", measureJson(doc));
  serializeJson(doc, publish);
  publish.send();
}

void process_ble(void) {
  size_t size = 0;
  void *buffer = bleadv_queue->receive(&size, 0);
  if (buffer == nullptr) {
    return;
  }
  JsonDocument doc;
  DeserializationError e = deserializeMsgPack(doc, buffer, size);
  bleDeliver(doc);
  bleadv_queue->return_item(buffer);
}
