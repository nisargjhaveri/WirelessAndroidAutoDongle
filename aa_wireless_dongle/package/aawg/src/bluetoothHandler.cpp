#include <stdio.h>

#include "common.h"
#include "bluetoothHandler.h"
#include "bluetoothProfiles.h"

static constexpr const char* ADAPTER_ALIAS = "AA Wireless Dongle";

static constexpr const char* BLUEZ_BUS_NAME = "org.bluez";
static constexpr const char* BLUEZ_ROOT_OBJECT_PATH = "/";
static constexpr const char* BLUEZ_OBJECT_PATH = "/org/bluez";

static constexpr const char* INTERFACE_BLUEZ_ADAPTER = "org.bluez.Adapter1";
static constexpr const char* INTERFACE_BLUEZ_DEVICE = "org.bluez.Device1";
static constexpr const char* INTERFACE_BLUEZ_PROFILE_MANAGER = "org.bluez.ProfileManager1";

static constexpr const char* AAWG_PROFILE_OBJECT_PATH = "/com/aawgd/bluetooth/aawg";
static constexpr const char* AAWG_PROfILE_UUID = "4de17a00-52cb-11e6-bdf4-0800200c9a66";

static constexpr const char* HSP_HS_PROFILE_OBJECT_PATH = "/com/aawgd/bluetooth/hsp";
static constexpr const char* HSP_AG_UUID = "00001112-0000-1000-8000-00805f9b34fb";
static constexpr const char* HSP_HS_UUID = "00001108-0000-1000-8000-00805f9b34fb";


class BluezAdapterProxy: private DBus::ObjectProxy {
    BluezAdapterProxy(std::shared_ptr<DBus::Connection> conn, DBus::Path path): DBus::ObjectProxy(conn, BLUEZ_BUS_NAME, path) {
        alias = this->create_property<std::string>(INTERFACE_BLUEZ_ADAPTER, "Alias");
        powered = this->create_property<bool>(INTERFACE_BLUEZ_ADAPTER, "Powered");
        discoverable = this->create_property<bool>(INTERFACE_BLUEZ_ADAPTER, "Discoverable");
        pairable = this->create_property<bool>(INTERFACE_BLUEZ_ADAPTER, "Pairable");
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
};


BluetoothHandler& BluetoothHandler::instance() {
    static BluetoothHandler instance;
    return instance;
}

DBus::ManagedObjects BluetoothHandler::getBluezObjects() {
    std::shared_ptr<DBus::ObjectProxy> m_bluezRootObject = m_connection->create_object_proxy(BLUEZ_BUS_NAME, BLUEZ_ROOT_OBJECT_PATH);
    DBus::MethodProxy getManagedObjects = *(m_bluezRootObject->create_method<DBus::ManagedObjects(void)>("org.freedesktop.DBus.ObjectManager", "GetManagedObjects"));

    return getManagedObjects();
}

void BluetoothHandler::initAdapter() {
    DBus::ManagedObjects objects = getBluezObjects();

    std::string adapter_path;
    for (auto const& [path, interfaces]: objects) {
        for (auto const& [interface, properties]: interfaces) {
            if (interface == INTERFACE_BLUEZ_ADAPTER) {
                adapter_path = path;
                Logger::instance()->info("Using bluetooth adapter at path: %s\n", path.c_str());
                break;
            }
        }
        if (!adapter_path.empty()) {
            break;
        }
    }

    if (adapter_path.empty()) {
        Logger::instance()->info("Did not find any bluetooth adapters\n");
    }
    else {
        m_adapter = BluezAdapterProxy::create(m_connection, adapter_path);
        m_adapter->alias->set_value(ADAPTER_ALIAS);
    }
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

void BluetoothHandler::exportProfiles() {
    std::shared_ptr<DBus::ObjectProxy> bluezObject = m_connection->create_object_proxy(BLUEZ_BUS_NAME, BLUEZ_OBJECT_PATH);
    DBus::MethodProxy registerProfile = *(bluezObject->create_method<void(DBus::Path, std::string, DBus::Properties)>(INTERFACE_BLUEZ_PROFILE_MANAGER, "RegisterProfile"));

    // Register AA Wireless Profile
    m_aawProfile = AAWirelessProfile::create(AAWG_PROFILE_OBJECT_PATH);
    if (m_connection->register_object(m_aawProfile, DBus::ThreadForCalling::DispatcherThread) != DBus::RegistrationStatus::Success) {
        Logger::instance()->info("Failed to register AA Wireless profile\n");
    }

    registerProfile(AAWG_PROFILE_OBJECT_PATH, AAWG_PROfILE_UUID, {
        {"Name", DBus::Variant("AA Wireless")},
        {"Role", DBus::Variant("server")},
        {"Channel", DBus::Variant(uint16_t(8))},
    });
    Logger::instance()->info("Bluetooth AA Wireless profile active\n");

    // Register HSP Handset profile
    m_hspProfile = HSPHSProfile::create(HSP_HS_PROFILE_OBJECT_PATH);
    if (m_connection->register_object(m_hspProfile, DBus::ThreadForCalling::DispatcherThread) != DBus::RegistrationStatus::Success) {
        Logger::instance()->info("Failed to register HSP Handset profile\n");
    }
    registerProfile(HSP_HS_PROFILE_OBJECT_PATH, HSP_HS_UUID, {
        {"Name", DBus::Variant("HSP HS")},
    });
    Logger::instance()->info("HSP Handset profile active\n");
}

void BluetoothHandler::connectDevice() {
    DBus::ManagedObjects objects = getBluezObjects();

    std::vector<std::string> device_paths;
    for (auto const& [path, interfaces]: objects) {
        for (auto const& [interface, properties]: interfaces) {
            if (interface == INTERFACE_BLUEZ_DEVICE) {
                device_paths.push_back(path);
            }
        }
    }

    if (!device_paths.size()) {
        Logger::instance()->info("Did not find any connected bluetooth device\n");
        return;
    }

    Logger::instance()->info("Found %d bluetooth devices\n", device_paths.size());

    for (const std::string &device_path: device_paths) {
        Logger::instance()->info("Trying to connect bluetooth device at path: %s\n", device_path.c_str());

        std::shared_ptr<DBus::ObjectProxy> bluezDevice = m_connection->create_object_proxy(BLUEZ_BUS_NAME, device_path);
        DBus::MethodProxy connectProfile = *(bluezDevice->create_method<void(std::string)>(INTERFACE_BLUEZ_DEVICE, "ConnectProfile"));

        try {
            connectProfile(HSP_AG_UUID);
            Logger::instance()->info("Bluetooth connected to the device\n");
            return;
        } catch (DBus::Error& e) {
            Logger::instance()->info("Failed to connect device at path: %s\n", device_path.c_str());
        }
    }

    Logger::instance()->info("Failed to connect to any known bluetooth device\n");

}

void BluetoothHandler::init() {
    // DBus::set_logging_function( DBus::log_std_err );
    // DBus::set_log_level( SL_TRACE );

    m_dispatcher = DBus::StandaloneDispatcher::create();
    m_connection = m_dispatcher->create_connection( DBus::BusType::SYSTEM );

    Logger::instance()->info("Unique Name: %s\n", m_connection->unique_name().c_str());

    initAdapter();
    exportProfiles();
}

void BluetoothHandler::connect() {
    if (!m_adapter) {
        return;
    }

    setPower(true);
    setPairable(true);
    connectDevice();
}

void BluetoothHandler::powerOff() {
    if (!m_adapter) {
        return;
    }

    setPower(false);
}