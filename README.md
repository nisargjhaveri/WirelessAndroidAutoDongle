# Wireless Android Auto Dongle

DIY Wireless Android Auto adapter to use with a car that supports only wired Android Auto using a Raspberry Pi.

This repository consists of the buildroot setup to generate an sd card image to create your own Wireless Android Auto adapter.

## Features
- Native Wireless Android Auto connection to the phone, no extra app needed on the phone.
- Passes through all Android Auto traffic without any modifications to ensure seamless and safe experience.
- Fast bootup, connection under 30 seconds.
- Supports multiple boards (Currently multiple Raspberry Pi boards).

## Supported Hardware
This is currently tested and built for the following Raspberry Pi boards supporting USB OTG.
- **Raspberry Pi Zero W**
- **Raspberry Pi Zero 2 W**
- **Raspberry Pi 3 A+** _(Raspberry Pi 3 B+ is not supported)_
- **Raspberry Pi 4**

In theory, this can be extended to more hardware in future with these basic requirements.

- The board should support USB OTG or Gadget mode.
- Has Wifi and Bluetooth. External should also work if not in-built.
- Should be able to operate on power provided by the car.

## Install and run
[Download a pre-built sd card image](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/releases) for your board. You can also [build one yourself](BUILDING.md). Install the image on the SD card using your favorite tool.

You may want to update the `country_code` in the `/etc/hostapd.conf` file.

### First-time connection
- Connect the phone to headunit via USB cable, make sure Android Auto starts. Disconnect phone.
- Connect the board to the car. Make sure to use a data cable, with the USB OTG enabled port on the board.
    - On **Raspberry Pi Zero W** and **Raspberry Pi Zero 2 W**: Use the second micro-usb port marked "USB" and not "PWR".
    - On **Raspberry Pi 3 A+**: Use the only USB-A port with an USB-A to USB-A cable.
    - On **Raspberry Pi 4**, use the USB-C port user for normally powering the board.
- Open Bluetooth settings and pair the new device called `AndroidAuto-Dongle-*` or `WirelessAADongle-*` on your phone.
- After this phone should automatically connect via Wifi and the dongle will connect to the headunit via USB and start Android Auto on the car screen.

### Subsequent connections
From the next time, it should automatically connect to the phone and start Android Auto.

Make sure your Bluetooth and Wifi are enabled on the phone.

## Troubleshoot

### Common issues

#### Bluetooth and Wifi seems connected, but the phone stuck at "Looking for Android Auto"
The most common issue behind this is either bad USB cable or use of wrong USB port on the device. Make sure:
1. The cable is good quality data cable and not power-only cable
2. You're using the OTG enabled usb port on the board, and not the power-only port.

### Getting logs
Once you've already tried multiple times and it still does not work, you can ssh into the device and try to get some logs.

- Connect the device to the headunit, let it boot and try to connect once. The logs are not persisted across reboots, so you need to get the logs in the same instance soon after you observe the issue.
- Connect to the device using wifi (SSID:AAWirelessDongle, Password: ConnectAAWirelessDongle, see [hostapd.conf](aa_wireless_dongle/board/common/rootfs_overlay/etc/hostapd.conf)).
- SSH into the device (username: root, password: password, see relevant defconfigs e.g. [raspberrypi0w_defconfig](aa_wireless_dongle/configs/raspberrypi0w_defconfig)).
- Once you're in, try to have a look at `/var/log/messages` file, it should have most relevant logs to start with. You can also copy the file and attach to issues you create if any.

## Contribute
[Find or create a new issue](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/issues) for any bugs or improvements.

Feel free to [Create a PR](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/pulls) to fix any issues. Refer [BUILDING.md](BUILDING.md) for instructions on how to build locally.

## Support 
Please [consider sponsoring](https://github.com/sponsors/nisargjhaveri) if you find the project useful. Even a small donation helps. This will help continuing fixing issues and getting support for more devices and headunit in future.

In any case, don't forget to star on github and spread the word if you think this project might be useful to someone else as well.

## Limitations
This is currently tested with very limited set of headunits and cars. Let me know if it does not work with your headunit.
