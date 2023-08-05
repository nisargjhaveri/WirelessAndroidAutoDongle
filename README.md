# AAWirelessDongle

Use Wireless Android Auto with a car that supports only wired Android Auto using a Raspberry Pi.

This repository consists of the buildroot setup to generate an sd card image to create your own Wireless Android Auto Dongle.

## Features
- Native Wireless Android Auto connection to the phone, no extra app needed on the phone.
- Passes through all Android Auto traffic without any modifications to ensure seamless and safe experience.
- Fast bootup, connection under 30 seconds.
- Supports multiple boards (Currently multiple Raspberry Pi boards).

## Supported Hardware
The setup should work on any devices with these basic requirements (albeit, with some modifications).
- The board should support USB OTG or Gadget mode.
- Has Wifi and Bluetooth. External should also work if not in-built.
- Should be able to operate on power provided by the car.

Currently tested with multiple Raspberry Pi boards supporting USB OTG.

## Build
### Start the vagrant box
```
$ git clone https://github.com/nisargjhaveri/AAWirelessDongle
$ vagrant up
$ vagrant ssh
```

### Setup buildroot environment (inside the VM) and build
```shell
$ git clone --recurse-submodules https://github.com/nisargjhaveri/AAWirelessDongle
$ cd AAWirelessDongle/buildroot
$ make BR2_EXTERNAL=../aa_wireless_dongle/ O=output/rpi0w raspberrypi0w_defconfig # Change output and defconfig for your board
$ cd output/rpi0w
$ make
```

Use one of the following defconfig for the board you intend to use:
- `raspberrypi0w_defconfig` - Raspberry Pi Zero W
- `raspberrypizero2w_defconfig` - Raspberry Pi Zero 2 W
- `raspberrypi4_defconfig` - Raspberry Pi 4

## Install and run
Build should generate the sd card image at `images/sdcard.img` in your output directory.  Install this image on the SD card using your favourite tool.

That should be it. Insert the SD card and connect the board to the car. Make sure to use a data cable, with the USB OTG enabled port.

On your phone, open Bluetooth settings and pair a new device called "AA Wireless Dongle". Once pair, it should automatically start the Wireless Android Auto on the car screen.

From the next time, it should automatically connect to your phone, no need to pair again. Make sure your Bluetooth and Wifi and enabled on the phone.

## Limitations
- Tested with very limited set of headunits and cars. Let me know if it does not work in your car.
