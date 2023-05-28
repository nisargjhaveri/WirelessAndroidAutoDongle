#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "bluetoothHandler.h"
#include "proxyHandler.h"

int main(void) {
    printf("AA Wireless Dongle\n");

    AAWProxy proxy;
    std::optional<std::thread> proxyThread = proxy.startServer(Config::instance()->getWifiInfo().port);

    if (!proxyThread) {
        return 1;
    }

    BluetoothHandler bt;
    bt.init();

    proxyThread->join();

    return 0;
}
