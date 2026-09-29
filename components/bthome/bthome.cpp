#include "bthome.h"

#include "codec.h"

#include "esphome/core/log.h"

#include <cinttypes>
#include <cstddef>
#include <cstring>

#ifdef USE_ESP32
#include "esphome/components/esp32_ble/ble.h"

#include <span>
#include <vector>
#endif

namespace esphome {
namespace bthome {

static const char *const TAG = "bthome";
// Company id (2) plus at most 15 payload bytes. Flags and the BTHome AD already
// consume most of the 31-byte legacy advertisement.
static constexpr size_t MAX_MANUFACTURER_BYTES = 17;

void BTHome::set_manufacturer_data(const std::vector<uint8_t> &data) {
  // Reject instead of truncating, so a partial field never goes on air.
  if (data.size() < 2 || data.size() > MAX_MANUFACTURER_BYTES) {
    ESP_LOGE(TAG, "Manufacturer data must be 2..%u bytes, got %zu", MAX_MANUFACTURER_BYTES, data.size());
    this->manufacturer_len_ = 0;
    return;
  }
  this->manufacturer_len_ = static_cast<uint8_t>(data.size());
  std::memcpy(this->manufacturer_, data.data(), data.size());
}

void BTHome::setup() {
  this->pref_ = global_preferences->make_preference<uint8_t>(this->pref_hash_);
  if (!this->pref_.load(&this->packet_id_)) {
    this->packet_id_ = 0;
  }
}

void BTHome::dump_config() {
  ESP_LOGCONFIG(TAG,
                "BTHome button\n"
                "  Burst: %" PRIu32 " ms\n"
                "  Manufacturer data: %u bytes",
                this->burst_duration_ms_, this->manufacturer_len_);
}

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
  uint8_t service[2 + sizeof(payload)];
  service[0] = 0xD2;
  service[1] = 0xFC;
  std::memcpy(service + 2, payload, payload_len);
  const size_t service_len = 2 + payload_len;

  esp32_ble::global_ble->advertising_set_service_data_and_name(std::span<const uint8_t>(service, service_len), false);
  if (this->manufacturer_len_ != 0) {
    esp32_ble::global_ble->advertising_set_manufacturer_data(
        std::vector<uint8_t>(this->manufacturer_, this->manufacturer_ + this->manufacturer_len_));
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
  if (this->manufacturer_len_ != 0) {
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
