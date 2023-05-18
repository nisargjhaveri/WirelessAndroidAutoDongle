#pragma once

#include <dbus-cxx.h>

namespace DBus {
    typedef std::map<std::string, DBus::Variant> Properties;
    typedef std::map<std::string, Properties> Interfaces;
    typedef std::map<DBus::Path, Interfaces> ManagedObjects;
}

class BluezAdapterProxy;
class AAWirelessProfile;
class HSPHSProfile;

class BluetoothHandler {
public:
    void init();

private:
    DBus::ManagedObjects getBluezObjects();

    void initAdapter();
    void powerOn();
    void setPairable(bool pairable);
    void exportProfiles();
    void connectDevice();

    std::shared_ptr<DBus::Dispatcher> m_dispatcher;
    std::shared_ptr<DBus::Connection> m_connection;
    std::shared_ptr<BluezAdapterProxy> m_adapter;

    std::shared_ptr<AAWirelessProfile> m_aawProfile;
    std::shared_ptr<HSPHSProfile> m_hspProfile;
};
