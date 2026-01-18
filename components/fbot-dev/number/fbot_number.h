#pragma once

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "../fbot.h"

#ifdef USE_ESP32

namespace esphome {
namespace fbot_dev {

class FbotNumber : public number::Number, public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_parent(Fbot *parent) { this->parent_ = parent; }
  void set_number_type(const std::string &type) { this->number_type_ = type; }

 protected:
  void control(float value) override;

  Fbot *parent_{nullptr};
  std::string number_type_;
};

}  // namespace fbot_dev
}  // namespace esphome

#endif  // USE_ESP32
