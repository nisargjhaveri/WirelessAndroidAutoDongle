#include <stdio.h>

#include "common.h"
#include "bluetoothHandler.h"
#include "bluetoothProfiles.h"
#include "bluetoothAdvertisement.h"

static constexpr const char* ADAPTER_ALIAS_PREFIX = "WirelessAADongle-";
static constexpr const char* ADAPTER_ALIAS_DONGLE_PREFIX = "AndroidAuto-Dongle-";

static constexpr const char* BLUEZ_BUS_NAME = "org.bluez";
static constexpr const char* BLUEZ_ROOT_OBJECT_PATH = "/";
static constexpr const char* BLUEZ_OBJECT_PATH = "/org/bluez";

static constexpr const char* INTERFACE_BLUEZ_ADAPTER = "org.bluez.Adapter1";
static constexpr const char* INTERFACE_BLUEZ_LE_ADVERTISING_MANAGER = "org.bluez.LEAdvertisingManager1";

static constexpr const char* INTERFACE_BLUEZ_DEVICE = "org.bluez.Device1";
static constexpr const char* INTERFACE_BLUEZ_PROFILE_MANAGER = "org.bluez.ProfileManager1";

static constexpr const char* LE_ADVERTISEMENT_OBJECT_PATH = "/com/aawgd/bluetooth/advertisement";

static constexpr const char* AAWG_PROFILE_OBJECT_PATH = "/com/aawgd/bluetooth/aawg";
static constexpr const char* AAWG_PROFILE_UUID = "4de17a00-52cb-11e6-bdf4-0800200c9a66";

static constexpr const char* HSP_HS_PROFILE_OBJECT_PATH = "/com/aawgd/bluetooth/hsp";
static constexpr const char* HSP_AG_UUID = "00001112-0000-1000-8000-00805f9b34fb";
static constexpr const char* HSP_HS_UUID = "00001108-0000-1000-8000-00805f9b34fb";

// D-Bus Property Keys
static constexpr const char* DBUS_PROPERTY_NAME = "Name";
static constexpr const char* DBUS_PROPERTY_ROLE = "Role";
static constexpr const char* DBUS_PROPERTY_CHANNEL = "Channel";
static constexpr const char* DBUS_PROPERTY_TYPE = "Type"; // Used by BLEAdvertisement object, not directly in Properties map here
static constexpr const char* DBUS_PROPERTY_SERVICE_UUIDS = "ServiceUUIDs"; // Used by BLEAdvertisement object
static constexpr const char* DBUS_PROPERTY_LOCAL_NAME = "LocalName"; // Used by BLEAdvertisement object
static constexpr const char* DBUS_PROPERTY_CONNECTED = "Connected"; // Used for Device1

// D-Bus Property Values
static constexpr const char* PROFILE_ROLE_SERVER = "server";
static constexpr const char* ADVERTISEMENT_TYPE_PERIPHERAL = "peripheral";
static constexpr const char* PROFILE_NAME_AA_WIRELESS = "AA Wireless";
static constexpr const char* PROFILE_NAME_HSP_HS = "HSP HS";

// D-Bus ObjectManager Interface and Method
static constexpr const char* DBUS_INTERFACE_OBJECT_MANAGER = "org.freedesktop.DBus.ObjectManager";
static constexpr const char* DBUS_METHOD_GET_MANAGED_OBJECTS = "GetManagedObjects";


