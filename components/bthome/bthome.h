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
  void set_manufacturer_data(const std::vector<uint8_t> &data);

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_BLUETOOTH; }

  void transmit(uint8_t event, uint8_t index);

 protected:
  void stop_burst_();

  ESPPreferenceObject pref_{};
  uint32_t burst_duration_ms_{1500};
  uint32_t pref_hash_{0};
  uint8_t packet_id_{0};
  uint8_t manufacturer_len_{0};
  bool bursting_{false};
  uint8_t manufacturer_[17]{};
};

}  // namespace bthome
}  // namespace esphome
