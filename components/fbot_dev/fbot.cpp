#include "fbot.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

namespace esphome {
namespace fbot_dev {

static const char *const TAG = "fbot_dev";

void Fbot::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Fbot...");
  this->write_handle_ = 0;
  this->notify_handle_ = 0;
  this->connected_ = false;
  this->characteristics_discovered_ = false;
  this->settings_received_ = false;
  this->consecutive_poll_failures_ = 0;
  this->last_successful_poll_ = 0;
  this->last_settings_request_time_ = 0;
  
  // Initialize connected sensor to disconnected state
  if (this->connected_binary_sensor_ != nullptr) {
    this->connected_binary_sensor_->publish_state(false);
  }
}

void Fbot::loop() {
  /*
  // Poll for data if connected
  if (this->connected_ && this->characteristics_discovered_) {
    uint32_t now = millis();
    
    // Check for poll timeout
    this->check_poll_timeout();
    
    // Send regular status request
    if (now - this->last_poll_time_ >= this->polling_interval_) {
      this->send_read_request();
      this->last_poll_time_ = now;
    }
    
    // Send settings request periodically
    if (now - this->last_settings_request_time_ >= this->settings_polling_interval_) {
      this->send_settings_request();
      this->last_settings_request_time_ = now;
    }
  }
  */
}

void Fbot::dump_config() {
  ESP_LOGCONFIG(TAG, "Fbot Battery:");
  ESP_LOGCONFIG(TAG, "  Polling interval: %ums", this->polling_interval_);
  ESP_LOGCONFIG(TAG, "  Settings polling interval: %ums", this->settings_polling_interval_);
  LOG_SENSOR("  ", "Battery Percent", this->battery_percent_sensor_);
  LOG_SENSOR("  ", "Battery S1 Percent", this->battery_percent_s1_sensor_);
  LOG_SENSOR("  ", "Battery S2 Percent", this->battery_percent_s2_sensor_);
  LOG_SENSOR("  ", "AC Input Power", this->ac_input_power_sensor_);
  LOG_SENSOR("  ", "DC Input Power", this->dc_input_power_sensor_);
  LOG_SENSOR("  ", "Input Power", this->input_power_sensor_);
  LOG_SENSOR("  ", "Output Power", this->output_power_sensor_);
  LOG_SENSOR("  ", "System Power", this->system_power_sensor_);
  LOG_SENSOR("  ", "Total Power", this->total_power_sensor_);
  LOG_SENSOR("  ", "Remaining Time", this->remaining_time_sensor_);
  LOG_SENSOR("  ", "Charge Threshold", this->threshold_charge_sensor_);
  LOG_SENSOR("  ", "Discharge Threshold", this->threshold_discharge_sensor_);
  LOG_SENSOR("  ", "Charge Level", this->charge_level_sensor_);
  LOG_SENSOR("  ", "AC Out Voltage", this->ac_out_voltage_sensor_);
  LOG_SENSOR("  ", "AC Out Frequency", this->ac_out_frequency_sensor_);
  LOG_SENSOR("  ", "AC In Frequency", this->ac_in_frequency_sensor_);
  LOG_SENSOR("  ", "Time to Full", this->time_to_full_sensor_);
  LOG_SENSOR("  ", "USB-A1 Power", this->usb_a1_power_sensor_);
  LOG_SENSOR("  ", "USB-A2 Power", this->usb_a2_power_sensor_);
  LOG_SENSOR("  ", "USB-C1 Power", this->usb_c1_power_sensor_);
  LOG_SENSOR("  ", "USB-C2 Power", this->usb_c2_power_sensor_);
  LOG_SENSOR("  ", "USB-C3 Power", this->usb_c3_power_sensor_);
  LOG_SENSOR("  ", "USB-C4 Power", this->usb_c4_power_sensor_);
  LOG_BINARY_SENSOR("  ", "Connected", this->connected_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Battery S1 Connected", this->battery_connected_s1_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Battery S2 Connected", this->battery_connected_s2_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "USB Active", this->usb_active_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "DC Active", this->dc_active_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "AC Active", this->ac_active_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "Light Active", this->light_active_binary_sensor_);
}

void Fbot::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                   esp_ble_gattc_cb_param_t *param) {
  control_key_sound(false);  // Disable key sound on connect
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status == ESP_GATT_OK) {
        control_key_sound(false);  // Disable key sound on connect
        ESP_LOGI(TAG, "Connected to Fbot - X1");
        this->connected_ = true;
        this->consecutive_poll_failures_ = 0;
        this->last_successful_poll_ = 0;
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
    
    /*
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      if (param->reg_for_notify.status == ESP_GATT_OK) {
        ESP_LOGD(TAG, "Notification registration successful");
        // Start polling - request both status and settings on initial connection
        uint32_t now = millis();
        this->last_poll_time_ = now;
        this->last_settings_request_time_ = now;
        this->send_read_request();
        // Request settings shortly after first status request
        this->set_timeout(500, [this]() { this->send_settings_request(); });
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
    */

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
  /*
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
  */
}

