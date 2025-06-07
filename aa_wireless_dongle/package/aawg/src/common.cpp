#include <cstdlib>
#include <cstdarg>
#include <sstream>
#include <fstream>   // For std::ifstream, std::ofstream
#include <syslog.h>
#include <nlohmann/json.hpp> // For JSON operations

#include "common.h"
#include "proto/WifiInfoResponse.pb.h"

// Use nlohmann::json namespace
using json = nlohmann::json;

static constexpr const char* CONFIG_FILE_PATH = "/etc/aawg/config.json";

#pragma region Config
// Constructor
Config::Config() :
    connectionStrategy(std::nullopt), // Explicitly initialize optional members
    m_wifiSsid_override(std::nullopt),
    m_wifiPassword_override(std::nullopt),
    m_bluetoothDiscoverable(true) // Default, will be overridden by load() if config exists
{
    // Default IP, can be overridden by load() or set dynamically
    m_ipAddress = getenv("AAWG_WEBUI_IP_ADDRESS", "0.0.0.0");

    // Load persisted configuration
    load();
}

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

std::string Config::getMacAddress(std::string interface) {
    std::ifstream addressFile("/sys/class/net/" + interface + "/address");

    std::string macAddress;
    getline(addressFile, macAddress);

    return macAddress;
}

std::string Config::getUniqueSuffix() {
    std::string uniqueSuffix = getenv("AAWG_UNIQUE_NAME_SUFFIX", "");
    if (!uniqueSuffix.empty()) {
        return uniqueSuffix;
    }

    std::ifstream serialNumberFile("/sys/firmware/devicetree/base/serial-number");

    std::string serialNumber;
    getline(serialNumberFile, serialNumber);

    // Removing trailing null from serialNumber, pad at the beginning
    serialNumber = std::string("00000000") + serialNumber.c_str();

    return serialNumber.substr(serialNumber.size() - 6);
}

WifiInfo Config::getWifiInfo() {
    // Prioritize overrides from config file / WebUI
    std::string ssid = m_wifiSsid_override.value_or(getenv("AAWG_WIFI_SSID", "AAWirelessDongle"));
    std::string password = m_wifiPassword_override.value_or(getenv("AAWG_WIFI_PASSWORD", "ConnectAAWirelessDongle"));

    return {
        ssid,
        password,
        getenv("AAWG_WIFI_BSSID", getMacAddress("wlan0")), // BSSID typically not user-configured
        SecurityMode::WPA2_PERSONAL, // Usually fixed or auto-detected
        AccessPointType::DYNAMIC,    // Usually fixed
        m_ipAddress, // Use the m_ipAddress member, which might have been loaded
        getenv("AAWG_PROXY_PORT", 5288), // Port usually fixed or env-configured
    };
}

void Config::setWifiSsid(const std::string& ssid) {
    m_wifiSsid_override = ssid;
    Logger::instance()->info("Config: WiFi SSID set to %s\n", ssid.c_str());
    save();
}

void Config::setWifiPassword(const std::string& password) {
    m_wifiPassword_override = password;
    // Be cautious logging passwords, even in info logs.
    Logger::instance()->info("Config: WiFi Password has been set (not logged for security).\n");
    save();
}

ConnectionStrategy Config::getConnectionStrategy() {
    // Prioritize value set via WebUI/config file, then environment, then default.
    if (connectionStrategy.has_value()) {
        return connectionStrategy.value();
    }

    // Fallback to environment variable if not loaded from config or set at runtime
    const int32_t connectionStrategyEnv = getenv("AAWG_CONNECTION_STRATEGY", 1); // Default to PHONE_FIRST (1)
    ConnectionStrategy strategyFromEnv;
    switch (connectionStrategyEnv) {
        case 0:
            strategyFromEnv = ConnectionStrategy::DONGLE_MODE;
            break;
        case 1:
            strategyFromEnv = ConnectionStrategy::PHONE_FIRST;
            break;
        case 2:
            strategyFromEnv = ConnectionStrategy::USB_FIRST;
            break;
        default:
            strategyFromEnv = ConnectionStrategy::PHONE_FIRST; // Default if env var is invalid
            break;
    }
    connectionStrategy = strategyFromEnv; // Cache it
    return strategyFromEnv;
}

