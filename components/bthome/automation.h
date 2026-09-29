#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#include "bthome.h"

namespace esphome {
namespace bthome {

template<typename... Ts> class TransmitAction : public Action<Ts...>, public Parented<BTHome> {
 public:
  void set_event(uint8_t event) { this->event_ = event; }
  void set_index(uint8_t index) { this->index_ = index; }

  void play(Ts... /*x*/) override { this->parent_->transmit(this->event_, this->index_); }

 protected:
  uint8_t event_{0x01};
  uint8_t index_{1};
};

}  // namespace bthome
}  // namespace esphome