void Fbot::send_settings_request() {
  /*
  if (!this->connected_ || !this->characteristics_discovered_) {
    return;
  }
  
  // Read 80 holding registers starting from 0: [0x11, 0x03, 0x00, 0x00, 0x00, 0x50]
  // Function code 0x03 = Read Holding Registers (settings)
  uint8_t payload[6] = {0x11, 0x03, 0x00, 0x00, 0x00, 0x50};
  uint16_t crc = this->calculate_checksum(payload, 6);
  uint8_t command[8];
  memcpy(command, payload, 6);
  command[6] = (crc >> 8) & 0xFF;
  command[7] = crc & 0xFF;
  
  auto status = esp_ble_gattc_write_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(),
                                         this->write_handle_, sizeof(command), command,
                                         ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
  if (status) {
    ESP_LOGW(TAG, "Error sending settings request, status=%d", status);
  } else {
    ESP_LOGD(TAG, "Sent settings request");
  }
  */
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
  // Validate minimum length
  if (length < 6) {
    ESP_LOGW(TAG, "Notification too short: %d bytes", length);
    return;
  }
  
  // ANY valid notification means device is responding - reset failure counter
  this->consecutive_poll_failures_ = 0;
  this->last_successful_poll_ = millis();
  
  // Check header byte to determine notification type
  if (data[0] != 0x11) {
    ESP_LOGVV(TAG, "Invalid header byte: 0x%02x", data[0]);
    return;
  }
  
  // Route based on function code
  if (data[1] == 0x03) {
    // 0x03 = Read Holding Registers response (settings)
    this->parse_settings_notification(data, length);
    return;
  } else if (data[1] == 0x04) {
    // 0x04 = Read Input Registers response (status)
    // Continue with status parsing below
  } else {
    ESP_LOGVV(TAG, "Unknown function code: 0x%02x", data[1]);
    return;
  }
  
  // Parse key registers
  float battery_percent = this->get_register(data, length, 56) / 10.0f;
  // Extra batteries (S1 / S2) ranges are 1 to 101, 0 means disconnected. Adding -1 to get proper range.
  float battery_percent_s1 = this->get_register(data, length, 53) / 10.0f - 1.0f;
  float battery_percent_s2 = this->get_register(data, length, 55) / 10.0f - 1.0f;
  
  // Charge level: register 2, values 1-5 map to 300W-1100W (increment by 200W)
  // 1 = 300, 2 = 500, 3 = 700, 4 = 900, 5 = 1100W
  uint16_t charge_level_raw = this->get_register(data, length, 2);
  uint16_t charge_level_watts = 0;
  if (charge_level_raw >= 1 && charge_level_raw <= 5) {
    charge_level_watts = 300 + ((charge_level_raw - 1) * 200);
  }
  
  // Determine if extra batteries are connected (raw value of 0 means disconnected)
  bool battery_s1_connected = this->get_register(data, length, 53) > 0;
  bool battery_s2_connected = this->get_register(data, length, 55) > 0;
  
  // Set to NAN if battery percentages are outside valid range (0-100)
  if (battery_percent_s1 < 0.0f || battery_percent_s1 > 100.0f) {
    battery_percent_s1 = NAN;
  }
  if (battery_percent_s2 < 0.0f || battery_percent_s2 > 100.0f) {
    battery_percent_s2 = NAN;
  }
  uint16_t ac_input_watts = this->get_register(data, length, 3);
  uint16_t dc_input_watts = this->get_register(data, length, 4);
  uint16_t input_watts = this->get_register(data, length, 6);
  uint16_t total_watts = this->get_register(data, length, 20);
  uint16_t system_watts = this->get_register(data, length, 21);
  uint16_t output_watts = this->get_register(data, length, 39);
  uint16_t state_flags = this->get_register(data, length, 41);
  float ac_out_voltage = this->get_register(data, length, 18) * 0.1f;
  float ac_out_frequency = this->get_register(data, length, 19) * 0.1f;
  float ac_in_frequency = this->get_register(data, length, 22) * 0.01f;
  uint16_t time_to_full = this->get_register(data, length, 58);
  uint16_t remaining_minutes = this->get_register(data, length, 59);
  
  // USB port power outputs (multiply by 0.1 to convert to watts)
  float usb_a1_power = this->get_register(data, length, REG_USB_A1_OUT) * 0.1f;
  float usb_a2_power = this->get_register(data, length, REG_USB_A2_OUT) * 0.1f;
  float usb_c1_power = this->get_register(data, length, REG_USB_C1_OUT) * 0.1f;
  float usb_c2_power = this->get_register(data, length, REG_USB_C2_OUT) * 0.1f;
  float usb_c3_power = this->get_register(data, length, REG_USB_C3_OUT) * 0.1f;
  float usb_c4_power = this->get_register(data, length, REG_USB_C4_OUT) * 0.1f;
  
  // Publish sensor values
  if (this->battery_percent_sensor_ != nullptr) {
    this->battery_percent_sensor_->publish_state(battery_percent);
  }
  if (this->battery_percent_s1_sensor_ != nullptr) {
    this->battery_percent_s1_sensor_->publish_state(battery_percent_s1);
  }
  if (this->battery_percent_s2_sensor_ != nullptr) {
    this->battery_percent_s2_sensor_->publish_state(battery_percent_s2);
  }  
  if (this->ac_input_power_sensor_ != nullptr) {
    this->ac_input_power_sensor_->publish_state(ac_input_watts);
  }
  if (this->dc_input_power_sensor_ != nullptr) {
    this->dc_input_power_sensor_->publish_state(dc_input_watts);
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
  if (this->remaining_time_sensor_ != nullptr) {
    this->remaining_time_sensor_->publish_state(remaining_minutes);
  }
  if (this->charge_level_sensor_ != nullptr) {
    this->charge_level_sensor_->publish_state(charge_level_watts);
  }
  if (this->ac_out_voltage_sensor_ != nullptr) {
    this->ac_out_voltage_sensor_->publish_state(ac_out_voltage);
  }
  if (this->ac_out_frequency_sensor_ != nullptr) {
    this->ac_out_frequency_sensor_->publish_state(ac_out_frequency);
  }
  if (this->ac_in_frequency_sensor_ != nullptr) {
    this->ac_in_frequency_sensor_->publish_state(ac_in_frequency);
  }
  if (this->time_to_full_sensor_ != nullptr) {
    this->time_to_full_sensor_->publish_state(time_to_full);
  }
  if (this->usb_a1_power_sensor_ != nullptr) {
    this->usb_a1_power_sensor_->publish_state(usb_a1_power);
  }
  if (this->usb_a2_power_sensor_ != nullptr) {
    this->usb_a2_power_sensor_->publish_state(usb_a2_power);
  }
  if (this->usb_c1_power_sensor_ != nullptr) {
    this->usb_c1_power_sensor_->publish_state(usb_c1_power);
  }
  if (this->usb_c2_power_sensor_ != nullptr) {
    this->usb_c2_power_sensor_->publish_state(usb_c2_power);
  }
  if (this->usb_c3_power_sensor_ != nullptr) {
    this->usb_c3_power_sensor_->publish_state(usb_c3_power);
  }
  if (this->usb_c4_power_sensor_ != nullptr) {
    this->usb_c4_power_sensor_->publish_state(usb_c4_power);
  }

  // Update binary sensors for battery connection status
  if (this->battery_connected_s1_binary_sensor_ != nullptr) {
    this->battery_connected_s1_binary_sensor_->publish_state(battery_s1_connected);
  }
  if (this->battery_connected_s2_binary_sensor_ != nullptr) {
    this->battery_connected_s2_binary_sensor_->publish_state(battery_s2_connected);
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
  
  ESP_LOGD(TAG, "Battery: %.1f%% S1:%.1f%%(con:%d) S2:%.1f%%(con:%d), Input: %dW, Output: %dW, USB: %d, DC: %d, AC: %d", 
           battery_percent, battery_percent_s1, battery_s1_connected, battery_percent_s2, battery_s2_connected, 
           input_watts, output_watts, usb_state, dc_state, ac_state);
}

void Fbot::parse_settings_notification(const uint8_t *data, uint16_t length) {
  // Parse holding registers (settings) from 0x1103 response
  ESP_LOGD(TAG, "Received settings notification");
  
  // Mark that we've received settings at least once
  if (!this->settings_received_) {
    this->settings_received_ = true;
    ESP_LOGI(TAG, "First settings response received");
  }
  
  // Parse AC Silent state (register 57: 0=off, 1=on)
  bool ac_silent_state = this->get_register(data, length, REG_AC_SILENT_CONTROL) == 1;
  
  // Sync AC Silent switch state with device
  if (this->ac_silent_switch_ != nullptr) {
    this->ac_silent_switch_->publish_state(ac_silent_state);
  }
  
  // Parse Key Sound state (register 56: 0=off, 1=on)
  bool key_sound_state = this->get_register(data, length, REG_KEY_SOUND) == 1;
  
  // Sync Key Sound switch state with device
  if (this->key_sound_switch_ != nullptr) {
    this->key_sound_switch_->publish_state(key_sound_state);
  }
  
  // Parse Light Mode state (register 27: 0=Off, 1=On, 2=SOS, 3=Flashing)
  uint16_t light_mode_value = this->get_register(data, length, REG_LIGHT_CONTROL);
  
#ifdef USE_SELECT
  // Sync Light Mode select state with device
  if (this->light_mode_select_ != nullptr) {
    std::string light_mode;
    switch (light_mode_value) {
      case 0: light_mode = "Off"; break;
      case 1: light_mode = "On"; break;
      case 2: light_mode = "SOS"; break;
      case 3: light_mode = "Flashing"; break;
      default: light_mode = "Off"; break;
    }
    this->light_mode_select_->publish_state(light_mode);
  }
#endif
  
  // Parse threshold registers (66 and 67 from holding registers)
  // Values are in permille (divide by 10 for percentage)
  float threshold_discharge = this->get_register(data, length, REG_THRESHOLD_DISCHARGE) / 10.0f;
  float threshold_charge = this->get_register(data, length, REG_THRESHOLD_CHARGE) / 10.0f;
  
  // Publish threshold sensor values (read-only display)
  if (this->threshold_discharge_sensor_ != nullptr) {
    this->threshold_discharge_sensor_->publish_state(threshold_discharge);
  }
  if (this->threshold_charge_sensor_ != nullptr) {
    this->threshold_charge_sensor_->publish_state(threshold_charge);
  }
  
#ifdef USE_NUMBER
  // Publish threshold number values (user-adjustable controls)
  if (this->threshold_discharge_number_ != nullptr) {
    this->threshold_discharge_number_->publish_state(threshold_discharge);
  }
  if (this->threshold_charge_number_ != nullptr) {
    this->threshold_charge_number_->publish_state(threshold_charge);
  }
#endif
  
  ESP_LOGD(TAG, "Settings: Discharge threshold: %.1f%%, Charge threshold: %.1f%%, AC Silent: %d, Key Sound: %d", 
           threshold_discharge, threshold_charge, ac_silent_state, key_sound_state);
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
  // Simple on/off control - uses value 1 for on, 0 for off
  this->send_control_command(REG_LIGHT_CONTROL, state ? 1 : 0);
}

void Fbot::control_light_mode(const std::string &value) {
  // Map the mode string to register value
  uint16_t mode_value = 0;
  if (value == "Off") {
    mode_value = 0;
  } else if (value == "On") {
    mode_value = 1;
  } else if (value == "SOS") {
    mode_value = 2;
  } else if (value == "Flashing") {
    mode_value = 3;
  } else {
    ESP_LOGW(TAG, "Unknown light mode: %s", value.c_str());
    return;
  }
  
  ESP_LOGI(TAG, "Setting light mode to: %s (value: %d)", value.c_str(), mode_value);
  this->send_control_command(REG_LIGHT_CONTROL, mode_value);
  
  // Update the light switch state based on the mode (off=false, any other mode=true)
  if (this->light_switch_ != nullptr) {
    this->light_switch_->publish_state(mode_value != 0);
  }
}

void Fbot::control_ac_silent(bool state) {
  // AC Silent uses holding register 57 (settings register)
  // Function code 0x06 (Write Single Register) writes to holding registers
  this->send_control_command(REG_AC_SILENT_CONTROL, state ? 1 : 0);
  
  // Request settings update to confirm the change
  this->set_timeout(500, [this]() { 
    this->send_settings_request(); 
  });
}

void Fbot::control_key_sound(bool state) {
  // Key Sound uses holding register 56 (settings register)
  // Function code 0x06 (Write Single Register) writes to holding registers
  this->send_control_command(REG_KEY_SOUND, state ? 1 : 0);
  
  // Request settings update to confirm the change
  this->set_timeout(500, [this]() { 
    this->send_settings_request(); 
  });
}

void Fbot::set_threshold_charge(float percent) {
  // Convert percentage to permille (multiply by 10)
  uint16_t value = static_cast<uint16_t>(percent * 10);
  if (value < 100) { value = 100; }  // Minimum 100
  if (value > 1000) { value = 1000; }  // Maximum 1000
  this->send_control_command(REG_THRESHOLD_CHARGE, value);
}

void Fbot::set_threshold_discharge(float percent) {
  // Convert percentage to permille (multiply by 10)
  uint16_t value = static_cast<uint16_t>(percent * 10);
  if (value < 0) { value = 0; }  // Minimum 0
  if (value > 500) { value = 500; }  // Maximum 500
  this->send_control_command(REG_THRESHOLD_DISCHARGE, value);
}

void Fbot::set_wifi_credentials(const std::string &ssid, const std::string &password) {
  if (!this->connected_ || !this->characteristics_discovered_) {
    ESP_LOGW(TAG, "Cannot set WiFi credentials: not connected");
    return;
  }
  
  // Validate SSID and password lengths
  if (ssid.length() == 0 || ssid.length() > 255) {
    ESP_LOGW(TAG, "Invalid SSID length: %d (must be 1-255)", ssid.length());
    return;
  }
  if (password.length() == 0 || password.length() > 255) {
    ESP_LOGW(TAG, "Invalid password length: %d (must be 1-255)", password.length());
    return;
  }
  
  // Build WiFi configuration packet
  // Format: [Header, OpCode, SSID_Len, Pass_Len, SSID..., Password..., CRC]
  uint8_t ssid_len = static_cast<uint8_t>(ssid.length());
  uint8_t pass_len = static_cast<uint8_t>(password.length());
  
  // Calculate total packet size: header(1) + opcode(1) + ssid_len(1) + pass_len(1) + ssid + password + crc(2)
  size_t packet_size = 4 + ssid_len + pass_len + 2;
  uint8_t *packet = new uint8_t[packet_size];
  
  // Build packet
  packet[0] = 0x11;  // Header
  packet[1] = 0x07;  // OpCode for WiFi configuration
  packet[2] = ssid_len;
  packet[3] = pass_len;
  
  // Copy SSID
  memcpy(&packet[4], ssid.c_str(), ssid_len);
  
  // Copy password
  memcpy(&packet[4 + ssid_len], password.c_str(), pass_len);
  
  // Calculate and append CRC
  uint16_t crc = this->calculate_checksum(packet, packet_size - 2);
  packet[packet_size - 2] = (crc >> 8) & 0xFF;
  packet[packet_size - 1] = crc & 0xFF;
  
  // Send the WiFi configuration command
  auto status = esp_ble_gattc_write_char(this->parent()->get_gattc_if(), this->parent()->get_conn_id(),
                                         this->write_handle_, packet_size, packet,
                                         ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
  
  if (status) {
    ESP_LOGW(TAG, "Error sending WiFi credentials, status=%d", status);
  } else {
    ESP_LOGI(TAG, "Sent WiFi credentials: SSID='%s' (len=%d), Password length=%d", 
             ssid.c_str(), ssid_len, pass_len);
    ESP_LOGI(TAG, "Device will connect to WiFi and disable AP mode");
  }
  
  // Clean up
  delete[] packet;
}

void Fbot::check_poll_timeout() {
  /*
  // Only check timeout if we've started polling
  if (this->last_poll_time_ == 0) {
    return;
  }
  
  // Don't check if we've already hit max failures
  if (this->consecutive_poll_failures_ >= MAX_POLL_FAILURES) {
    return;
  }
  
  uint32_t now = millis();
  uint32_t time_since_success = (this->last_successful_poll_ > 0) ? (now - this->last_successful_poll_) : 0;
  
  // Check if we've exceeded the timeout period since last successful poll
  if (this->last_successful_poll_ > 0 && time_since_success > POLL_TIMEOUT_MS) {
    // Increment failure counter only once per timeout period
    uint32_t time_since_poll = now - this->last_poll_time_;
    
    // Only count as a new failure if it's been at least polling_interval since last poll attempt
    if (time_since_poll >= this->polling_interval_) {
      this->consecutive_poll_failures_++;
      
      ESP_LOGW(TAG, "Poll timeout detected (failure %d/%d)", this->consecutive_poll_failures_, MAX_POLL_FAILURES);
      
      // Update last_successful_poll to now plus timeout, so we wait another full timeout period
      // before incrementing again
      this->last_successful_poll_ = now;
      
      // Check if we've reached the maximum failures
      if (this->consecutive_poll_failures_ >= MAX_POLL_FAILURES) {
        ESP_LOGE(TAG, "Max poll failures reached - marking as disconnected and resetting sensors");
        this->reset_sensors_to_unknown();
        this->update_connected_state(false);
      }
    }
  }
  */
}

void Fbot::reset_sensors_to_unknown() {
  // Reset all sensor values to unknown/unavailable
  if (this->battery_percent_sensor_ != nullptr) {
    this->battery_percent_sensor_->publish_state(NAN);
  }
  if (this->battery_percent_s1_sensor_ != nullptr) {
    this->battery_percent_s1_sensor_->publish_state(NAN);
  }
  if (this->battery_percent_s2_sensor_ != nullptr) {
    this->battery_percent_s2_sensor_->publish_state(NAN);
  }
  if (this->ac_input_power_sensor_ != nullptr) {
    this->ac_input_power_sensor_->publish_state(NAN);
  }
  if (this->dc_input_power_sensor_ != nullptr) {
    this->dc_input_power_sensor_->publish_state(NAN);
  }
  if (this->input_power_sensor_ != nullptr) {
    this->input_power_sensor_->publish_state(NAN);
  }
  if (this->output_power_sensor_ != nullptr) {
    this->output_power_sensor_->publish_state(NAN);
  }
  if (this->system_power_sensor_ != nullptr) {
    this->system_power_sensor_->publish_state(NAN);
  }
  if (this->total_power_sensor_ != nullptr) {
    this->total_power_sensor_->publish_state(NAN);
  }
  if (this->remaining_time_sensor_ != nullptr) {
    this->remaining_time_sensor_->publish_state(NAN);
  }
  if (this->charge_level_sensor_ != nullptr) {
    this->charge_level_sensor_->publish_state(NAN);
  }
  if (this->ac_out_voltage_sensor_ != nullptr) {
    this->ac_out_voltage_sensor_->publish_state(NAN);
  }
  if (this->ac_out_frequency_sensor_ != nullptr) {
    this->ac_out_frequency_sensor_->publish_state(NAN);
  }
  if (this->ac_in_frequency_sensor_ != nullptr) {
    this->ac_in_frequency_sensor_->publish_state(NAN);
  }
  if (this->time_to_full_sensor_ != nullptr) {
    this->time_to_full_sensor_->publish_state(NAN);
  }
  if (this->usb_a1_power_sensor_ != nullptr) {
    this->usb_a1_power_sensor_->publish_state(NAN);
  }
  if (this->usb_a2_power_sensor_ != nullptr) {
    this->usb_a2_power_sensor_->publish_state(NAN);
  }
  if (this->usb_c1_power_sensor_ != nullptr) {
    this->usb_c1_power_sensor_->publish_state(NAN);
  }
  if (this->usb_c2_power_sensor_ != nullptr) {
    this->usb_c2_power_sensor_->publish_state(NAN);
  }
  if (this->usb_c3_power_sensor_ != nullptr) {
    this->usb_c3_power_sensor_->publish_state(NAN);
  }
  if (this->usb_c4_power_sensor_ != nullptr) {
    this->usb_c4_power_sensor_->publish_state(NAN);
  }
  
  // Reset binary sensors for output states to unknown
  if (this->usb_active_binary_sensor_ != nullptr) {
    this->usb_active_binary_sensor_->publish_state(false);
  }
  if (this->dc_active_binary_sensor_ != nullptr) {
    this->dc_active_binary_sensor_->publish_state(false);
  }
  if (this->ac_active_binary_sensor_ != nullptr) {
    this->ac_active_binary_sensor_->publish_state(false);
  }
  if (this->light_active_binary_sensor_ != nullptr) {
    this->light_active_binary_sensor_->publish_state(false);
  }
}

}  // namespace fbot_dev
}  // namespace esphome

#endif  // USE_ESP32
