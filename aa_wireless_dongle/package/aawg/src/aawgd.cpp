#include <stdio.h>
#include <unistd.h>

#include "bluetoothHandler.h"
#include "proxyHandler.h"

int main(void) {
    printf("AA Wireless Dongle\n");

    AAWProxy proxy;
    std::optional<std::thread> proxyThread = proxy.startServer();

    if (!proxyThread) {
        return 1;
    }

    BluetoothHandler bt;
    bt.init();

    proxyThread->join();

    return 0;
}
