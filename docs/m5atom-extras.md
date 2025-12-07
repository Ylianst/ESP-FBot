# M5Atom Extras

![M5Atom Button Image](https://raw.githubusercontent.com/Ylianst/ESP-FBot/refs/heads/main/docs/images/atom-button.png)

If you happen to be using a [M5Stack ATOM Light](https://shop.m5stack.com/products/atom-lite-esp32-development-kit) as your ESP32 device, there is an extra fun trick you can add to make this integration even more useful. There is a button on the ATOM you can configure to toggle one of the functions on the battery and this will work even of Home Assistant is offline.

## Button Configuration

Add the following configuration to the `binary_sensor` section of your YAML file. This will enable the button and connect it to the `light_switch`on the battery.

```yaml
# Binary sensors for connection and output states
binary_sensor:
  - platform: gpio
    pin:
      number: 39
      inverted: true
    name: M5Atom Button
    on_press:
      then:
      - switch.toggle: light_switch
```

Instead of `light_switch` the other possible values are `switch.toggle` are `usb_switch`, `dc_switch` and `ac_switch`.

Nice way to control your battery from a short distance.
