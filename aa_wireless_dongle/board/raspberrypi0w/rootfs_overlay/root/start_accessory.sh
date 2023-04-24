#!/bin/sh

cd /sys/kernel/config/usb_gadget

# Assume default is active
UDC=$(cat default/UDC)

# Switch to accessory if we get an accessory start request
if [ "$UDC" != "" -a "$DEVNAME" = "usb_accessory" -a "$ACCESSORY" = "START" ]; then
    echo "" > default/UDC
    echo "$UDC" > accessory/UDC
    logger -s -p INFO -t start_accessory START: Switched to accessory gadget from default
fi
