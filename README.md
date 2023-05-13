# AAWirelessGatewayDongle

## Setup

### Start the vagrant box
```
$ git clone <repository-url>
$ vagrant up
$ vagrant ssh
```

### Setup buildroot environment (inside the VM)
```
$ git clone --recurse-submodules <repository-url>
$ cd AAWirelessGatewayDongle/buildroot
$ make BR2_EXTERNAL=../aa_wireless_dongle/ list-defconfigs
$ make raspberrypi0w_defconfig
$ make
```
