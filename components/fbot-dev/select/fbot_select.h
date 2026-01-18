#pragma once

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "../fbot.h"

#ifdef USE_ESP32

namespace esphome {
namespace fbot {

class FbotSelect : public select::Select, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_parent(Fbot *parent) { this->parent_ = parent; }
  void set_select_type(const std::string &type) { this->select_type_ = type; }
  
 protected:
  void control(const std::string &value) override;
  
  Fbot *parent_{nullptr};
  std::string select_type_;
};

}  // namespace fbot
}  // namespace esphome

#endif  // USE_ESP32
