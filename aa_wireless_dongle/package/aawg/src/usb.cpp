#include <dirent.h>
#include <string.h>
#include <future>
#include <stdexcept>

#include "common.h"
#include "uevent.h"
#include "usb.h"

// String constants for USB gadget management and Uevent handling
static constexpr const char* DEFAULT_GADGET_NAME = "default";
static constexpr const char* ACCESSORY_GADGET_NAME = "accessory";
static constexpr const char* SYS_CLASS_UDC_PATH = "/sys/class/udc/";
static constexpr const char* SYS_KERNEL_CONFIG_USB_GADGET_PATH = "/sys/kernel/config/usb_gadget/";
static constexpr const char* UDC_FILE_NAME = "UDC";
static constexpr const char* USB_ACCESSORY_DEVNAME = "usb_accessory";
static constexpr const char* UEVENT_DEVNAME_KEY = "DEVNAME";
static constexpr const char* UEVENT_ACCESSORY_KEY = "ACCESSORY";
static constexpr const char* UEVENT_ACCESSORY_START_VALUE = "START";

/*static*/ std::string UsbManager::s_udcName;

UsbManager& UsbManager::instance() {
    static UsbManager instance;
    return instance;
}

void UsbManager::init() {
    // Init does not actually do anything but provides an intuitive place to cause the constructor call earlier than first use.
}

// Constructor for UsbManager
// Initializes the USB configuration, primarily by identifying the available USB Device Controller (UDC).
UsbManager::UsbManager() {
    Logger::instance()->info("Initializing USB Manager\n");

    // Ensure any existing gadget configurations are cleared initially.
    disableGadget();

    // Open the directory that lists available UDCs.
    // UDCs are hardware components that allow the system to act as a USB device.
    DIR* dirSysClassUdc = opendir(SYS_CLASS_UDC_PATH);
    if (dirSysClassUdc == NULL) {
        throw std::runtime_error("USB Manager: Error opening " + std::string(SYS_CLASS_UDC_PATH) + ": " + std::string(strerror(errno)));
    }
    
    struct dirent* dirEntry = NULL;
    // Iterate through the entries in /sys/class/udc/ to find a UDC name.
    // The first valid entry (not "." or "..") is typically used.
    while ((dirEntry = readdir(dirSysClassUdc)) != NULL) {
        if (dirEntry->d_name[0] == '.') {
            continue;
        }

        s_udcName = dirEntry->d_name; // Store the name of the found UDC.
    }

    closedir(dirSysClassUdc);

    if (s_udcName.empty()) {
        // This is problematic as a UDC is required for USB gadget functionality.
        Logger::instance()->info("USB Manager: Did not find a valid UDC to use\n");
    } else {
        Logger::instance()->info("USB Manager: Found UDC %s\n", s_udcName.c_str());
    }
}

// Helper function to write content to a specific file within a USB gadget's configuration directory.
// gadgetName: The name of the USB gadget (e.g., "default", "accessory").
// relativeFilePath: The path of the file within the gadget's directory (e.g., "UDC", "bDeviceClass").
// content: The string to write to the file.
void UsbManager::writeGadgetFile(std::string gadgetName, std::string relativeFilePath, const char* content) {
    std::string gadgetFilePath = std::string(SYS_KERNEL_CONFIG_USB_GADGET_PATH) + gadgetName + "/" + relativeFilePath;
    FILE* gadgetFile = fopen(gadgetFilePath.c_str(), "w");
    fputs(content, gadgetFile);
    fputc('\n', gadgetFile);
    fclose(gadgetFile);
}

// Enables a specific USB gadget by writing the UDC name to its "UDC" file.
// This binds the gadget configuration to the actual USB Device Controller.
void UsbManager::enableGadget(std::string gadgetName) {
    // Writing the UDC name to the gadget's UDC file activates it.
    writeGadgetFile(gadgetName, UDC_FILE_NAME, s_udcName.c_str());
}

// Disables a specific USB gadget by writing an empty string to its "UDC" file.
// This unbinds the gadget from the UDC.
void UsbManager::disableGadget(std::string gadgetName) {
    // Writing an empty string to the gadget's UDC file deactivates it.
    writeGadgetFile(gadgetName, UDC_FILE_NAME, "");
}

// Switches the active USB gadget from the 'default' to the 'accessory' gadget.
// The 'default' gadget is typically a simple one (e.g., MTP or RNDIS).
// The 'accessory' gadget is the Android Open Accessory (AOA) gadget.
void UsbManager::switchToAccessoryGadget() {
    disableGadget(DEFAULT_GADGET_NAME); // Disable the currently active default gadget.
    // A short delay can be important for the USB host to recognize the device detachment
    // before the new gadget (accessory) is enabled.
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 0.1 second
    enableGadget(ACCESSORY_GADGET_NAME); // Enable the accessory gadget.

    Logger::instance()->info("USB Manager: Switched to accessory gadget from default\n");
}

// Disables all known USB gadgets (both 'default' and 'accessory').
// This effectively disconnects the device from the USB host's perspective.
void UsbManager::disableGadget() {
    disableGadget(DEFAULT_GADGET_NAME);
    disableGadget(ACCESSORY_GADGET_NAME);

    Logger::instance()->info("USB Manager: Disabled all USB gadgets\n");
}

// Enables the 'default' USB gadget and then waits for a uevent indicating that
// the Android Accessory protocol has started. Once the event is received,
// it switches to the 'accessory' gadget.
// timeout: Maximum time to wait for the accessory event.
bool UsbManager::enableDefaultAndWaitForAccessory(std::chrono::milliseconds timeout) {
    // A promise is used to signal the arrival of the accessory uevent.
    std::shared_ptr<std::promise<void>> accessoryPromise = std::make_shared<std::promise<void>>();
    std::weak_ptr<std::promise<void>> accessoryPromiseWeak = accessoryPromise;

    UeventMonitor::instance().addHandler([accessoryPromiseWeak](UeventEnv env) {
        std::shared_ptr<std::promise<void>> accessoryPromise = accessoryPromiseWeak.lock();

        // If the promise is no longer active (e.g., timeout occurred), nothing to do.
        if (!accessoryPromise) {
            return true; // Remove this handler from the UeventMonitor.
        }

        // Check if the uevent is for the "usb_accessory" device.
        if (auto it = env.find(UEVENT_DEVNAME_KEY); it == env.end() || it->second != USB_ACCESSORY_DEVNAME) {
            return false; // Not the event we're looking for, keep handler active.
        }

        // Check if the uevent indicates that the accessory has started.
        if (auto it = env.find(UEVENT_ACCESSORY_KEY); it == env.end() || it->second != UEVENT_ACCESSORY_START_VALUE) {
            return false; // Not the "START" event, keep handler active.
        }

        // Correct uevent received: Android Accessory mode has been initiated by the host.
        Logger::instance()->info("USB Manager: Received accessory start request\n");
        // Switch from the default gadget to the accessory gadget.
        UsbManager::instance().switchToAccessoryGadget();
        // Fulfill the promise to signal that the switch has occurred.
        accessoryPromise->set_value();

        return true; // Remove this handler from the UeventMonitor.
    });

    // Enable the 'default' gadget. This is the initial state before AOA negotiation.
    // The host (e.g., car head unit) will interact with this gadget and
    // may then send control requests to switch to accessory mode.
    enableGadget(DEFAULT_GADGET_NAME);

    Logger::instance()->info("USB Manager: Enabled default gadget\n");

    // Wait for the accessory uevent.
    if (timeout == std::chrono::milliseconds(0)) {
        // Wait indefinitely if timeout is zero.
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
