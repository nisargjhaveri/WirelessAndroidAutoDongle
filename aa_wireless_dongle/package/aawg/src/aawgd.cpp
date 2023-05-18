#include <stdio.h>
#include <unistd.h>

#include "bluetoothHandler.h"

int main(void) {
    printf("AA Wireless Dongle\n");

    BluetoothHandler bt;
    bt.init();

    // Run infinitely
    while (true) {
        sleep(2);
    }

    return 0;
}