void Config::setConnectionStrategy(ConnectionStrategy strategy) {
    if (!connectionStrategy.has_value() || connectionStrategy.value() != strategy) {
        connectionStrategy = strategy;
        Logger::instance()->info("Config: Connection Strategy set to %s\n", connectionStrategyToString(strategy).c_str());
        save();
    }
}

std::string Config::connectionStrategyToString(ConnectionStrategy strategy) {
    switch (strategy) {
        case ConnectionStrategy::DONGLE_MODE: return "DONGLE_MODE";
        case ConnectionStrategy::PHONE_FIRST: return "PHONE_FIRST";
        case ConnectionStrategy::USB_FIRST: return "USB_FIRST";
        default: return "UNKNOWN";
    }
}

ConnectionStrategy Config::stringToConnectionStrategy(const std::string& strategyStr) {
    if (strategyStr == "DONGLE_MODE") return ConnectionStrategy::DONGLE_MODE;
    if (strategyStr == "USB_FIRST") return ConnectionStrategy::USB_FIRST;
    if (strategyStr == "PHONE_FIRST") return ConnectionStrategy::PHONE_FIRST; // Assuming PHONE_FIRST is a valid string representation
    return ConnectionStrategy::PHONE_FIRST; // Default or throw error
}

// WebUI related config implementations
std::string Config::getIpAddress() const {
    return m_ipAddress;
}

void Config::setIpAddress(const std::string& ip) {
    m_ipAddress = ip;
    Logger::instance()->info("Config: IP Address set to %s (runtime only, not persisted by default)\n", m_ipAddress.c_str());
    // IP address is usually dynamic or set by network services.
    // If this needs to be persisted as a *requested* static IP for the device itself, then save() would be called.
    // save();
}

bool Config::isBluetoothDiscoverable() const {
    return m_bluetoothDiscoverable;
}

void Config::setBluetoothDiscoverable(bool discoverable) {
    if (m_bluetoothDiscoverable != discoverable) {
        m_bluetoothDiscoverable = discoverable;
        Logger::instance()->info("Config: Bluetooth Discoverable set to %s\n", m_bluetoothDiscoverable ? "true" : "false");
        save();
    }
}

void Config::load() {
    std::ifstream configFile(CONFIG_FILE_PATH);
    if (!configFile.is_open()) {
        Logger::instance()->info("Config: Configuration file '%s' not found. Using defaults and environment variables.\n", CONFIG_FILE_PATH);
        // Attempt to save a default config if it doesn't exist, so user has a template
        // This also ensures the directory structure is tested for writability if it's the first run.
        // save(); // Be careful about calling save() inside load() if save() also calls load() or if it's too early.
        // For now, just return and rely on defaults/env. A save can happen on first config change via UI.
        return;
    }

    try {
        json j;
        configFile >> j;

        // Load values, using current member values (from constructor/env) as defaults if key is missing
        m_wifiSsid_override = j.value("wifiSsid", m_wifiSsid_override.value_or(""));
        if (m_wifiSsid_override.value_or("").empty()) m_wifiSsid_override = std::nullopt; // Treat empty string as not set

        m_wifiPassword_override = j.value("wifiPassword", m_wifiPassword_override.value_or(""));
        if (m_wifiPassword_override.value_or("").empty()) m_wifiPassword_override = std::nullopt;

        m_bluetoothDiscoverable = j.value("bluetoothDiscoverable", m_bluetoothDiscoverable);

        if (j.contains("connectionStrategy")) {
            connectionStrategy = stringToConnectionStrategy(j.value("connectionStrategy", "PHONE_FIRST"));
        }

        m_ipAddress = j.value("ipAddress", m_ipAddress); // IP might be more dynamic, but load if present

        Logger::instance()->info("Config: Configuration loaded from %s\n", CONFIG_FILE_PATH);
    } catch (json::parse_error& e) {
        Logger::instance()->info("Config: Failed to parse configuration file '%s': %s. Using defaults and environment variables.\n", CONFIG_FILE_PATH, e.what());
    } catch (json::exception& e) {
        Logger::instance()->info("Config: JSON error while processing file '%s': %s. Using defaults and environment variables.\n", CONFIG_FILE_PATH, e.what());
    }
}

