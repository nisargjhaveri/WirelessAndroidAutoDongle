#pragma once

#include "bluetoothCommon.h"

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
