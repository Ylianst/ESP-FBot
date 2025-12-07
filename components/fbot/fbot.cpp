#include "fbot.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

namespace esphome {
namespace fbot {

static const char *const TAG = "fbot";

void Fbot::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Fbot...");
  this->write_handle_ = 0;
  this->notify_handle_ = 0;
  this->connected_ = false;
  this->characteristics_discovered_ = false;
}

void Fbot::loop() {
  // Poll for data if connected
  if (this->connected_ && this->characteristics_discovered_) {
    uint32_t now = millis();
    if (now - this->last_poll_time_ >= this->polling_interval_) {
      this->send_read_request();
      this->last_poll_time_ = now;
    }
  }
}

void Fbot::dump_config() {
  ESP_LOGCONFIG(TAG, "Fbot Battery:");
  ESP_LOGCONFIG(TAG, "  Polling interval: %ums", this->polling_interval_);
  LOG_SENSOR("  ", "Battery Percent", this->battery_percent_sensor_);
  LOG_SENSOR("  ", "Input Power", this->input_power_sensor_);
  LOG_SENSOR("  ", "Output Power", this->output_power_sensor_);
  LOG_SENSOR("  ", "System Power", this->system_power_sensor_);
  LOG_SENSOR("  ", "Total Power", this->total_power_sensor_);
  LOG_SENSOR("  ", "Remaining kWh", this->remaining_kwh_sensor_);
  LOG_SENSOR("  ", "Remaining Time", this->remaining_time_sensor_);
  LOG_BINARY_SENSOR("  ", "Connected", this->connected_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "USB Active", this->usb_active_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "DC Active", this->dc_active_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "AC Active", this->ac_active_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Light Active", this->light_active_binary_sensor_);
}

void Fbot::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                   esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status == ESP_GATT_OK) {
        ESP_LOGI(TAG, "Connected to Fbot");
        this->connected_ = true;
        this->update_connected_state(true);
      } else {
        ESP_LOGW(TAG, "Connection failed, status=%d", param->open.status);
        this->update_connected_state(false);
      }
      break;
    }
    
    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGW(TAG, "Disconnected from Fbot");
      this->connected_ = false;
      this->characteristics_discovered_ = false;
      this->update_connected_state(false);
      break;
    }
    
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      ESP_LOGD(TAG, "Service search complete");
      
      // Get write characteristic
      auto *write_chr = this->parent()->get_characteristic(
          esp32_ble_tracker::ESPBTUUID::from_raw(SERVICE_UUID),
          esp32_ble_tracker::ESPBTUUID::from_raw(WRITE_CHAR_UUID));
      if (write_chr == nullptr) {
        ESP_LOGW(TAG, "Write characteristic not found");
        break;
      }
      this->write_handle_ = write_chr->handle;
      ESP_LOGD(TAG, "Write characteristic handle: 0x%04x", this->write_handle_);
      
      // Get notify characteristic
      auto *notify_chr = this->parent()->get_characteristic(
          esp32_ble_tracker::ESPBTUUID::from_raw(SERVICE_UUID),
          esp32_ble_tracker::ESPBTUUID::from_raw(NOTIFY_CHAR_UUID));
      if (notify_chr == nullptr) {
        ESP_LOGW(TAG, "Notify characteristic not found");
        break;
      }
      this->notify_handle_ = notify_chr->handle;
      ESP_LOGD(TAG, "Notify characteristic handle: 0x%04x", this->notify_handle_);
      
      // Register for notifications
      auto status = esp_ble_gattc_register_for_notify(gattc_if, this->parent()->get_remote_bda(),
                                                       this->notify_handle_);
      if (status) {
        ESP_LOGW(TAG, "esp_ble_gattc_register_for_notify failed, status=%d", status);
      } else {
        ESP_LOGD(TAG, "Registered for notifications");
      }
      
      this->characteristics_discovered_ = true;
      this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
      break;
    }
    
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      if (param->reg_for_notify.status == ESP_GATT_OK) {
        ESP_LOGD(TAG, "Notification registration successful");
        // Start polling
        this->last_poll_time_ = millis();
        this->send_read_request();
      }
      break;
    }
    
    case ESP_GATTC_NOTIFY_EVT: {
      if (param->notify.handle == this->notify_handle_) {
        ESP_LOGVV(TAG, "Received notification, length=%d", param->notify.value_len);
        this->parse_notification(param->notify.value, param->notify.value_len);
      }
      break;
    }
    
    default:
      break;
  }
}

