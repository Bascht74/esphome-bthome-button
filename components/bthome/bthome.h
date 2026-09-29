#pragma once

#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

#include <vector>

namespace esphome {
namespace bthome {

class BTHome : public Component {
 public:
  void set_burst_duration(uint32_t ms) { this->burst_duration_ms_ = ms; }
  void set_pref_hash(uint32_t hash) { this->pref_hash_ = hash; }
  void set_manufacturer_data(const std::vector<uint8_t> &data) { this->manufacturer_data_ = data; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void transmit(uint8_t event, uint8_t index);

 protected:
  void stop_burst_();

  uint32_t burst_duration_ms_{1500};
  uint32_t pref_hash_{0};
  ESPPreferenceObject pref_{};
  uint8_t packet_id_{0};
  bool bursting_{false};
  std::vector<uint8_t> manufacturer_data_{};
};

}  // namespace bthome
}  // namespace esphome
