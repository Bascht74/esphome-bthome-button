#include "bthome.h"

#include "codec.h"

#include "esphome/core/log.h"

#ifdef USE_ESP32
#include "esphome/components/esp32_ble/ble.h"

#include <span>
#endif

namespace esphome {
namespace bthome {

static const char *const TAG = "bthome";

void BTHome::setup() {
  this->pref_ = global_preferences->make_preference<uint8_t>(this->pref_hash_);
  if (!this->pref_.load(&this->packet_id_)) {
    this->packet_id_ = 0;
  }
}

void BTHome::dump_config() {
  ESP_LOGCONFIG(TAG, "BTHome button");
  ESP_LOGCONFIG(TAG, "  Burst: %u ms", this->burst_duration_ms_);
  if (!this->manufacturer_data_.empty()) {
    ESP_LOGCONFIG(TAG, "  Manufacturer data: %u bytes", this->manufacturer_data_.size());
  }
}

float BTHome::get_setup_priority() const { return setup_priority::AFTER_BLUETOOTH; }

void BTHome::transmit(uint8_t event, uint8_t index) {
#ifndef USE_ESP32
  ESP_LOGE(TAG, "Sending BTHome advertisements requires ESP32");
  return;
#else
  this->packet_id_++;
  this->pref_.save(&this->packet_id_);

  uint8_t payload[1 + 2 + codec::MAX_BUTTONS * 2];
  size_t payload_len = 0;
  if (!codec::encode_button(this->packet_id_, event, index, payload, sizeof(payload), &payload_len)) {
    ESP_LOGE(TAG, "Cannot encode button index %u", index);
    return;
  }

  // Service data begins with the 16-bit UUID, little-endian, then the BTHome payload.
  std::vector<uint8_t> service;
  service.reserve(2 + payload_len);
  service.push_back(0xD2);
  service.push_back(0xFC);
  service.insert(service.end(), payload, payload + payload_len);

  esp32_ble::global_ble->advertising_set_service_data_and_name(std::span<const uint8_t>(service.data(), service.size()),
                                                                false);
  if (!this->manufacturer_data_.empty()) {
    esp32_ble::global_ble->advertising_set_manufacturer_data(this->manufacturer_data_);
  }
  if (!this->bursting_) {
    esp32_ble::global_ble->advertising_start();
    this->bursting_ = true;
  }
  this->set_timeout("bthome_burst", this->burst_duration_ms_, [this]() { this->stop_burst_(); });
  ESP_LOGD(TAG, "Button event 0x%02X index %u packet %u", event, index, this->packet_id_);
#endif
}

void BTHome::stop_burst_() {
#ifdef USE_ESP32
  const std::vector<uint8_t> empty;
  esp32_ble::global_ble->advertising_set_service_data(empty);
  if (!this->manufacturer_data_.empty()) {
    esp32_ble::global_ble->advertising_set_manufacturer_data(empty);
  }
  if (this->bursting_) {
    esp32_ble::global_ble->advertising_stop();
    this->bursting_ = false;
  }
#endif
}

}  // namespace bthome
}  // namespace esphome