uint16_t Fbot::calculate_checksum(const uint8_t *data, size_t len) {
  // CRC-16 Modbus variant (from WPA app)
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;  // 40961 decimal = 0xA001
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void Fbot::generate_command_bytes(uint8_t address, uint16_t reg, uint16_t value, uint8_t *output) {
  // Build payload: [address, 6, reg_high, reg_low, value_high, value_low]
  output[0] = address;
  output[1] = 0x06;  // Function code for write
  output[2] = (reg >> 8) & 0xFF;
  output[3] = reg & 0xFF;
  output[4] = (value >> 8) & 0xFF;
  output[5] = value & 0xFF;
  
  // Calculate and append checksum
  uint16_t crc = this->calculate_checksum(output, 6);
  output[6] = (crc >> 8) & 0xFF;
  output[7] = crc & 0xFF;
}

void Fbot::send_read_request() {
  if (!this->connected_ || !this->characteristics_discovered_) {
    return;
  }
  
  // Read 80 registers starting from 0: [0x11, 0x04, 0x00, 0x00, 0x00, 0x50]
  uint8_t payload[6] = {0x11, 0x04, 0x00, 0x00, 0x00, 0x50};
  uint16_t crc = this->calculate_checksum(payload, 6);
  uint8_t command[8];
  memcpy(command, payload, 6);
  command[6] = (crc >> 8) & 0xFF;
  command[7] = crc & 0xFF;
  
  auto status = esp_ble_gattc_write_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(),
                                         this->write_handle_, sizeof(command), command,
                                         ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
  if (status) {
    ESP_LOGW(TAG, "Error sending read request, status=%d", status);
  } else {
    ESP_LOGVV(TAG, "Sent read request");
  }
}

void Fbot::send_control_command(uint16_t reg, uint16_t value) {
  if (!this->connected_ || !this->characteristics_discovered_) {
    ESP_LOGW(TAG, "Cannot send command: not connected");
    return;
  }
  
  uint8_t command[8];
  this->generate_command_bytes(0x11, reg, value, command);
  
  auto status = esp_ble_gattc_write_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(),
                                         this->write_handle_, sizeof(command), command,
                                         ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
  if (status) {
    ESP_LOGW(TAG, "Error sending control command, status=%d", status);
  } else {
    ESP_LOGI(TAG, "Sent control command: reg=%d, value=%d", reg, value);
    // Request update after full polling interval (2000ms) to allow inverter to stabilize
    this->last_poll_time_ = millis();
  }
}

uint16_t Fbot::get_register(const uint8_t *data, uint16_t length, uint16_t reg_index) {
  // Registers start at byte offset 6, each register is 2 bytes (big-endian)
  uint16_t offset = 6 + (reg_index * 2);
  if (offset + 1 >= length) {
    return 0;
  }
  return (data[offset] << 8) | data[offset + 1];
}

void Fbot::parse_notification(const uint8_t *data, uint16_t length) {
  // Validate minimum length and header
  if (length < 6) {
    ESP_LOGW(TAG, "Notification too short: %d bytes", length);
    return;
  }
  
  if (data[0] != 0x11 || data[1] != 0x04) {
    ESP_LOGVV(TAG, "Skipping non-status notification");
    return;
  }
  
  // Parse key registers
  float battery_percent = this->get_register(data, length, 56) / 10.0f;
  uint16_t input_watts = this->get_register(data, length, 3);
  uint16_t output_watts = this->get_register(data, length, 39);
  uint16_t system_watts = this->get_register(data, length, 21);
  uint16_t total_watts = this->get_register(data, length, 20);
  uint16_t remaining_minutes = this->get_register(data, length, 59);
  uint16_t state_flags = this->get_register(data, length, 41);
  
  // Calculate remaining kWh (battery capacity is 2.048kWh)
  float remaining_kwh = (battery_percent / 100.0f) * 2.048f;
  
  // Publish sensor values
  if (this->battery_percent_sensor_ != nullptr) {
    this->battery_percent_sensor_->publish_state(battery_percent);
  }
  if (this->input_power_sensor_ != nullptr) {
    this->input_power_sensor_->publish_state(input_watts);
  }
  if (this->output_power_sensor_ != nullptr) {
    this->output_power_sensor_->publish_state(output_watts);
  }
  if (this->system_power_sensor_ != nullptr) {
    this->system_power_sensor_->publish_state(system_watts);
  }
  if (this->total_power_sensor_ != nullptr) {
    this->total_power_sensor_->publish_state(total_watts);
  }
  if (this->remaining_kwh_sensor_ != nullptr) {
    this->remaining_kwh_sensor_->publish_state(remaining_kwh);
  }
  if (this->remaining_time_sensor_ != nullptr) {
    this->remaining_time_sensor_->publish_state(remaining_minutes);
  }
  
  // Update binary sensors for output states
  bool usb_state = (state_flags & STATE_USB_BIT) != 0;
  bool dc_state = (state_flags & STATE_DC_BIT) != 0;
  bool ac_state = (state_flags & STATE_AC_BIT) != 0;
  bool light_state = (state_flags & STATE_LIGHT_BIT) != 0;
  
  if (this->usb_active_binary_sensor_ != nullptr) {
    this->usb_active_binary_sensor_->publish_state(usb_state);
  }
  if (this->dc_active_binary_sensor_ != nullptr) {
    this->dc_active_binary_sensor_->publish_state(dc_state);
  }
  if (this->ac_active_binary_sensor_ != nullptr) {
    this->ac_active_binary_sensor_->publish_state(ac_state);
  }
  if (this->light_active_binary_sensor_ != nullptr) {
    this->light_active_binary_sensor_->publish_state(light_state);
  }
  
  // Sync switch states with device state
  if (this->usb_switch_ != nullptr) {
    this->usb_switch_->publish_state(usb_state);
  }
  if (this->dc_switch_ != nullptr) {
    this->dc_switch_->publish_state(dc_state);
  }
  if (this->ac_switch_ != nullptr) {
    this->ac_switch_->publish_state(ac_state);
  }
  if (this->light_switch_ != nullptr) {
    this->light_switch_->publish_state(light_state);
  }
  
  ESP_LOGD(TAG, "Battery: %.1f%%, Input: %dW, Output: %dW, USB: %d, DC: %d, AC: %d", 
           battery_percent, input_watts, output_watts, usb_state, dc_state, ac_state);
}

void Fbot::update_connected_state(bool state) {
  this->connected_ = state;
  if (this->connected_binary_sensor_ != nullptr) {
    this->connected_binary_sensor_->publish_state(state);
  }
}

// Control methods
void Fbot::control_usb(bool state) {
  this->send_control_command(REG_USB_CONTROL, state ? 1 : 0);
}

void Fbot::control_dc(bool state) {
  this->send_control_command(REG_DC_CONTROL, state ? 1 : 0);
}

void Fbot::control_ac(bool state) {
  this->send_control_command(REG_AC_CONTROL, state ? 1 : 0);
}

void Fbot::control_light(bool state) {
  this->send_control_command(REG_LIGHT_CONTROL, state ? 1 : 0);
}

}  // namespace fbot
}  // namespace esphome

#endif  // USE_ESP32
