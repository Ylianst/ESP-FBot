# M5StickC PLUS2 Extras

![M5Atom Button Image](https://raw.githubusercontent.com/Ylianst/ESP-FBot/refs/heads/main/docs/images/m5stick-2.png)

If you happen to be using a [M5StickC PLUS2 ESP32 Mini](https://shop.m5stack.com/products/m5stickc-plus2-esp32-mini-iot-development-kit) as your ESP32 device, here is a sample ESPHome configuration file that will display the battery state, net power input/output and what is active (AC, DC, USB, Light). I also made the device's speaker act as a remote buzzer in Home Assistant and you can also control the red LED of the device.

```yaml
esphome:
  name: yellow
  friendly_name: Yellow Device
  comment: "M5StickC PLUS2 ESP32 Mini"
  platformio_options:
    upload_speed: 115200

esp32:
  board: m5stick-c
  variant: esp32
  framework:
    type: esp-idf

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

logger:
  level: WARN
#  level: DEBUG
  logs:
    component: ERROR  # Attempt to suppress strapping pin warnings

api:
  encryption:
    key: "XXXXXXXXXXXXXXXXXX"

ota:
  - platform: esphome
    password: "XXXXXXXXXXXXXXXXXX"

light:
  - platform: monochromatic
    output: builtin_led
    name: "M5 Red LED"
    id: led1
  - platform: monochromatic
    output: backlight
    name: Backlight
    id: display_bl

output:
  - platform: ledc
    pin: GPIO19
    id: builtin_led
  - platform: ledc
    pin: 2
    inverted: true
    id: buzzer
  - platform: ledc
    pin: 27
    inverted: false
    id: backlight

spi:
  clk_pin: GPIO13
  mosi_pin: GPIO15

font:
  - file: "gfonts://Roboto"
    id: roboto_large
    size: 24
  - file: "gfonts://Roboto"
    id: roboto_medium
    size: 18
  - file: "gfonts://Roboto"
    id: roboto_small
    size: 14

color:
  - id: my_black
    red: 0%
    green: 0%
    blue: 0%
  - id: my_white
    red: 100%
    green: 100%
    blue: 100%
  - id: my_green
    red: 0%
    green: 100%
    blue: 0%
  - id: my_red
    red: 100%
    green: 0%
    blue: 0%
  - id: my_yellow
    red: 100%
    green: 100%
    blue: 0%

# 1.14 inch, 135*240 Colorful TFT LCD, ST7789v2
display:
  - platform: st7789v
    id: power_station_display
    model: TTGO TDisplay 135x240
    cs_pin: GPIO5
    dc_pin: GPIO14
    reset_pin: GPIO12
    rotation: 270
    update_interval: never
    lambda: |-
      // Check if connected to power station
      if (!id(connected).state) {
        // Not connected - show searching message (centered vertically and horizontally)
        it.filled_rectangle(0, 0, 240, 135, id(my_black));
        it.print(120, 55, id(roboto_medium), id(my_white), TextAlign::CENTER, "Searching for");
        it.print(120, 78, id(roboto_medium), id(my_white), TextAlign::CENTER, "Power Station...");
      } else {
        // Connected - show power station data
        it.filled_rectangle(0, 0, 240, 135, id(my_black));
        
        // Display battery percentage (top section)
        if (id(battery_percent).has_state()) {
          char battery_text[32];
          snprintf(battery_text, sizeof(battery_text), "Battery: %.0f%%", id(battery_percent).state);
          it.print(120, 20, id(roboto_large), id(my_white), TextAlign::TOP_CENTER, battery_text);
        }
        
        // Calculate and display net power (charging/discharging)
        if (id(input_watts).has_state() && id(output_watts).has_state()) {
          float net_power = id(input_watts).state - id(output_watts).state;
          char power_text[32];
          auto power_color = id(my_white);
          
          if (net_power > 5) {
            // Charging (net positive power)
            snprintf(power_text, sizeof(power_text), "Net: +%.0fW", net_power);
            power_color = id(my_green);
          } else if (net_power < -5) {
            // Discharging (net negative power)
            snprintf(power_text, sizeof(power_text), "Net: %.0fW", net_power);
            power_color = id(my_red);
          } else {
            // Idle (near zero)
            snprintf(power_text, sizeof(power_text), "Net: %.0fW", net_power);
            power_color = id(my_yellow);
          }
          
          it.print(120, 55, id(roboto_medium), power_color, TextAlign::TOP_CENTER, power_text);
        }
        
        // Build active outputs string
        std::string active_outputs;
        bool any_active = false;
        
        if (id(ac_active).state) {
          if (!any_active) {
            active_outputs = "Active: AC";
          } else {
            active_outputs += ", AC";
          }
          any_active = true;
        }
        if (id(dc_active).state) {
          if (!any_active) {
            active_outputs = "Active: DC";
          } else {
            active_outputs += ", DC";
          }
          any_active = true;
        }
        if (id(usb_active).state) {
          if (!any_active) {
            active_outputs = "Active: USB";
          } else {
            active_outputs += ", USB";
          }
          any_active = true;
        }
        if (id(light_active).state) {
          if (!any_active) {
            active_outputs = "Active: Light";
          } else {
            active_outputs += ", Light";
          }
          any_active = true;
        }
        
        if (!any_active) {
          active_outputs = "Nothing Active";
        }
        
        // Display active outputs (bottom section)
        it.print(120, 95, id(roboto_small), id(my_white), TextAlign::TOP_CENTER, active_outputs.c_str());
      }

# Script to update display
script:
  - id: update_display
    then:
      - component.update: power_station_display
  - id: turn_off_backlight_delayed
    then:
      - delay: 20s
      - light.turn_off: display_bl

# Load external component from local path
external_components:
  - source: github://ylianst/esp-fbot
    refresh: 10s

# BLE Client configuration
ble_client:
  - mac_address: "A8:46:74:41:4C:42"  # Replace with your Fossibot MAC address
    id: ble_client_1

# Configure the Fbot component. This component will inherit the ble_client above.
fbot:
  id: my_fbot
  ble_client_id: ble_client_1
  polling_interval: 5s              # Status data (default: 2s)
  settings_polling_interval: 30s    # Holding registers (default: 60s)

# Binary sensors for connection and output states
binary_sensor:
  - platform: fbot
    fbot_id: my_fbot
    connected:
      name: "Connected"
      id: connected
      on_state:
        then:
          # When connection state changes, update the LED
          - script.execute: update_display
    # Optional sensors to indicate if the extra batteries 1 is connected
    #battery_connected_s1:
    #  name: "Connected S1"
    # Optional sensors to indicate if the extra batteries 2 is connected
    #battery_connected_s2:
    #  name: "Connected S2"
    usb_active:
      internal: true
      name: "USB Active"
      id: usb_active
      on_state:
        then:
          - script.execute: update_display
    dc_active:
      internal: true
      name: "DC Active"
      id: dc_active
      on_state:
        then:
          - script.execute: update_display
    ac_active:
      internal: true
      name: "AC Inverter Active"
      id: ac_active
      on_state:
        then:
          - script.execute: update_display
    light_active:
      internal: true
      name: "Light Active"
      id: light_active
      on_state:
        then:
          - script.execute: update_display
  # Top Button
  - platform: gpio
    pin:
      number: GPIO37
      inverted: true
    name: Button A
    on_press:
      then:
        # Turn on backlight and stop any running delayed turn-off script
        - script.stop: turn_off_backlight_delayed
        - light.turn_on: display_bl
    on_release:
      then:
        # Start the delayed turn-off timer (resets if already running)
        - script.execute: turn_off_backlight_delayed
  # Side Button
  - platform: gpio
    pin:
      number: GPIO39
      inverted: true
    name: Button B
    on_press:
      then:
        # Toggle the power station light
        - switch.toggle: light_switch

# Sensors for battery and power readings
sensor:
  - platform: fbot
    fbot_id: my_fbot
    battery_level:
      name: "Battery"
      id: battery_percent
      on_value:
        then:
          # When battery level changes, update the display
          - script.execute: update_display
    ac_input_power:
      name: "AC Input Power"
      id: ac_input_watts
    dc_input_power:
      name: "DC Input Power"
      id: dc_input_watts
    input_power:
      name: "Total Input Power"
      id: input_watts
    output_power:
      name: "Output Power"
      id: output_watts
    system_power:
      name: "System Power"
      id: system_watts
    total_power:
      name: "Total Power"
      id: total_watts
    remaining_time:
      internal: true
      name: "Remaining Minutes"
      id: remaining_minutes
    # Optional sensors for charge/discharge thresholds
    threshold_charge:
      name: "Charge Threshold"
      id: threshold_charge
    threshold_discharge:
      name: "Discharge Threshold"
      id: threshold_discharge
    charge_level:
      name: "Charge Level"
  # Convert remaining time from minutes to hours
  - platform: template
    name: "Remaining Hours"
    lambda: |-
      if (id(remaining_minutes).has_state()) {
        return id(remaining_minutes).state / 60.0;
      }
      return 0.0;
    unit_of_measurement: "h"
    accuracy_decimals: 1
    device_class: duration
    state_class: measurement
    update_interval: 5s
  # Net power (positive = charging, negative = discharging)
  - platform: template
    name: "Net Power"
    lambda: |-
      if (id(input_watts).has_state() && id(output_watts).has_state()) {
        return id(input_watts).state - id(output_watts).state;
      }
      return 0.0;
    unit_of_measurement: "W"
    accuracy_decimals: 0
    device_class: power
    state_class: measurement
    update_interval: 5s
  - platform: adc
    pin: GPIO38
    attenuation: 12db
    name: "M5 Battery Voltage"
    id: battery_voltage
    update_interval: 30s
    unit_of_measurement: "V"
    filters:
      - multiply: 2.0
      # Optional: Add a sliding window to smooth out jumpy readings
      - sliding_window_moving_average:
          window_size: 5
          send_every: 1
  - platform: template
    name: "M5 Battery Level"
    id: battery_level
    device_class: battery
    unit_of_measurement: "%"
    accuracy_decimals: 0
    update_interval: 30s
    lambda: |-
      if (id(battery_voltage).has_state()) {
        float vol = id(battery_voltage).state;
        // Adjusted for your 4.28V reading
        float percentage = (vol - 3.2) / (4.2 - 3.2) * 100.0;
        
        if (percentage > 100.0) return 100.0;
        if (percentage < 0.0) return 0.0;
        return percentage;
      }
      return {}; // Wait for a valid voltage

# Switches to control outputs
switch:
  - platform: fbot
    fbot_id: my_fbot
    usb:
      name: "USB Output"
      id: usb_switch
    dc:
      name: "DC Output"
      id: dc_switch
    ac:
      name: "AC Inverter"
      id: ac_switch
    light:
      name: "Light"
      id: light_switch
    ac_silent:
      name: "AC Silent Charging"
      id: ac_silent_switch
  - platform: template
    name: "Remote Buzzer"
    id: remote_buzzer
    icon: "mdi:bullhorn"
    turn_on_action:
      - output.turn_on: buzzer
      - output.ledc.set_frequency:
          id: buzzer
          frequency: "1000Hz"
      - output.set_level:
          id: buzzer
          level: "50%"
    turn_off_action:
      - output.turn_off: buzzer

# Number controls for charge/discharge thresholds
number:
  - platform: fbot
    fbot_id: my_fbot
    # Charge threshold: stop charging when battery reaches this level (60-100%)
    threshold_charge:
      name: "Charge Max"
      min_value: 10
      max_value: 100
      step: 1
    # Discharge threshold: stop discharging when battery reaches this level (0-50%)
    threshold_discharge:
      name: "Discharge Min"
      min_value: 0
      max_value: 50
      step: 1

```
