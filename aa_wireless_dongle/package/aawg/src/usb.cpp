#include <dirent.h>
#include <string.h>
#include <future>

#include "common.h"
#include "uevent.h"
#include "usb.h"

constexpr const char* defaultGadgetName = "default";
constexpr const char* accessoryGadgetName = "accessory";

/*static*/ std::string UsbManager::s_udcName;

UsbManager& UsbManager::instance() {
    static UsbManager instance;
    return instance;
}

void UsbManager::init() {
    // Init does not actually do anything but provides an intuitive place to cause the constructor call earlier than first use.
}

UsbManager::UsbManager() {
    Logger::instance()->info("Initializing USB Manager\n");

    disableGadget();

    DIR* dirSysClassUdc = opendir("/sys/class/udc/");
    if (dirSysClassUdc == NULL) {
        Logger::instance()->info("USB Manager: Error opening /sys/class/udc/: %s\n", strerror(errno));
        return;
    }
    
    struct dirent* dirEntry = NULL;
    while ((dirEntry = readdir(dirSysClassUdc)) != NULL) {
        if (dirEntry->d_name[0] == '.') {
            continue;
        }

        s_udcName = dirEntry->d_name;
    }

    closedir(dirSysClassUdc);

    if (s_udcName.empty()) {
        Logger::instance()->info("USB Manager: Did not find a valid UDC to use\n");
    } else {
        Logger::instance()->info("USB Manager: Found UDC %s\n", s_udcName.c_str());
    }
}

void UsbManager::writeGadgetFile(std::string gadgetName, std::string relativeFilePath, const char* content) {
    std::string gadgetFilePath = "/sys/kernel/config/usb_gadget/" + gadgetName + "/" + relativeFilePath;
    FILE* gadgetFile = fopen(gadgetFilePath.c_str(), "w");
    fputs(content, gadgetFile);
    fputc('\n', gadgetFile);
    fclose(gadgetFile);
}

void UsbManager::enableGadget(std::string gadgetName) {
    writeGadgetFile(gadgetName, "UDC", s_udcName.c_str());
}

void UsbManager::disableGadget(std::string gadgetName) {
    writeGadgetFile(gadgetName, "UDC", "");
}

void UsbManager::switchToAccessoryGadget() {
    disableGadget(defaultGadgetName);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 0.1 second, keep the gadget disabled for a short time to let the host recognize the change
    enableGadget(accessoryGadgetName);

    Logger::instance()->info("USB Manager: Switched to accessory gadget from default\n");
}

void UsbManager::disableGadget() {
    disableGadget(defaultGadgetName);
    disableGadget(accessoryGadgetName);

    Logger::instance()->info("USB Manager: Disabled all USB gadgets\n");
}

bool UsbManager::enableDefaultAndWaitForAccessory(std::chrono::milliseconds timeout) {
    std::shared_ptr<std::promise<void>> accessoryPromise = std::make_shared<std::promise<void>>();
    std::weak_ptr<std::promise<void>> accessoryPromiseWeak = accessoryPromise;

    UeventMonitor::instance().addHandler([accessoryPromiseWeak](UeventEnv env) {
        std::shared_ptr<std::promise<void>> accessoryPromise = accessoryPromiseWeak.lock();

        // If the promise is no longer active, nothing to do.
        if (!accessoryPromise) {
            return true;
        }

        if (auto it = env.find("DEVNAME"); it == env.end() || it->second != "usb_accessory") {
            return false;
        }

        if (auto it = env.find("ACCESSORY"); it == env.end() || it->second != "START") {
            return false;
        }

        // Got an accessory start event
        Logger::instance()->info("USB Manager: Received accessory start request\n");
        UsbManager::instance().switchToAccessoryGadget();
        accessoryPromise->set_value();

        return true;
    });

    enableGadget(defaultGadgetName);

    Logger::instance()->info("USB Manager: Enabled default gadget\n");

    if (timeout == std::chrono::milliseconds(0)) {
        accessoryPromise->get_future().wait();
        return true;
    } else {
        std::future_status status = accessoryPromise->get_future().wait_for(timeout);

        if (status == std::future_status::ready) {
            return true;
        } else {
            Logger::instance()->info("USB Manager: Timeout waiting for accessory start request\n");
            return false;
        }
    }
}
