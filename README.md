# Home Assistant Power Station Interface

![ESP-FBot Image](https://raw.githubusercontent.com/Ylianst/ESP-FBot/refs/heads/main/docs/images/main.png)

This is a HomeAssistant, ESP-Home custom component for locally monitoring and controlling battery systems via Bluetooth. Should work with the following batteries:

 - [FOSSiBOT F3600 Pro](https://www.fossibot.com/products/fossibot-f3600-pro), [FOSSiBOT F2400](https://www.fossibot.com/products/fossibot-f2400)
 - [SYDPOWER N052](https://www.sydpower.com/product/n052?id=665461cb8b0da4a4e43e4609), [SYDPOWER N066](https://www.sydpower.com/product/detail?id=665462e4a7c432936b1b583d)
 - [AFERIY P210](https://www.aferiy.com/products/aferiy-p210-portable-power-station-2400w-2048wh), [AFERIY P310](https://www.aferiy.com/products/aferiy-p310-portable-power-station-3300w-3840wh)
 
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
- I suggest the [M5Stack ATOM Light](https://shop.m5stack.com/products/atom-lite-esp32-development-kit) or the [M5StickC PLUS2 ESP32 Mini](https://shop.m5stack.com/products/m5stickc-plus2-esp32-mini-iot-development-kit) device, but most ESP32 devices (ESP32-WROOM, ESP32-DevKit, etc.) will do.
- The ESP32 device will need to be within range of the battery (typically 10-30 feet)

## Getting the MAC address

You will need to get the Bluetooth LE MAC address of your battery. Search for a Bluetooth LE device that starts with the name "FOSSIBOT" and "POWER".

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
  polling_interval: 5s  # How often to poll for updates (default: 2s)

# Binary sensors for connection and output states
binary_sensor:
  - platform: fbot
    fbot_id: my_fbot
    connected:
      name: "Connected"
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
    input_power:
      name: "Input Power"
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
      name: "Remaining Minutes"
      id: remaining_minutes
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
```

</details>

## Battery Factory Reset

If this integration works for you and want to remove your battery WIFI connection to the cloud, you can do a factory reset of the battery. Press and hold the `DC button`, `light button` and `USB button` simultaneously for about 5 seconds. You will hear a beep.