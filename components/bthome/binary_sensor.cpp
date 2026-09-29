#include "binary_sensor.h"

#include "codec.h"

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace bthome {

static const char *const TAG = "bthome.button";
static constexpr uint32_t NO_PACKET_COOLDOWN_MS = 1200;

void BTHomeButtonBinarySensor::setup() { this->publish_initial_state(false); }

void BTHomeButtonBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "BTHome Button", this);
  ESP_LOGCONFIG(TAG,
                "  Index: %u\n"
                "  Event: 0x%02X",
                this->index_, this->event_);
}

bool BTHomeButtonBinarySensor::parse_device(const ble_device_base::ESPBTDevice &device) {
  if (device.address_uint64() != this->address_) {
    return false;
  }
  bool matched = false;
  for (const auto &service_data : device.get_service_datas()) {
    if (!service_data.uuid.contains(0xD2, 0xFC)) {
      continue;
    }
    matched = true;
    codec::Parsed parsed{};
    const uint8_t *bytes = service_data.data.data();
    const size_t size = service_data.data.size();
    if (!codec::parse(bytes, size, &parsed)) {
      if (parsed.encrypted && !this->encrypted_logged_) {
        this->encrypted_logged_ = true;
        ESP_LOGW(TAG, "Encrypted BTHome advertisement ignored");
      }
      continue;
    }
    if (parsed.button_count < this->index_) {
      continue;
    }
    uint8_t event = parsed.buttons[this->index_ - 1];
    if (event == codec::BUTTON_HOLD_ALIAS && this->event_ == codec::BUTTON_HOLD) {
      event = codec::BUTTON_HOLD;
    }
    if (event == 0x00 || event != this->event_) {
      continue;
    }
    const uint32_t now = millis();
    if (parsed.has_packet_id) {
      if (this->has_packet_id_ && parsed.packet_id == this->last_packet_id_) {
        continue;
      }
      this->has_packet_id_ = true;
      this->last_packet_id_ = parsed.packet_id;
    } else if (now - this->last_emit_ms_ < NO_PACKET_COOLDOWN_MS) {
      continue;
    }
    this->last_emit_ms_ = now;
    this->publish_state(true);
    this->set_timeout("bthome_pulse", this->pulse_length_ms_, [this]() { this->publish_state(false); });
  }
  return matched;
}

}  // namespace bthome
}  // namespace esphome
