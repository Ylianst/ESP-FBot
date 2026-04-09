#include "fbot_rescue.h"
#include "esphome/core/log.h"

#ifdef USE_ESP32

namespace esphome {
namespace fbot_rescue {

static const char *const TAG = "fbot_rescue";

// Same UUIDs as the main fbot component
static const char *const SERVICE_UUID = "0000a002-0000-1000-8000-00805f9b34fb";
static const char *const WRITE_CHAR_UUID = "0000c304-0000-1000-8000-00805f9b34fb";

void FbotRescue::setup() {
  ESP_LOGI(TAG, "FBot Rescue mode active");
  ESP_LOGI(TAG, "  Target: register %d = 0x%04X", this->register_, this->value_);
  ESP_LOGI(TAG, "  Handle scan range: 0x%04X - 0x%04X", this->handle_min_, this->handle_max_);
  ESP_LOGI(TAG, "  Blast interval: %ums, duration: %ums", this->blast_interval_, this->blast_duration_);
}

void FbotRescue::loop() {
  if (!this->blast_active_ || !this->connected_) {
    return;
  }

  uint32_t now = millis();

  // Stop blasting after duration expires
  if (now - this->blast_start_time_ >= this->blast_duration_) {
    ESP_LOGI(TAG, "Blast duration complete. Sent %u total writes.", this->write_count_);
    this->blast_active_ = false;
    return;
  }

  // Send another blast round at the configured interval
  if (now - this->last_blast_time_ >= this->blast_interval_) {
    this->last_blast_time_ = now;
    if (this->discovered_handle_ != 0) {
      // We know the real handle - just write to it
      ESP_LOGD(TAG, "Writing to discovered handle 0x%04X", this->discovered_handle_);
      this->write_register(this->discovered_handle_);
    } else {
      this->blast_all_handles();
    }
  }
}

void FbotRescue::dump_config() {
  ESP_LOGCONFIG(TAG, "FBot Rescue:");
  ESP_LOGCONFIG(TAG, "  Register: %d", this->register_);
  ESP_LOGCONFIG(TAG, "  Value: 0x%04X", this->value_);
  ESP_LOGCONFIG(TAG, "  Handle range: 0x%04X - 0x%04X", this->handle_min_, this->handle_max_);
  ESP_LOGCONFIG(TAG, "  Blast interval: %ums", this->blast_interval_);
  ESP_LOGCONFIG(TAG, "  Blast duration: %ums", this->blast_duration_);
}

void FbotRescue::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                      esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "Connection failed, status=%d", param->open.status);
        this->connected_ = false;
        break;
      }

      ESP_LOGI(TAG, "Connected! Immediately blasting writes (no discovery)...");
      this->gattc_if_ = gattc_if;
      this->conn_id_ = param->open.conn_id;
      this->connected_ = true;
      this->write_count_ = 0;

      // Start blasting immediately - don't wait for service discovery
      this->blast_active_ = true;
      this->blast_start_time_ = millis();
      this->last_blast_time_ = 0;  // Force immediate first blast

      // Fire first blast right now
      this->blast_all_handles();
      break;
    }

    case ESP_GATTC_SEARCH_CMPL_EVT: {
      // If service discovery completes (unlikely in reboot-loop scenario),
      // use the real handle for remaining writes
      auto *chr = this->parent()->get_characteristic(
          esp32_ble_tracker::ESPBTUUID::from_raw(SERVICE_UUID),
          esp32_ble_tracker::ESPBTUUID::from_raw(WRITE_CHAR_UUID));
      if (chr != nullptr) {
        this->discovered_handle_ = chr->handle;
        ESP_LOGI(TAG, "Discovery completed! Real write handle: 0x%04X", chr->handle);
        // Send a few targeted writes immediately
        for (int i = 0; i < 3; i++) {
          this->write_register(chr->handle);
        }
      }
      this->node_state = esp32_ble_tracker::ClientState::ESTABLISHED;
      break;
    }

    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGW(TAG, "Disconnected (reason 0x%02X) - will reconnect on next advertisement",
               param->disconnect.reason);
      this->connected_ = false;
      this->blast_active_ = false;
      this->discovered_handle_ = 0;
      break;
    }

    default:
      break;
  }
}

uint16_t FbotRescue::calculate_checksum(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

void FbotRescue::write_register(uint16_t handle) {
  // Build Modbus write command: [addr, func, reg_h, reg_l, val_h, val_l, crc_h, crc_l]
  uint8_t payload[6] = {
    0x11, 0x06,
    (uint8_t)((this->register_ >> 8) & 0xFF),
    (uint8_t)(this->register_ & 0xFF),
    (uint8_t)((this->value_ >> 8) & 0xFF),
    (uint8_t)(this->value_ & 0xFF)
  };
  uint16_t crc = this->calculate_checksum(payload, 6);
  uint8_t cmd[8];
  memcpy(cmd, payload, 6);
  cmd[6] = (crc >> 8) & 0xFF;
  cmd[7] = crc & 0xFF;

  auto status = esp_ble_gattc_write_char(
      this->gattc_if_, this->conn_id_, handle,
      sizeof(cmd), cmd,
      ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);

  if (status == ESP_OK) {
    this->write_count_++;
  }
  // Silently ignore failures - many handles will be invalid
}

void FbotRescue::blast_all_handles() {
  ESP_LOGD(TAG, "Blasting handles 0x%04X-0x%04X (total writes so far: %u)",
           this->handle_min_, this->handle_max_, this->write_count_);
  for (uint16_t h = this->handle_min_; h <= this->handle_max_; h++) {
    this->write_register(h);
  }
}

}  // namespace fbot_rescue
}  // namespace esphome

#endif  // USE_ESP32
