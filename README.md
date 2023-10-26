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
Use docker to build:
```shell
git clone --recurse-submodules https://github.com/hkfuertes/AAWirelessDongle
docker-compose run build_XXXX # See docker-compose.yml for available options.
```

You may want to update the `country_code` in the relavent `hostapd.conf` file for your board. See [/board/common/rootfs_overlay/etc/hostapd.conf](aa_wireless_dongle/board/common/rootfs_overlay/etc/hostapd.conf) and other board specific overrides.


## Install and run
Build should generate the sd card image at `images/sdcard.img` in your output directory. Install this image on the SD card using your favourite tool.

That should be it. Insert the SD card and connect the board to the car. Make sure to use a data cable, with the USB OTG enabled port.

On your phone, open Bluetooth settings and pair a new device called "AA Wireless Dongle". Once paired, it should automatically start the Wireless Android Auto on the car screen.

From the next time, it should automatically connect to your phone, no need to pair again. Make sure your Bluetooth and Wifi are enabled on the phone.

## Limitations
- Tested with very limited set of headunits and cars. Let me know if it does not work with your headunit.