void Config::save() {
    Logger::instance()->info("Config: Attempting to save configuration to %s\n", CONFIG_FILE_PATH);
    json j;

    // Only save overrides if they have a value, otherwise they won't be in the JSON
    if (m_wifiSsid_override.has_value()) {
        j["wifiSsid"] = m_wifiSsid_override.value();
    }
    if (m_wifiPassword_override.has_value()) {
        j["wifiPassword"] = m_wifiPassword_override.value(); // Security: Saving password to file
    }

    j["bluetoothDiscoverable"] = m_bluetoothDiscoverable;

    if (connectionStrategy.has_value()) {
        j["connectionStrategy"] = connectionStrategyToString(connectionStrategy.value());
    }

    j["ipAddress"] = m_ipAddress; // Persist IP address if it was set (e.g. for static config)

    std::ofstream configFile(CONFIG_FILE_PATH);
    if (!configFile.is_open()) {
        Logger::instance()->info("Config: Failed to open configuration file '%s' for writing.\n", CONFIG_FILE_PATH);
        return;
    }

    try {
        configFile << std::setw(4) << j << std::endl; // Pretty print JSON
        Logger::instance()->info("Config: Configuration saved successfully to %s\n", CONFIG_FILE_PATH);
    } catch (json::exception& e) {
        Logger::instance()->info("Config: JSON error while saving to file '%s': %s.\n", CONFIG_FILE_PATH, e.what());
    }
}

#pragma endregion Config

#pragma region Logger
            case 0:
                connectionStrategy = ConnectionStrategy::DONGLE_MODE;
                break;
            case 1:
                connectionStrategy = ConnectionStrategy::PHONE_FIRST;
                break;
            case 2:
                connectionStrategy = ConnectionStrategy::USB_FIRST;
                break;
            default:
                connectionStrategy = ConnectionStrategy::PHONE_FIRST;
                break;
        }
    }

    return connectionStrategy.value();
}

std::string Config::connectionStrategyToString(ConnectionStrategy strategy) {
    switch (strategy) {
        case ConnectionStrategy::DONGLE_MODE: return "DONGLE_MODE";
        // Assuming PHONE_FIRST was meant to be a valid case based on prior observation of this file.
        // If not, it can be removed or handled by default.
        case ConnectionStrategy::PHONE_FIRST: return "PHONE_FIRST";
        case ConnectionStrategy::USB_FIRST: return "USB_FIRST";
        default: return "UNKNOWN";
    }
}

// WebUI related config implementations
std::string Config::getIpAddress() const {
    // This could potentially fetch a live IP if not set/updated elsewhere
    return m_ipAddress;
}

void Config::setIpAddress(const std::string& ip) {
    m_ipAddress = ip;
    // Note: IP address is usually dynamic or set by network services,
    // persisting it here might not be standard unless it's a static IP configuration for the device itself.
    // No call to save() here by default.
    Logger::instance()->info("Config: IP Address set to %s (runtime only for now)\n", m_ipAddress.c_str());
}

bool Config::isBluetoothDiscoverable() const {
    return m_bluetoothDiscoverable;
}

void Config::setBluetoothDiscoverable(bool discoverable) {
    if (m_bluetoothDiscoverable != discoverable) {
        m_bluetoothDiscoverable = discoverable;
        Logger::instance()->info("Config: Bluetooth Discoverable set to %s\n", m_bluetoothDiscoverable ? "true" : "false");
        // If save() from a persistent file was implemented, it would be called here.
        // save();
        // For now, we log that it would be saved.
        Logger::instance()->info("Config: (Simulated) save() would be called here to persist m_bluetoothDiscoverable.\n");
    }
}

// void Config::load() {
//    // Placeholder: Implementation for loading config from a file (e.g., JSON)
//    // Example for m_bluetoothDiscoverable if using nlohmann::json json_config object:
//    // m_bluetoothDiscoverable = json_config.value("bluetoothDiscoverable", true);
// }

// void Config::save() {
//    // Placeholder: Implementation for saving config to a file (e.g., JSON)
//    // Example for m_bluetoothDiscoverable if using nlohmann::json json_config object:
//    // json_config["bluetoothDiscoverable"] = m_bluetoothDiscoverable;
//    // ... write json_config to file ...
//    Logger::instance()->info("Config: save() called (currently a placeholder).\n");
// }

#pragma endregion Config

#pragma region Logger
/*static*/ Logger* Logger::instance() {
    static Logger s_instance;
    return &s_instance;
}

Logger::Logger() {
    openlog(nullptr, LOG_PERROR | LOG_PID, LOG_USER);
}

Logger::~Logger() {
    closelog();
}

void Logger::info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsyslog(LOG_INFO, format, args);
    va_end(args);
}
#pragma endregion Logger