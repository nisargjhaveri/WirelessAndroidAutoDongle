#include <cstdlib>
#include "common.h"

#include "proto/WifiInfoResponse.pb.h"

/*static*/ Config* Config::instance() {
    static Config s_instance;
    return &s_instance;
}

int32_t Config::getenv(std::string name, int32_t defaultValue) {
    char* envValue = std::getenv(name.c_str());
    try {
        return envValue != nullptr ? std::stoi(envValue) : defaultValue;
    }
    catch(...) {
        return defaultValue;
    }
}

std::string Config::getenv(std::string name, std::string defaultValue) {
    char* envValue = std::getenv(name.c_str());
    return envValue != nullptr ? envValue : defaultValue;
}

WifiInfo Config::getWifiInfo() {
    return {
        getenv("AAWG_WIFI_SSID", "AAWirelessGateway"),
        getenv("AAWG_WIFI_PASSWORD", "ConnectAAWirelessGateway"),
        getenv("AAWG_WIFI_BSSID", "00:00:00:00:00:00"),
        SecurityMode::WPA2_PERSONAL,
        AccessPointType::DYNAMIC,
        getenv("AAWG_PROXY_IP_ADDRESS", "10.0.0.1"),
        getenv("AAWG_PROXY_PORT", 5288),
    };
}
