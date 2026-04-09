#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#ifdef USE_ESP32

#include <esp_gattc_api.h>

namespace esphome {
namespace fbot_rescue {

class FbotRescue : public esphome::ble_client::BLEClientNode, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

  void set_register(uint16_t reg) { this->register_ = reg; }
  void set_value(uint16_t value) { this->value_ = value; }
  void set_handle_min(uint16_t handle_min) { this->handle_min_ = handle_min; }
  void set_handle_max(uint16_t handle_max) { this->handle_max_ = handle_max; }
  void set_blast_interval(uint32_t interval) { this->blast_interval_ = interval; }
  void set_blast_duration(uint32_t duration) { this->blast_duration_ = duration; }

 protected:
  uint16_t calculate_checksum(const uint8_t *data, size_t len);
  void write_register(uint16_t handle);
  void blast_all_handles();

  // Configuration
  uint16_t register_{2};
  uint16_t value_{0x00FF};
  uint16_t handle_min_{0x0002};
  uint16_t handle_max_{0x0020};
  uint32_t blast_interval_{500};
  uint32_t blast_duration_{5000};

  // Runtime state
  esp_gatt_if_t gattc_if_{0};
  uint16_t conn_id_{0};
  bool connected_{false};
  bool blast_active_{false};
  uint32_t blast_start_time_{0};
  uint32_t last_blast_time_{0};
  uint16_t discovered_handle_{0};
  uint32_t write_count_{0};
};

}  // namespace fbot_rescue
}  // namespace esphome

#endif  // USE_ESP32
