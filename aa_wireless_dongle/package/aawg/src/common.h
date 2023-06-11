#pragma once

#include <string>
#include <cstdint>

enum SecurityMode: int;
enum AccessPointType: int;

struct WifiInfo {
    std::string ssid;
    std::string key;
    std::string bssid;
    SecurityMode securityMode;
    AccessPointType accessPointType;
    std::string ipAddress;
    int32_t port;
};

class Config {
public:
    static Config* instance();

    WifiInfo getWifiInfo();
private:
    Config() = default;

    int32_t getenv(std::string name, int32_t defaultValue);
    std::string getenv(std::string name, std::string defaultValue);

    std::string getMacAddress(std::string interface);
};

class Logger {
public:
    static Logger* instance();

    void info(const char *format, ...);
private:
    Logger();
    ~Logger();
};