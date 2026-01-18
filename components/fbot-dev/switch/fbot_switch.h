#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "../fbot.h"

#ifdef USE_ESP32

namespace esphome {
namespace fbot {

class FbotSwitch : public switch_::Switch, public Component {
 public:
  void setup() override;
  void dump_config() override;
  
  void set_parent(Fbot *parent) { this->parent_ = parent; }
  void set_switch_type(const std::string &type) { this->switch_type_ = type; }
  
 protected:
  void write_state(bool state) override;
  
  Fbot *parent_{nullptr};
  std::string switch_type_;
};

}  // namespace fbot
}  // namespace esphome

#endif  // USE_ESP32