class BluezAdapterProxy: private DBus::ObjectProxy {
    BluezAdapterProxy(std::shared_ptr<DBus::Connection> conn, DBus::Path path): DBus::ObjectProxy(conn, BLUEZ_BUS_NAME, path) {
        alias = this->create_property<std::string>(INTERFACE_BLUEZ_ADAPTER, "Alias");
        powered = this->create_property<bool>(INTERFACE_BLUEZ_ADAPTER, "Powered");
        discoverable = this->create_property<bool>(INTERFACE_BLUEZ_ADAPTER, "Discoverable");
        pairable = this->create_property<bool>(INTERFACE_BLUEZ_ADAPTER, "Pairable");

        registerAdvertisement = this->create_method<void(DBus::Path, DBus::Properties)>(INTERFACE_BLUEZ_LE_ADVERTISING_MANAGER, "RegisterAdvertisement");
        unregisterAdvertisement = this->create_method<void(DBus::Path)>(INTERFACE_BLUEZ_LE_ADVERTISING_MANAGER, "UnregisterAdvertisement");
    }

public:
    static std::shared_ptr<BluezAdapterProxy> create(std::shared_ptr<DBus::Connection> conn, DBus::Path path)
    {
      return std::shared_ptr<BluezAdapterProxy>(new BluezAdapterProxy(conn, path));
    }

    std::shared_ptr<DBus::PropertyProxy<std::string>> alias;
    std::shared_ptr<DBus::PropertyProxy<bool>> powered;
    std::shared_ptr<DBus::PropertyProxy<bool>> discoverable;
    std::shared_ptr<DBus::PropertyProxy<bool>> pairable;

    std::shared_ptr<DBus::MethodProxy<void(DBus::Path, DBus::Properties)>> registerAdvertisement;
    std::shared_ptr<DBus::MethodProxy<void(DBus::Path)>> unregisterAdvertisement;
};


BluetoothHandler& BluetoothHandler::instance() {
    static BluetoothHandler instance;
    return instance;
}

DBus::ManagedObjects BluetoothHandler::getBluezObjects() {
    std::shared_ptr<DBus::ObjectProxy> m_bluezRootObject = m_connection->create_object_proxy(BLUEZ_BUS_NAME, BLUEZ_ROOT_OBJECT_PATH);
    DBus::MethodProxy getManagedObjects = *(m_bluezRootObject->create_method<DBus::ManagedObjects(void)>(DBUS_INTERFACE_OBJECT_MANAGER, DBUS_METHOD_GET_MANAGED_OBJECTS));

    return getManagedObjects();
}

// Initializes the Bluetooth adapter.
// This method finds the primary Bluetooth adapter available on the system via D-Bus,
// then creates a proxy object for it and sets its alias (the name visible to other devices).
void BluetoothHandler::initAdapter() {
    // Retrieve all D-Bus managed objects from BlueZ.
    // BlueZ uses the D-Bus ObjectManager interface to expose all its objects (adapters, devices, etc.)
    // and their properties in a single method call.
    DBus::ManagedObjects objects = getBluezObjects();

    std::string adapter_path; // Will store the D-Bus object path of the found adapter (e.g., "/org/bluez/hci0").
    // Iterate over all D-Bus objects provided by BlueZ.
    for (auto const& [path, interfaces]: objects) {
        // Each object (identified by its 'path') can have multiple D-Bus 'interfaces'.
        for (auto const& [interface, properties]: interfaces) {
            // We are looking for an object that implements the 'org.bluez.Adapter1' interface.
            if (interface == INTERFACE_BLUEZ_ADAPTER) {
                adapter_path = path; // Found the adapter's D-Bus path.
                Logger::instance()->info("Using bluetooth adapter at path: %s\n", path.c_str());
                break; // Stop searching interfaces for this object.
            }
        }
        if (!adapter_path.empty()) {
            break; // Stop searching objects as we've found our adapter.
        }
    }

    if (adapter_path.empty()) {
        // This is a critical issue if no adapter is found, as Bluetooth functionality will be unavailable.
        Logger::instance()->info("Did not find any bluetooth adapters\n");
    }
    else {
        // Create a proxy object for the Bluetooth adapter.
        // This proxy (BluezAdapterProxy) simplifies interaction with the adapter's D-Bus methods and properties.
        m_adapter = BluezAdapterProxy::create(m_connection, adapter_path);
        // Set the "Alias" D-Bus property of the adapter.
        // This is the human-readable name that remote Bluetooth devices will see.
        m_adapter->alias->set_value(m_adapterAlias);
        Logger::instance()->info("Bluetooth adapter alias: %s\n", m_adapterAlias.c_str());
    }
}

