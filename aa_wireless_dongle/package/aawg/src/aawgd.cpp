#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "bluetoothHandler.h"
#include "proxyHandler.h"
#include "uevent.h"
#include "usb.h"

int main(void) {
    Logger::instance()->info("AA Wireless Dongle\n");

    // Global init
    std::optional<std::thread> ueventThread =  UeventMonitor::instance().start();
    UsbManager::instance().init();
    BluetoothHandler::instance().init();

    while (true) {
        // Per connection setup and processing
        UsbManager::instance().enableDefaultAndWaitForAccessroy();

        AAWProxy proxy;
        std::optional<std::thread> proxyThread = proxy.startServer(Config::instance()->getWifiInfo().port);

        if (!proxyThread) {
            return 1;
        }

        BluetoothHandler::instance().connect();

        proxyThread->join();
        BluetoothHandler::instance().powerOff();
        UsbManager::instance().disableGadget();

        // sleep for a couple of seconds before retrying
        sleep(2);
    }

    ueventThread->join();

    return 0;
}
