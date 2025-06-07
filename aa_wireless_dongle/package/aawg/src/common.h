#pragma once

#include <string>
#include <cstdint>
#include <optional>

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

enum class ConnectionStrategy {
    DONGLE_MODE = 0,
    PHONE_FIRST = 1,
    USB_FIRST = 2
};

class Config {
public:
    static Config* instance();

    WifiInfo getWifiInfo(); // Will need modification to use overrides
    void setWifiSsid(const std::string& ssid);
    void setWifiPassword(const std::string& password); // Security: consider implications

    ConnectionStrategy getConnectionStrategy();
    void setConnectionStrategy(ConnectionStrategy strategy);
    static std::string connectionStrategyToString(ConnectionStrategy strategy); // Helper
    static ConnectionStrategy stringToConnectionStrategy(const std::string& strategyStr);

    std::string getUniqueSuffix();

    // WebUI related config values
    std::string getIpAddress() const;
    void setIpAddress(const std::string& ip); // Might not be persisted if dynamic
    bool isBluetoothDiscoverable() const;
    void setBluetoothDiscoverable(bool discoverable); // Should be persisted


    void load();
    void save();

private:
    Config(); // Modified to allow initialization

    int32_t getenv(std::string name, int32_t defaultValue);
    std::string getenv(std::string name, std::string defaultValue);

    std::string getMacAddress(std::string interface);

    // Overrides for WiFi settings, populated from config file or WebUI
    std::optional<std::string> m_wifiSsid_override;
    std::optional<std::string> m_wifiPassword_override;

    std::optional<ConnectionStrategy> connectionStrategy; // Retains optional for env-first load
    std::string m_ipAddress; // Could be from config or dynamic
    bool m_bluetoothDiscoverable; // From config
};

class Logger {
public:
    static Logger* instance();

    void info(const char *format, ...);
private:
    Logger();
    ~Logger();
};