// Registers custom Bluetooth profiles (AAWireless and HSP Headset) with the BlueZ daemon.
// This involves:
// 1. Creating D-Bus object instances for our custom profile handlers (AAWirelessProfile, HSPHSProfile).
// 2. Registering these D-Bus objects with our D-Bus connection, so BlueZ can call methods on them.
// 3. Calling the `RegisterProfile` method on BlueZ's `org.bluez.ProfileManager1` interface
//    to inform BlueZ about these profiles, their UUIDs, and options.
void BluetoothHandler::exportProfiles() {
    // Create a D-Bus proxy for the main BlueZ object (/org/bluez) to access the ProfileManager1 interface.
    std::shared_ptr<DBus::ObjectProxy> bluezObject = m_connection->create_object_proxy(BLUEZ_BUS_NAME, BLUEZ_OBJECT_PATH);
    // Create a method proxy for the RegisterProfile method.
    DBus::MethodProxy registerProfile = *(bluezObject->create_method<void(DBus::Path, std::string, DBus::Properties)>(INTERFACE_BLUEZ_PROFILE_MANAGER, "RegisterProfile"));

    // Register AA Wireless Profile
    // This profile is custom for Android Auto Wireless functionality.
    // 1. Create an instance of our AAWirelessProfile D-Bus object.
    //    This object implements methods like NewConnection, RequestDisconnection, Release,
    //    which BlueZ will call when a device connects or disconnects using this profile.
    m_aawProfile = AAWirelessProfile::create(AAWG_PROFILE_OBJECT_PATH);
    // 2. Register our AAWirelessProfile object with our D-Bus connection.
    //    This makes its methods callable by BlueZ via the D-Bus path AAWG_PROFILE_OBJECT_PATH.
    if (m_connection->register_object(m_aawProfile, DBus::ThreadForCalling::DispatcherThread) != DBus::RegistrationStatus::Success) {
        Logger::instance()->info("Failed to register AA Wireless profile D-Bus object\n");
    }

    // 3. Call BlueZ's RegisterProfile method to make BlueZ aware of the AA Wireless profile.
    //    - AAWG_PROFILE_OBJECT_PATH: The D-Bus path where our profile handler is registered.
    //    - AAWG_PROFILE_UUID: The unique UUID for the AA Wireless service.
    //    - Options: A map of properties for the profile.
    //        - "Name": Human-readable name.
    //        - "Role": "server" indicates this device provides the service.
    //        - "Channel": RFCOMM channel for SPP-like communication (if applicable).
    registerProfile(AAWG_PROFILE_OBJECT_PATH, AAWG_PROFILE_UUID, {
        {DBUS_PROPERTY_NAME, DBus::Variant(PROFILE_NAME_AA_WIRELESS)},
        {DBUS_PROPERTY_ROLE, DBus::Variant(PROFILE_ROLE_SERVER)},
        {DBUS_PROPERTY_CHANNEL, DBus::Variant(uint16_t(8))}, // Specific RFCOMM channel for AAW
    });
    Logger::instance()->info("Bluetooth AA Wireless profile active\n");

    // Conditionally register the Hands-Free Profile (HSP) Headset role.
    // This is typically not needed if the device is purely in "dongle mode" (acting as a peripheral for one specific head unit).
    if (Config::instance()->getConnectionStrategy() != ConnectionStrategy::DONGLE_MODE) {
        // Register HSP Handset (HS) profile. This allows the device to act as a HSP headset.
        // 1. Create an instance of our HSPHSProfile D-Bus object.
        m_hspProfile = HSPHSProfile::create(HSP_HS_PROFILE_OBJECT_PATH);
        // 2. Register our HSPHSProfile object with our D-Bus connection.
        if (m_connection->register_object(m_hspProfile, DBus::ThreadForCalling::DispatcherThread) != DBus::RegistrationStatus::Success) {
            Logger::instance()->info("Failed to register HSP Handset profile D-Bus object\n");
        }
        // 3. Call BlueZ's RegisterProfile for the HSP HS role.
        //    - HSP_HS_PROFILE_OBJECT_PATH: D-Bus path for our HSP profile handler.
        //    - HSP_HS_UUID: Standard UUID for HSP Headset Service.
        //    - Options:
        //        - "Name": Human-readable name.
        registerProfile(HSP_HS_PROFILE_OBJECT_PATH, HSP_HS_UUID, {
            {DBUS_PROPERTY_NAME, DBus::Variant(PROFILE_NAME_HSP_HS)},
        });
        Logger::instance()->info("HSP Handset profile active\n");
    }
}

