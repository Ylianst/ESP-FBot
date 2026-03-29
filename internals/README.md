# Power Station Internals

I have a AFERIY P310 that is stuck in a reboot loop, likely because a bad setting was sent to the device using Bluetooth. This is a really sad thing about these devices, anyone with a cellphone in proximity of this battery could, without any authentication, send a bad command to the battery to cause it to go into a reboot loop basically bricking it. On this page, we will be taking a look at the internals of the device and how it works.

## Opening the Battery

Obviously, this is a battery with a lot of power and a huge inverter so, be super careful when doing this. The first step to to remove the top of the device. There are a set of rubber plugs and two rubber strips that are easy to remove. After that, remove all of the screws and take off the top of the device. I then you can removed the sides but this may not be needed because depending on what you do, you may have access to the pins you want to monitor. The sides are a bit more difficult to remove as there are healed at two snap-in locations at the bottom. Once you know these locations it's easier to un-snap and pull up. There is the battery with the top and sides removed.

<img src="images/battery-opened.jpeg" alt="AFERIY P310 Opened up" width="400"/>

I kept a small bag with all of the screws. Luckly it's very clear what goes where. If you take your voltmeter out and test around, you will see there are two bars that are exposed with 48v on them. So again, be super careful if you do this. In my case, I then took the front motherboard out.

## Looking at the Motherboard

There is while glue used to make sure nothing gets disconnected during transport which is easy to remove. You have to disconnect 3 white connectors and 1 power connection from the main board. All the connectors are a bit different so, it's not possible to put them back incorrectly which is nice. Here is a picture of the main board:

<img src="images/board-top.jpeg" alt="AFERIY P310 Motherboard Image" width="400"/>

I have [other pictures of the motherboard here](https://github.com/Ylianst/ESP-FBot/tree/main/internals/boardimages). At the bottom of the board, I see the following indication:

<img src="images/board-number.png" alt="AFERIY P310 Board Indication" width="400"/>

It says "SYD-N051-DC-V1.5" on the first line and "20230915" on the second line. You can immidiately see that this is a board from [Shenzhen SYD Network Technology Co. Ltd.](https://sydpower.com/) and on their web site there is a range of power stations that you can get and brand anyway you like. This is the original source of the battery with AFERIY and others re-branding it.

It looks like the board is run by an ARM Cortex processor, but you can see that they took extra care to black out all of the chips so to make them more difficult to identify. On the top left is the typical ARM debug port with 4 connectors (3.3v, GND, SWCLK, SWDIO on the J13 connector).

<img src="images/board-debug-port.png" alt="AFERIY P310 Board Debug Port" width="400"/>

This is typically a Serial Wire Debug (SWD) for ARM Cortex processors. It's typically used to flash and debug the ARM processor. On the top right of the motherboard is the ESP32 chip that does WIFI and Bluetooth. The 2.4Ghz antenna is the small wiggle in the black area.

<img src="images/board-esp32.jpeg" alt="AFERIY P310 ESP32" width="400"/>

This is a `ESP32-C3-MINI-1 M4N4`, you can find the [documentation for this chip here](https://documentation.espressif.com/esp32-c3-mini-1_datasheet_en.pdf). Going with a voltmeter and just testing the pins along with looking at the location of the pins on the ESP32-C3-MINI-1, I think this are the pin connections for the ESP32:

```
    Ground *   * Ground
      3.3v *   * Ground
    Ground *   * Ground
EN (Reset) *   * 21 UART TX   <-- ESP32 to ARM
    Ground *   * 20 UART RX   <-- ARM to ESP32
```

While there are 10 pins connects to the ESP32, there are only really 5: Ground, 3.3v, Enable, TX and RX.

## ARM to ESP32 Communication

I put the motherboard back into the battery and used a [cheap 10$ USB logic analyser](https://www.amazon.com/dp/B0CHZ13R6D) to spy on the communication between the ARM and ESP32 using software called [PulseView](https://sigrok.org/wiki/PulseView). My one hand, I pushed the channel 4 and 5 of the logic analyser wires to the UART RX and TX, started the PluseView recording and powered on the battery. It's crazy that it worked, but it did. You can then easily decode the messages between both chips.

You can find my [PulseView capture files here](https://github.com/Ylianst/ESP-FBot/tree/main/internals/pulseview). "Capture001" was my first succesful capture of both TX and RX pins at a sampling rate that is way too high since I did not know what I would except. D4 is from the ARM to the ESP32, and D5 are the responses from the ESP32.

It turns out the ARM Cortex chip talks to the ESP32 using standard 115200,N,8,1 serial settings and using standard "AT" commands. It seems like the ESP32 is loaded with a standard "proxy" firmwarm from Espressif and documentation of the AT commands is here: [Espressif AT Command Set](https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Command_Set/Basic_AT_Commands.html)

So, the ARM chip instructions the ESP32 to enable it's Bluetooth, command to WIFI, etc. over a standard serial port.