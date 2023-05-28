#pragma once

#include "bluetoothCommon.h"

class BluezProfile: public DBus::Object {
    virtual void Release() = 0;
    virtual void NewConnection(DBus::Path path, std::shared_ptr<DBus::FileDescriptor> fd, DBus::Properties fdProperties) = 0;
    virtual void RequestDisconnection(DBus::Path path) = 0;

protected:
    BluezProfile(DBus::Path path);
};


class AAWirelessProfile: public BluezProfile {
public:
    static std::shared_ptr<AAWirelessProfile> create(DBus::Path path);

private:
    void Release() override;
    void NewConnection(DBus::Path path, std::shared_ptr<DBus::FileDescriptor> fd, DBus::Properties fdProperties) override;
    void RequestDisconnection(DBus::Path path) override;

    AAWirelessProfile(DBus::Path path);
};


class HSPHSProfile: public BluezProfile {
public:
    static std::shared_ptr<HSPHSProfile> create(DBus::Path path);

private:
    void Release() override;
    void NewConnection(DBus::Path path, std::shared_ptr<DBus::FileDescriptor> fd, DBus::Properties fdProperties) override;
    void RequestDisconnection(DBus::Path path) override;

    HSPHSProfile(DBus::Path path);
};
