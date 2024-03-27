#include <stdio.h>

#include "common.h"
#include "bluetoothAdvertisement.h"

static constexpr const char* INTERFACE_BLUEZ_LE_ADVERTISEMENT = "org.bluez.LEAdvertisement1";

/* static */ std::shared_ptr<BLEAdvertisement> BLEAdvertisement::create(DBus::Path path) {
    return std::shared_ptr<BLEAdvertisement>(new BLEAdvertisement(path));
}

BLEAdvertisement::BLEAdvertisement(DBus::Path path): DBus::Object(path) {
    this->create_method<void(void)>(INTERFACE_BLUEZ_LE_ADVERTISEMENT, "Release", sigc::mem_fun(*this, &BLEAdvertisement::Release));

    type = this->create_property<std::string>(INTERFACE_BLUEZ_LE_ADVERTISEMENT, "Type", DBus::PropertyAccess::ReadOnly);
    serviceUUIDs = this->create_property<std::vector<std::string>>(INTERFACE_BLUEZ_LE_ADVERTISEMENT, "ServiceUUIDs");
    localName = this->create_property<std::string>(INTERFACE_BLUEZ_LE_ADVERTISEMENT, "LocalName");
}

void BLEAdvertisement::Release() {
    Logger::instance()->info("Bluetooth LE Advertisement released\n");
}
