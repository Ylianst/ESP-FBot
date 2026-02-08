# Home Assistant Power Station Interface

![ESP-FBot Image](https://raw.githubusercontent.com/Ylianst/ESP-FBot/refs/heads/main/docs/images/main.png)

This is a HomeAssistant, ESP-Home custom component for locally monitoring and controlling battery systems via Bluetooth. Should work with the following batteries:

 - [FOSSiBOT F3600 Pro](https://www.fossibot.com/products/fossibot-f3600-pro), [FOSSiBOT F2400](https://www.fossibot.com/products/fossibot-f2400)
 - [SYDPOWER N052](https://www.sydpower.com/product/n052?id=665461cb8b0da4a4e43e4609), [SYDPOWER N066](https://www.sydpower.com/product/detail?id=665462e4a7c432936b1b583d)
 - [AFERIY P210](https://www.aferiy.com/products/aferiy-p210-portable-power-station-2400w-2048wh), [AFERIY P310](https://www.aferiy.com/products/aferiy-p310-portable-power-station-3300w-3840wh)
 - [ABOK Power Ark3600](https://abokpower.com/all-products/portable-power-station-3600/) (Probalby works, but not tested)
 
Basically, any power station that works with the "BrightEMS" application. You normally manage these batteries thru a cloud service, which is not ideal in outage situations. Instead, you can get fast local management by loading this ESP-Home component on a small device near the battery. The device will communicate to the battery using Bluetooth and relay the data locally using WIFI to Home Assistant.

## Features

- **Battery Monitoring**: Real-time battery state of charge (%), remaining capacity (kWh), and time remaining
- **Power Monitoring**: Track input power, output power, system consumption, and total power flow
- **Output Control**: Switch USB ports, DC outputs, AC inverter, and light on/off
- **Home Assistant Integration**: All sensors and controls automatically discovered in Home Assistant
- **Auto-reconnect**: Handles BLE disconnections and automatically reconnects

## Requirements

- A compatible battery that makes use of the "BrightEMS" application. You do not need to pair the battery to WIFI.
- You need [Home Assistant](https://www.home-assistant.io/) installed with the [ESP-Home](https://esphome.io/) add-in.
- I suggest the [M5Stack ATOM Light](https://shop.m5stack.com/products/atom-lite-esp32-development-kit) device, but most ESP32 devices (ESP32-WROOM, ESP32-DevKit, etc.) will do.
- The ESP32 device will need to be within range of the battery (typically 10-30 feet)

## Getting the MAC address

You will need to get the Bluetooth LE MAC address of your battery. Search for a Bluetooth LE device that starts with the name "FOSSIBOT" or "POWER".

- On Windows, use [BluetoothLEView](https://www.nirsoft.net/utils/bluetooth_low_energy_scanner.html)

The MAC address will look like "A1:B2:C3:D4:E5:F6".

## Installation

Assuming you have Home Assistant and ESP-Home installed and know how to use it.

1. Use the example configuration below as a starting point.
2. Update WiFi credentials, API encryption key and Battery MAC address.
3. Flash to your ESP32 device.

## Configuration

Here is a ESP-Home configuration you can use. Make sure to put your own API and OTA keys and fill in the Bluetooth LE MAC address of your power station. 

<details>
<summary>Sample ESP-Home configuration file</summary>

```yaml
esphome:
  name: bigbattery
  friendly_name: Big Battery
  comment: "AFERIY 3840Wh Portable Power Station"

esp32:
  board: esp32dev
  framework:
    type: esp-idf

# Enable Home Assistant API
api:
  encryption:
    key: "(PLACE YOU KEY HERE)"

ota:
  - platform: esphome
    password: "(PLACE YOU KEY HERE)"

# This assumed you put your WIFI SSID and passing in the secrets file
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

# Enable logging
logger:
  level: WARN

# Load external component from local path
external_components:
  - source: github://ylianst/esp-fbot
    refresh: 10s

# BLE Client configuration
ble_client:
  - mac_address: "AA:BB:CC:DD:EE:FF"  # Replace with your Fossibot MAC address

# Configure the Fbot component. This component will inherit the ble_client above.
fbot:
  id: my_fbot
  polling_interval: 5s              # Status data (default: 2s)
  settings_polling_interval: 30s    # Holding registers (default: 60s)
  poll_timeout: 15s                 # Time to wait for response (default 15s)
  max_poll_failures: 5              # Failures before disconnect (default 3)

# Binary sensors for connection and output states
binary_sensor:
  - platform: fbot
    fbot_id: my_fbot
    connected:
      name: "Connected"
    # Optional sensors to indicate if the extra batteries 1 is connected
    battery_connected_s1:
      name: "Connected S1"
    # Optional sensors to indicate if the extra batteries 2 is connected
    battery_connected_s2:
      name: "Connected S2"
    usb_active:
      name: "USB Active"
    dc_active:
      name: "DC Active"
    ac_active:
      name: "AC Inverter Active"
    light_active:
      name: "Light Active"

# Sensors for battery and power readings
sensor:
  - platform: fbot
    fbot_id: my_fbot
    battery_level:
      name: "Battery"
      id: battery_percent
    # Optional sensors for the state of charge of extra battery 1
    battery_s1_level:
      name: "Battery S1"
      id: battery_percent_s1
    # Optional sensors for the state of charge of extra battery 2
    battery_s2_level:
      name: "Battery S2"
      id: battery_percent_s2
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
      name: "Total Output Power"
      id: total_watts
    remaining_time:
      name: "Remaining Minutes"
      id: remaining_minutes
    ac_out_voltage:
      name: "AC Output Voltage"
    ac_out_frequency:
      name: "AC Output Frequency"
    ac_in_frequency:
      name: "AC Input Frequency"
    time_to_full:
      name: "Time to Full Charge"
    # Optional sensors for charge/discharge thresholds
    threshold_charge:
      name: "Charge Threshold"
      id: threshold_charge
    threshold_discharge:
      name: "Discharge Threshold"
      id: threshold_discharge
    charge_level:
      name: "Charge Level"
    usb_a1_power:
      name: "USB-A1 Power"
    usb_a2_power:
      name: "USB-A2 Power"
    usb_c1_power:
      name: "USB-C1 Power"
    usb_c2_power:
      name: "USB-C2 Power"
    usb_c3_power:
      name: "USB-C3 Power"
    usb_c4_power:
      name: "USB-C4 Power"
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
    name: "Net Charging Power"
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

# Switches to control outputs
switch:
  - platform: fbot
    fbot_id: my_fbot
    key_sound:
      name: "Key Sound"
      id: keysound_switch
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

# Number controls for charge/discharge thresholds
number:
  - platform: fbot
    fbot_id: my_fbot
    # Charge threshold: stop charging when battery reaches this level (10-100%)
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

select:
  - platform: fbot
    fbot_id: my_fbot
    light_mode:
      name: "Light Mode"
    ac_charge_limit:
      name: "AC Charge Limit"
      options:
        - "300W"
        - "500W"
        - "700W"
        - "900W"
        - "1100W"
```

</details>

## Battery Factory Reset

If this integration works for you and want to remove your battery WIFI connection to the cloud, you can do a factory reset of the battery. Press and hold the `DC button`, `light button` and `USB button` simultaneously for about 5 seconds. You will hear a beep.

One known issue is that when not associated to any WIFI, the power station will show up as it's own access point with the name "ESP_xxxxxx".

### Extras

- [M5 ATOM Extras](https://github.com/Ylianst/ESP-FBot/blob/main/docs/m5atom-extras.md) - If you are using the M5 ATOM Light as you ESP32 device, check this out.
- [M5StickC PLUS2 Extras](https://github.com/Ylianst/ESP-FBot/blob/main/docs/m5stick-extras.md) - If you are using the M5StickC PLUS2 as you ESP32 device, check this out.