// Starts Bluetooth Low Energy (BLE) advertising.
// This involves:
// 1. Creating a BLEAdvertisement D-Bus object which defines the advertisement data.
// 2. Registering this D-Bus object with our D-Bus connection.
// 3. Calling `RegisterAdvertisement` on BlueZ's `org.bluez.LEAdvertisingManager1` interface.
void BluetoothHandler::startAdvertising() {
    if (!m_adapter) {
        Logger::instance()->info("Cannot start advertising: Bluetooth adapter not available.\n");
        return;
    }

    // 1. Create an instance of our BLEAdvertisement D-Bus object.
    // This object holds the properties of the advertisement, like type, service UUIDs, and local name.
    m_leAdvertisement = BLEAdvertisement::create(LE_ADVERTISEMENT_OBJECT_PATH);

    // Set advertisement properties:
    // - Type: "peripheral" indicates that the device is connectable and advertises services.
    // - ServiceUUIDs: A list of UUIDs that this device advertises. Here, the AAWireless profile UUID.
    // - LocalName: The name to be shown to scanning devices.
    m_leAdvertisement->type->set_value(ADVERTISEMENT_TYPE_PERIPHERAL);
    m_leAdvertisement->serviceUUIDs->set_value(std::vector<std::string>{AAWG_PROFILE_UUID});
    m_leAdvertisement->localName->set_value(m_adapterAlias);

    // 2. Register our BLEAdvertisement object with our D-Bus connection.
    // This makes its D-Bus path (LE_ADVERTISEMENT_OBJECT_PATH) and properties readable by BlueZ.
    if (m_connection->register_object(m_leAdvertisement, DBus::ThreadForCalling::DispatcherThread) != DBus::RegistrationStatus::Success) {
        Logger::instance()->info("Failed to register BLE Advertisement D-Bus object\n");
        return; // Cannot proceed if D-Bus object registration fails.
    }

    // 3. Call BlueZ's RegisterAdvertisement method.
    // This tells BlueZ to start advertising using the data provided by our D-Bus object
    // at LE_ADVERTISEMENT_OBJECT_PATH.
    // The second argument, an empty DBus::Properties map, means no special options are passed for the registration itself.
    // The actual advertisement data is read by BlueZ from our m_leAdvertisement object's properties.
    (*m_adapter->registerAdvertisement)(LE_ADVERTISEMENT_OBJECT_PATH, {});
    Logger::instance()->info("BLE Advertisement started\n");
}

// Stops Bluetooth Low Energy (BLE) advertising.
// This involves calling `UnregisterAdvertisement` on BlueZ's `org.bluez.LEAdvertisingManager1` interface.
void BluetoothHandler::stopAdvertising() {
    if (!m_adapter) {
        Logger::instance()->info("Cannot stop advertising: Bluetooth adapter not available.\n");
        return;
    }

    // Call BlueZ's UnregisterAdvertisement method, passing the D-Bus path of the advertisement
    // that was previously registered. This tells BlueZ to stop this specific advertisement.
    (*m_adapter->unregisterAdvertisement)(LE_ADVERTISEMENT_OBJECT_PATH);
    Logger::instance()->info("BLE Advertisement stopped\n");
}

