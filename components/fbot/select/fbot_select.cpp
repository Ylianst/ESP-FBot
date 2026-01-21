#include "fbot_select.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace fbot {

static const char *const TAG = "fbot.select";

void FbotSelect::setup() {
  // Select starts with no option selected
}

void FbotSelect::dump_config() {
  LOG_SELECT("", "Fbot Select", this);
  ESP_LOGCONFIG(TAG, "  Type: %s", this->select_type_.c_str());
}

void FbotSelect::control(const std::string &value) {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "No parent set for select");
    return;
  }
  
  // Check if device is connected before allowing select changes
  if (!this->parent_->is_connected()) {
    ESP_LOGW(TAG, "Cannot change select '%s': device is disconnected", this->select_type_.c_str());
    return;
  }
  
  // Call the appropriate control method based on select type
  if (this->select_type_ == "light_mode") {
    this->parent_->control_light_mode(value);
  } else if (this->select_type_ == "ac_charge_limit") {
    this->parent_->control_ac_charge_limit(value);
  } else {
    ESP_LOGW(TAG, "Unknown select type: %s", this->select_type_.c_str());
    return;
  }
  
  // Publish the new state
  this->publish_state(value);
}

}  // namespace fbot
}  // namespace esphome

#endif  // USE_ESP32
