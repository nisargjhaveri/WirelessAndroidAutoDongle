# Wireless Android Auto Dongle Documentation

## Project Overview

The Wireless Android Auto Dongle project aims to create a DIY Wireless Android Auto adapter using a Raspberry Pi. This adapter allows you to use Android Auto wirelessly with a car that only supports wired Android Auto.

## Setup Instructions

### Prerequisites

- A Raspberry Pi board (Zero W, Zero 2 W, 3 A+, or 4)
- A microSD card (at least 8GB)
- A computer with an SD card reader
- A data cable to connect the Raspberry Pi to the car's USB port

### Installation

1. [Download a pre-built sd card image](https://github.com/nisargjhaveri/WirelessAndroidAutoDongle/releases) for your board or [build one yourself](../BUILDING.md).
2. Install the image on the SD card using your favorite tool (e.g., Balena Etcher).
3. Insert the SD card into the Raspberry Pi.
4. Connect the Raspberry Pi to the car's USB port using a data cable.
5. Power on the Raspberry Pi by connecting it to the car's USB port.

### First-time Connection

1. Connect your phone to the car's head unit via a USB cable and ensure Android Auto starts. Disconnect the phone afterward.
2. Pair your phone with the new Bluetooth device called "AndroidAuto-Dongle" or "AA Wireless Dongle."
3. The phone should automatically connect via Wi-Fi, and the dongle will connect to the head unit via USB, starting Android Auto on the car screen.

### Subsequent Connections

Ensure Bluetooth and Wi-Fi are enabled on your phone. The phone should automatically connect to the dongle and start Android Auto.

## Code Structure

The project is organized as follows:

- `aa_wireless_dongle/`: Contains the main code for the Wireless Android Auto Dongle.
  - `board/`: Contains board-specific configurations and files.
  - `configs/`: Contains configuration files for different Raspberry Pi boards.
  - `package/`: Contains packages and their configurations.
  - `patches/`: Contains patches for various components.
- `buildroot/`: Contains the Buildroot setup for building the project.
- `docker-compose.yml`: Docker Compose configuration for building the project.
- `Dockerfile`: Dockerfile for setting up the build environment.
- `BUILDING.md`: Instructions for building the project.
- `README.md`: Project overview and basic setup instructions.
- `docs/`: Contains detailed documentation (this file).

## Project Architecture and Components

### Overview

The Wireless Android Auto Dongle project consists of several components that work together to provide a seamless wireless Android Auto experience. The main components are:

- **Bluetooth Handler**: Manages Bluetooth connections and profiles.
- **USB Manager**: Manages USB gadgets and connections.
- **AAW Proxy**: Forwards data between the phone and the car's head unit.
- **Uevent Monitor**: Monitors uevents and handles USB accessory mode switching.
- **Buildroot**: A tool for building custom embedded Linux systems.

### Bluetooth Handler

The Bluetooth Handler is responsible for managing Bluetooth connections and profiles. It initializes the Bluetooth adapter, sets it to discoverable and pairable mode, and handles connections with the phone.

### USB Manager

The USB Manager manages USB gadgets and connections. It enables and disables USB gadgets, switches between default and accessory gadgets, and waits for accessory mode requests from the phone.

### AAW Proxy

The AAW Proxy forwards data between the phone and the car's head unit. It sets up a TCP server to accept connections from the phone and forwards data between the TCP connection and the USB accessory.

### Uevent Monitor

The Uevent Monitor monitors uevents and handles USB accessory mode switching. It listens for uevents related to USB devices and switches the USB gadget to accessory mode when a request is received.

### Buildroot

Buildroot is a tool for building custom embedded Linux systems. It is used to build the custom Linux image for the Raspberry Pi, including all necessary components and configurations.

## Making Changes or Adding New Features

### Prerequisites

- A computer with a development environment set up (e.g., Docker, Buildroot)
- Basic knowledge of C++ and Linux

### Steps

1. Clone the repository and set up the development environment as described in the [BUILDING.md](../BUILDING.md) file.
2. Make the necessary changes or add new features to the code.
3. Build the project using the instructions in the [BUILDING.md](../BUILDING.md) file.
4. Test the changes on your Raspberry Pi and ensure everything works as expected.
5. Create a pull request with a detailed description of the changes and any relevant information.

## Contributing Guidelines

1. Fork the repository and create a new branch for your changes.
2. Follow the coding style and conventions used in the project.
3. Write clear and concise commit messages.
4. Ensure your changes do not break existing functionality.
5. Test your changes thoroughly before submitting a pull request.
6. Provide a detailed description of the changes in the pull request, including any relevant information or context.

## Support

If you find the project useful, please consider [sponsoring](https://github.com/sponsors/nisargjhaveri) the project. Even a small donation helps. This will help continue fixing issues and getting support for more devices and head units in the future.

Don't forget to star the project on GitHub and spread the word if you think this project might be useful to someone else as well.

## Troubleshooting

### Common Issues

#### Bluetooth and Wi-Fi seem connected, but the phone is stuck at "Looking for Android Auto"

This issue is often caused by a bad USB cable or using the wrong USB port on the device. Ensure:
1. The cable is a good quality data cable and not a power-only cable.
2. You are using the OTG-enabled USB port on the board, not the power-only port.

### Getting Logs

If you encounter issues, you can SSH into the device and retrieve logs for troubleshooting.

1. Connect the device to the head unit and let it boot. Try to connect once. Logs are not persisted across reboots, so you need to retrieve the logs in the same instance soon after observing the issue.
2. Connect to the device using Wi-Fi (SSID: AAWirelessDongle, Password: ConnectAAWirelessDongle, see [hostapd.conf](../aa_wireless_dongle/board/common/rootfs_overlay/etc/hostapd.conf)).
3. SSH into the device (username: root, password: password, see relevant defconfigs, e.g., [raspberrypi0w_defconfig](../aa_wireless_dongle/configs/raspberrypi0w_defconfig)).
4. Check the `/var/log/messages` file for relevant logs. You can also copy the file and attach it to any issues you create.

## License

This project is licensed under the MIT License. See the [LICENSE](../LICENSE) file for more information.