void BluetoothHandler::setPower(bool on) {
    if (!m_adapter) {
        return;
    }

    m_adapter->powered->set_value(on);
    Logger::instance()->info("Bluetooth adapter was powered %s\n", on ? "on" : "off");
}

void BluetoothHandler::setPairable(bool pairable) {
    if (!m_adapter) {
        return;
    }

    m_adapter->discoverable->set_value(pairable);
    m_adapter->pairable->set_value(pairable);
    Logger::instance()->info("Bluetooth adapter is now discoverable and pairable\n");
}

// Attempts to establish a Bluetooth connection to a (presumably paired) device.
// It iterates through known Bluetooth devices managed by BlueZ and tries to connect
// to a specific profile on them.
void BluetoothHandler::connectDevice() {
    // Fetch all D-Bus managed objects from BlueZ to find device paths.
    DBus::ManagedObjects objects = getBluezObjects();

    std::vector<std::string> device_paths; // To store D-Bus paths of found Bluetooth devices.
    // Iterate through all D-Bus objects to find those representing Bluetooth devices.
    for (auto const& [path, interfaces]: objects) {
        // Each object can implement multiple D-Bus interfaces.
        for (auto const& [interface, properties]: interfaces) {
            // We are looking for objects implementing the org.bluez.Device1 interface.
            if (interface == INTERFACE_BLUEZ_DEVICE) {
                device_paths.push_back(path); // Store the D-Bus path of the device.
            }
        }
    }

    if (device_paths.empty()) { // Corrected from !device_paths.size() to .empty() for clarity
        Logger::instance()->info("Did not find any bluetooth devices\n"); // Corrected log message
        return;
    }

    const bool isDongleMode = (Config::instance()->getConnectionStrategy() == ConnectionStrategy::DONGLE_MODE);

    Logger::instance()->info("Found %zu bluetooth devices\n", device_paths.size()); // Use %zu for size_t

    for (const std::string &device_path: device_paths) {
        Logger::instance()->info("Trying to connect bluetooth device at path: %s\n", device_path.c_str());

        // Create a D-Bus proxy for the specific Bluetooth device.
        std::shared_ptr<DBus::ObjectProxy> bluezDevice = m_connection->create_object_proxy(BLUEZ_BUS_NAME, device_path);
        // Create method proxies for ConnectProfile and Disconnect methods of org.bluez.Device1.
        DBus::MethodProxy connectProfile = *(bluezDevice->create_method<void(std::string)>(INTERFACE_BLUEZ_DEVICE, "ConnectProfile"));
        DBus::MethodProxy disconnect = *(bluezDevice->create_method<void()>(INTERFACE_BLUEZ_DEVICE, "Disconnect"));
        // Create a property proxy for the "Connected" property of org.bluez.Device1.
        std::shared_ptr<DBus::PropertyProxy<bool>> deviceConnected = bluezDevice->create_property<bool>(INTERFACE_BLUEZ_DEVICE, DBUS_PROPERTY_CONNECTED);

        try {
            // If the device is already connected, disconnect it first to ensure a clean state.
            // This might be necessary if a previous connection didn't terminate cleanly.
            if (deviceConnected && deviceConnected->get_value()) { // Check if proxy and value are valid
                Logger::instance()->info("Bluetooth device already connected, disconnecting first\n");
                disconnect(); // Call Disconnect D-Bus method.
                // It might be good to wait a bit here for disconnection to complete.
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Brief pause
            }

            // Attempt to connect to a specific Bluetooth profile.
            // In DongleMode, it might connect to a default profile (empty string for UUID).
            // Otherwise, it attempts to connect to the HSP Audio Gateway (AG) profile.
            connectProfile(isDongleMode ? "" : HSP_AG_UUID);
            Logger::instance()->info("Bluetooth connection attempt successful to device: %s\n", device_path.c_str());

            // If not in dongle mode, one successful connection is enough.
            if (!isDongleMode) {
                return; // Exit after the first successful connection.
            }
        } catch (DBus::Error& e) {
            // Log connection errors, but only if not in dongle mode,
            // as dongle mode might try multiple devices or profiles.
            if (!isDongleMode) {
                Logger::instance()->info("Failed to connect to device at path: %s, Error: %s\n", device_path.c_str(), e.what());
            }
        }
    }

    // If not in dongle mode and loop finishes, it means no device connected successfully.
    if (!isDongleMode) {
        Logger::instance()->info("Failed to connect to any known bluetooth device after trying all.\n");
    }
}

