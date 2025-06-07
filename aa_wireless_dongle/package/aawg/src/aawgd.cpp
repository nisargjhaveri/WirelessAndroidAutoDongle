#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "bluetoothHandler.h"
#include "proxyHandler.h"
#include "uevent.h"
#include "usb.h"
#include "webServer.h" // Added WebServer header

int main(void) {
    Logger::instance()->info("AA Wireless Dongle\n");

    // Global init
    std::optional<std::thread> ueventThread =  UeventMonitor::instance().start();
    UsbManager::instance().init();
    BluetoothHandler::instance().init();

    // Initialize and start the WebServer
    // Port 8080 is a common alternative HTTP port.
    // The WebServer constructor attempts to start the server.
    // Its destructor will attempt to stop it.
    WebServer webUi(8080);

    ConnectionStrategy connectionStrategy = Config::instance()->getConnectionStrategy();
    if (connectionStrategy == ConnectionStrategy::DONGLE_MODE) {
        BluetoothHandler::instance().powerOn();
    }

    while (true) {
        Logger::instance()->info("Connection Strategy: %d\n", connectionStrategy);

        // Per connection setup and processing
        if (connectionStrategy == ConnectionStrategy::USB_FIRST) {
            Logger::instance()->info("Waiting for the accessory to connect first\n");
            UsbManager::instance().enableDefaultAndWaitForAccessory();
        }

        AAWProxy proxy;
        std::optional<std::thread> proxyThread = proxy.startServer(Config::instance()->getWifiInfo().port);

        if (!proxyThread) {
            return 1;
        }

        if (connectionStrategy != ConnectionStrategy::DONGLE_MODE) {
            BluetoothHandler::instance().powerOn();
        }

        std::optional<std::thread> btConnectionThread = BluetoothHandler::instance().connectWithRetry();

        proxyThread->join();

        if (btConnectionThread) {
            BluetoothHandler::instance().stopConnectWithRetry();
            btConnectionThread->join();
        }

        UsbManager::instance().disableGadget();

        if (connectionStrategy != ConnectionStrategy::DONGLE_MODE) {
            // sleep for a couple of seconds before retrying
            sleep(2);
        }
    }

    ueventThread->join();

    return 0;
}
