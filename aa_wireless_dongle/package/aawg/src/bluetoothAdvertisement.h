#pragma once

#include "bluetoothCommon.h"

class BLEAdvertisement: public DBus::Object {
public:
    static std::shared_ptr<BLEAdvertisement> create(DBus::Path path);

    std::shared_ptr<DBus::Property<std::string>> type;
    std::shared_ptr<DBus::Property<std::vector<std::string>>> serviceUUIDs;
    std::shared_ptr<DBus::Property<std::string>> localName;

protected:
    BLEAdvertisement(DBus::Path path);

    void Release();
};
