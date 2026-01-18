#include "fbot_switch.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace fbot_dev {

static const char *const TAG = "fbot_dev.switch";

void FbotSwitch::setup() {
  // Switches start in unknown state
  this->state = false;
}

void FbotSwitch::dump_config() {
  LOG_SWITCH("", "Fbot Switch", this);
  ESP_LOGCONFIG(TAG, "  Type: %s", this->switch_type_.c_str());
}

void FbotSwitch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "No parent set for switch");
    return;
  }
  
  // Check if device is connected before allowing switch changes
  if (!this->parent_->is_connected()) {
    ESP_LOGW(TAG, "Cannot change switch '%s': device is disconnected", this->switch_type_.c_str());
    // Immediately publish the current state (false) to reject the change
    this->publish_state(false);
    return;
  }
  
  // Call the appropriate control method based on switch type
  if (this->switch_type_ == "usb") {
    this->parent_->control_usb(state);
  } else if (this->switch_type_ == "dc") {
    this->parent_->control_dc(state);
  } else if (this->switch_type_ == "ac") {
    this->parent_->control_ac(state);
  } else if (this->switch_type_ == "light") {
    this->parent_->control_light(state);
  } else if (this->switch_type_ == "ac_silent") {
    this->parent_->control_ac_silent(state);
  } else if (this->switch_type_ == "key_sound") {
    this->parent_->control_key_sound(state);
  } else {
    ESP_LOGW(TAG, "Unknown switch type: %s", this->switch_type_.c_str());
    return;
  }
  
  // Publish the new state optimistically
  // The actual state will be confirmed when the next status update arrives
  this->publish_state(state);
}

}  // namespace fbot_dev
}  // namespace esphome

#endif  // USE_ESP32
