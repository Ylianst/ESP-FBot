# M5Atom Extras

![M5Atom Button Image](https://raw.githubusercontent.com/Ylianst/ESP-FBot/refs/heads/main/docs/images/atom-button.png)

If you happen to be using a [M5Stack ATOM Light](https://shop.m5stack.com/products/atom-lite-esp32-development-kit) as your ESP32 device, there is an extra fun trick you can add to make this integration even more useful. There is a button on the ATOM you can configure to toggle one of the functions on the battery and this will work even of Home Assistant is offline. With this configuration, the M5Atom will also act as a bride for Home Assistant to read the power station's state over WIFI.

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

Nice way to control your battery from a short distance. See a [full example configuration of this here](https://github.com/Ylianst/ESP-FBot/blob/main/docs/example-m5atom-button.yaml).

## Light Configuration

You can also use the RGB LED light on the ATOM light to show the state of the battery. To get started, add this new section:

```yaml
# RGB LED on M5 ATOM (Pin 27, WS2812B)
light:
  - platform: esp32_rmt_led_strip
    chipset: WS2812
    pin: 27
    num_leds: 1
    rgb_order: GRB
    rmt_channel: 0
    id: status_led
    name: "M5Atom Status LED"
    default_transition_length: 0.5s
    restore_mode: ALWAYS_ON
    effects:
      - pulse:
          name: "Pulse"
          transition_length: 1s
          update_interval: 1s
```


### Light Showing State of Charge

Now that you have control over the light we can make it do things. In this first example, we will have it change color based on the state of change of the battery. Change the battery level sensor to this:

```yaml
sensor:
  - platform: fbot
    fbot_id: my_fbot
    battery_level:
      name: "Battery"
      id: battery_percent
      on_value:
        then:
          - if:
              condition:
                lambda: 'return x > 30;'
              then:
                - light.turn_on:
                    id: status_led
                    red: 0%
                    green: 100%
                    blue: 0%
                    brightness: 50%
          - if:
              condition:
                lambda: 'return x > 15 && x <= 30;'
              then:
                - light.turn_on:
                    id: status_led
                    red: 100%
                    green: 100%
                    blue: 0%
                    brightness: 50%
          - if:
              condition:
                lambda: 'return x <= 15;'
              then:
                - light.turn_on:
                    id: status_led
                    red: 100%
                    green: 0%
                    blue: 0%
                    brightness: 50%
```

This will change the color to green at over 30%, yellow at more than 15% and red at 15% or lower. You can also change the connected battery sensor to this:

```yaml
binary_sensor:
  - platform: fbot
    fbot_id: my_fbot
    connected:
      name: "Connected"
      on_state:
        then:
          - if:
              condition:
                binary_sensor.is_off: connected
              then:
                - light.turn_off: status_led
```

This way, if the ATOM light disconnects from the battery, the light will turn off. See a [full example configuration of this here](https://github.com/Ylianst/ESP-FBot/blob/main/docs/example-m5atom-light-soc.yaml).

### Light Showing State of Switch

As an alternative to showing the state of charge, in this example we are going to only turn the light on and green when the AC inverter is enabled. In the `ac_active` section, all the `on_state` text here.

```yaml
    ac_active:
      name: "AC Inverter Active"
      on_state:
        then:
          - if:
              condition:
                binary_sensor.is_on: ac_active
              then:
                - light.turn_on:
                    id: status_led
                    red: 0%
                    green: 100%
                    blue: 0%
                    brightness: 50%
              else:
                - light.turn_off: status_led
```

There is also a [full example configuration of this here](https://github.com/Ylianst/ESP-FBot/blob/main/docs/example-m5atom-light-switch.yaml). You can combine this with the use of the button above to toggle the AC inverter. You can also change this code a bit to show the state of the other switches if you like.

## Full Configuration

Since I have a M5 ATOM Light, here is the [configuration I use](https://github.com/Ylianst/ESP-FBot/blob/main/docs/example-m5atom-full.yaml).

- The button with toggle the light on the battery.
- The RGB light will be white if it can't connect to the battery.
- The RGB light will be off normally, but on when the AC inverter is active.
- When the AC inverter is active, the color of the light will show state of change (Green over 30%, Yellow over 15%, Red at 15% or under).

This seems a resonable configuration and you it's pretty easy to change it as you want.
