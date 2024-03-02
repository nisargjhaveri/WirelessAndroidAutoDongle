# Wireless Android Auto Dongle

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

## Install and run
[Download a pre-built sd card image](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/releases) or build one as described below. Install the image on the SD card using your favorite tool.

You may want to update the `country_code` in the `/etc/hostapd.conf` file.

That should be it. Insert the SD card and connect the board to the car. Make sure to use a data cable, with the USB OTG enabled port.

Connect the device's OTG enabled usb port to car's usb port.
Once it boots up, open Bluetooth settings and pair a new device called "AA Wireless Dongle" on your phone.
After this it will try to connect to the car via USB and automatically start the Wireless Android Auto on the car screen.

From the next time, it should automatically connect to your phone, no need to pair again. Make sure your Bluetooth and Wifi are enabled on the phone.

## Troubleshoot
Once you've already tried multiple times and it still does not work, you can ssh into the device and try to get some logs.

- Connect the device to the headunit, let it boot and try to connect once. The logs are not persisted across reboots, so you need to get the logs in the same instance soon after you observe the issue.
- Connect to the device using wifi (SSID:AAWirelessDongle, Password: ConnectAAWirelessDongle, see [hostapd.conf](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/blob/main/aa_wireless_dongle/board/common/rootfs_overlay/etc/hostapd.conf)).
- SSH into the device (username: root, password: password, see relevant defconfigs e.g. [raspberrypi0w_defconfig](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/blob/main/aa_wireless_dongle/configs/raspberrypi0w_defconfig)).
- Once you're in, try to have a look at `/var/log/messages` file, it should have most relevant logs to start with. You can also copy the file and attach to issues you create if any.

## Build

### Clone
```shell
$ git clone --recurse-submodules https://github.com/nisargjhaveri/WirelessAndroidAutoDongle
```

### Build with Docker
```shell
$ docker compose run --rm rpi4 # See docker-compose.yml for available options.
```

You can use `rpi0w`, `rpi02w`, `rpi3a` or `rpi4` to build and generate an sdcard image. Once the build is successful, it'll copy the generated sdcard image in `images/` directory.

You can also use the `bash` service for more control over the build process and experimentation.

```shell
$ docker compose run --rm bash
```

### Build with manual setup
Once you have a recursive clone, you can manually build using the following set of commands.

```shell
$ cd buildroot
$ make BR2_EXTERNAL=../aa_wireless_dongle/ O=output/rpi0w raspberrypi0w_defconfig # Change output and defconfig for your board
$ cd output/rpi0w
$ make
```

When successful, this should generate the sd card image at `images/sdcard.img` in your output directory. See the "Install and Run" instructions above to use this image.

Use one of the following defconfig for the board you intend to use:
- `raspberrypi0w_defconfig` - Raspberry Pi Zero W
- `raspberrypizero2w_defconfig` - Raspberry Pi Zero 2 W
- `raspberrypi3a_defconfig` - Raspberry Pi 3A+
- `raspberrypi4_defconfig` - Raspberry Pi 4

## Limitations
- Tested with very limited set of headunits and cars. Let me know if it does not work with your headunit.