// Manages a loop that periodically calls `connectDevice` to attempt Bluetooth connections.
// This is useful for retrying connections in the background until successful or explicitly stopped.
void BluetoothHandler::retryConnectLoop() {
    bool should_exit = false;
    // `connectWithRetryPromise` is a std::promise whose future is waited upon.
    // Setting a value on this promise will cause the wait to terminate, effectively stopping the loop.
    std::future<void> connectWithRetryFuture = connectWithRetryPromise->get_future();

    while (!should_exit) {
        connectDevice(); // Attempt to connect to devices.

        // Wait on the future associated with `connectWithRetryPromise`.
        // This wait will be interrupted if `stopConnectWithRetry` is called (which sets the promise value).
        // It also has a timeout of 20 seconds, after which it will loop again if the promise wasn't set.
        if (connectWithRetryFuture.wait_for(std::chrono::seconds(20)) == std::future_status::ready) {
            // The promise was set (likely by `stopConnectWithRetry`), so exit the loop.
            should_exit = true;
            connectWithRetryPromise = nullptr; // Reset the promise.
        }
        // If wait_for timed out, the loop continues, and connectDevice() is called again.
    }

    // If not in dongle mode, power off Bluetooth adapter after the retry loop finishes.
    // This is a cleanup action if connection attempts are abandoned.
    if (Config::instance()->getConnectionStrategy() != ConnectionStrategy::DONGLE_MODE) {
        BluetoothHandler::instance().powerOff();
    }
}

void BluetoothHandler::init() {
    // DBus::set_logging_function( DBus::log_std_err );
    // DBus::set_log_level( SL_TRACE );

    m_dispatcher = DBus::StandaloneDispatcher::create();
    m_connection = m_dispatcher->create_connection( DBus::BusType::SYSTEM );

    std::string adapterAliasPrefix = (Config::instance()->getConnectionStrategy() == ConnectionStrategy::DONGLE_MODE) ? ADAPTER_ALIAS_DONGLE_PREFIX : ADAPTER_ALIAS_PREFIX;

    m_adapterAlias = adapterAliasPrefix + Config::instance()->getUniqueSuffix();

    initAdapter();
    exportProfiles();
}

void BluetoothHandler::powerOn() {
    if (!m_adapter) {
        return;
    }

    setPower(true);
    setPairable(true);

    if (Config::instance()->getConnectionStrategy() == ConnectionStrategy::DONGLE_MODE) {
        startAdvertising();
    }
}

std::optional<std::thread> BluetoothHandler::connectWithRetry() {
    if (!m_adapter) {
        return std::nullopt;
    }

    connectWithRetryPromise = std::make_shared<std::promise<void>>();
    return std::thread(&BluetoothHandler::retryConnectLoop, this);
}

void BluetoothHandler::stopConnectWithRetry() {
    if (connectWithRetryPromise) {
        connectWithRetryPromise->set_value();
    }
}

void BluetoothHandler::powerOff() {
    if (!m_adapter) {
        return;
    }

    if (Config::instance()->getConnectionStrategy() == ConnectionStrategy::DONGLE_MODE) {
        stopAdvertising();
    }
    setPower(false);
}