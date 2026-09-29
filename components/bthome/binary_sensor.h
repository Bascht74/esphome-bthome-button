#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/ble_device_base/ble_device.h"
#include "esphome/core/component.h"

namespace esphome {
namespace bthome {

class BTHomeButtonBinarySensor : public binary_sensor::BinarySensor,
                                 public Component,
                                 public ble_device_base::ESPBTDeviceListener {
 public:
  void set_address(uint64_t address) { this->address_ = address; }
  void set_index(uint8_t index) { this->index_ = index; }
  void set_event(uint8_t event) { this->event_ = event; }
  void set_pulse_length(uint32_t ms) { this->pulse_length_ms_ = ms; }

  void setup() override;
  void dump_config() override;
  bool parse_device(const ble_device_base::ESPBTDevice &device) override;

 protected:
  uint64_t address_{0};
  uint8_t index_{1};
  uint8_t event_{0x01};
  uint32_t pulse_length_ms_{200};
  bool has_packet_id_{false};
  uint8_t last_packet_id_{0};
  uint32_t last_emit_ms_{0};
  bool encrypted_logged_{false};
};

}  // namespace bthome
}  // namespace esphome
