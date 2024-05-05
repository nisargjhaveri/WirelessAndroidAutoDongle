# Wireless Android Auto Dongle

Use Wireless Android Auto with a car that supports only wired Android Auto using a Raspberry Pi.

This repository consists of the buildroot setup to generate an sd card image to create your own Wireless Android Auto Dongle.

## Features
- Native Wireless Android Auto connection to the phone, no extra app needed on the phone.
- Passes through all Android Auto traffic without any modifications to ensure seamless and safe experience.
- Fast bootup, connection under 30 seconds.
- Supports multiple boards (Currently multiple Raspberry Pi boards).

## Supported Hardware
This is currently tested and built for multiple Raspberry Pi boards supporting USB OTG.

The setup should technically work on any devices with these basic requirements (albeit, with some modifications).
- The board should support USB OTG or Gadget mode.
- Has Wifi and Bluetooth. External should also work if not in-built.
- Should be able to operate on power provided by the car.

## Install and run
[Download a pre-built sd card image](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/releases) or build one as described below. Install the image on the SD card using your favorite tool.

You may want to update the `country_code` in the `/etc/hostapd.conf` file.

### First-time connection
- Connect the phone to headunit via USB cable, make sure Android Auto starts. Disconnect phone.
- Connect the board to the car. Make sure to use a data cable, with the USB OTG enabled port on the board.
- Open Bluetooth settings and pair the new device called "AndroidAuto-Dongle" or "AA Wireless Dongle" on your phone.
- After this phone should automatically connect via Wifi and the dongle will connect to the headunit via USB and start Android Auto on the car screen.

### Subsequent connections
From the next time, it should automatically connect to the phone and start Android Auto.

Make sure your Bluetooth and Wifi are enabled on the phone.

## Troubleshoot
Once you've already tried multiple times and it still does not work, you can ssh into the device and try to get some logs.

- Connect the device to the headunit, let it boot and try to connect once. The logs are not persisted across reboots, so you need to get the logs in the same instance soon after you observe the issue.
- Connect to the device using wifi (SSID:AAWirelessDongle, Password: ConnectAAWirelessDongle, see [hostapd.conf](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/blob/main/aa_wireless_dongle/board/common/rootfs_overlay/etc/hostapd.conf)).
- SSH into the device (username: root, password: password, see relevant defconfigs e.g. [raspberrypi0w_defconfig](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/blob/main/aa_wireless_dongle/configs/raspberrypi0w_defconfig)).
- Once you're in, try to have a look at `/var/log/messages` file, it should have most relevant logs to start with. You can also copy the file and attach to issues you create if any.

## Hardening / making system read-only
Sometimes it is desirable (because of SD cards longevity) to make a whole system read-only. This would also help because we don't have any control when the car headunit is powering off the dongle (USB port).<br>
In some corner cases the filesystem could be damaged because the system is not properly shutdown and unmounted.

When you have the dongle set up properly and it is working as intended (you was connecting with your phone to the car, BT was paired, AA is working) you can make the following changes in the SD card:

_Partition #1 (boot):_<br>
edit the `cmdline.txt` file and add `ro` at the end of the line

_Partition #2 (main filesystem):_
```diff
--- old/etc/fstab	2024-03-30 17:44:15.000000000 +0100
+++ new/etc/fstab	2024-05-03 16:33:48.083059982 +0200
@@ -1,5 +1,5 @@
 # <file system>	<mount pt>	<type>	<options>	<dump>	<pass>
-/dev/root	/		ext2	rw,noauto	0	1
+/dev/root	/		ext2	ro,noauto	0	1
 proc		/proc		proc	defaults	0	0
 devpts		/dev/pts	devpts	defaults,gid=5,mode=620,ptmxmode=0666	0	0
 tmpfs		/dev/shm	tmpfs	mode=0777	0	0
diff -Nru 22/etc/inittab pizero-aa-backup/p2/etc/inittab
--- old/etc/inittab	2024-03-30 18:57:51.000000000 +0100
+++ new/etc/inittab	2024-05-03 16:45:24.184119996 +0200
@@ -15,7 +15,7 @@
 
 # Startup the system
 ::sysinit:/bin/mount -t proc proc /proc
-::sysinit:/bin/mount -o remount,rw /
+#::sysinit:/bin/mount -o remount,rw /
 ::sysinit:/bin/mkdir -p /dev/pts /dev/shm
 ::sysinit:/bin/mount -a
 ::sysinit:/bin/mkdir -p /run/lock/subsys
```

Again: before doing this, make sure that you've connect your phone at least once and all is working fine, specifically the `/var/lib/bluetooth/` directory is populated with your phone pairing information.<br>
This way after reboot all partitions will stay in read-only mode and should work longer and without possible problems.

If you want to make some changes to the filesystem or pair new phone you should revert those changes and it will be read-write again.<br>
It should be also possible to `ssh` and execute:<br>
`mount -o remount,rw /`<br>
to make root filesystem read-write again temporarily.

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
