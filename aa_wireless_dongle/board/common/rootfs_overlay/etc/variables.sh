# Uncomment this setting to wait for the usb to connect first.
# By default, we try to connect and wait for the phone to connect first regardless of the usb connection.
# export AAWG_CONNECTION_WAIT_FOR_ACCESSORY=1

# Gets the MAC address from the BT and creates the variable.
# xx=$(ls /var/lib/bluetooth/ | awk -F':' '{print $4$5$6 }' | tr [[:upper:]] [[:lower:]] | tr -d '\n')
# export ADAPTER_ALIAS=AndroidAuto_$xx