#include "webServer.h"
#include "config.h"  // For Config class (now known to be in common.h, but config.h is conventional)
                     // If common.h is the true source of Config, this might need adjustment
                     // For now, assuming config.h or common.h provides Config::instance()
#include "common.h"  // For Logger and Config
#include "civetweb.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <nlohmann/json.hpp> // For JSON operations

// Use nlohmann::json namespace
using json = nlohmann::json;

// Basic constructor
WebServer::WebServer(int port) : m_ctx(nullptr), m_port(port), m_running(false), m_config(nullptr) {
    // Initialize config instance
    m_config = Config::instance();

    // Convert port to string for options
    std::string port_str = std::to_string(m_port);

    // --- Authentication Setup ---
    // Create .htpasswd file for basic authentication
    // User: admin, Password: admin, Realm: AAWG WebUI
    // MD5 hash for "admin:AAWG WebUI:admin" is 79c244906d4780a6c5046e8131aa0073
    const char* htpasswd_path = "/tmp/aawg.htpasswd"; // Runtime generated
    FILE *fp = fopen(htpasswd_path, "w");
    if (fp) {
        fprintf(fp, "admin:AAWG WebUI:79c244906d4780a6c5046e8131aa0073\n");
        fclose(fp);
        Logger::instance()->info("WebServer: Created .htpasswd file at %s\n", htpasswd_path);
    } else {
        Logger::instance()->error("WebServer: Failed to create .htpasswd file at %s! Web UI will be unprotected.\n", htpasswd_path);
        // If file creation fails, auth might not work as expected or might be disabled by CivetWeb.
        // For this example, we proceed, but in production, this might be a critical error.
    }

    // Define options for CivetWeb
    // Document root is set to the installation path for web assets.
    const char *options[] = {
        "document_root", "/usr/share/aawg/www",
        "listening_ports", port_str.c_str(),
        "authentication_domain", "AAWG WebUI",
        "global_auth_file", htpasswd_path,
        "enable_directory_listing", "no",
        nullptr // Sentinel
    };

    Logger::instance()->info("WebServer: Initializing with port %d, DocRoot: /usr/share/aawg/www, AuthFile: %s\n", m_port, htpasswd_path);

    // Start CivetWeb server
    m_ctx = mg_start(nullptr, nullptr, options); // Pass options directly

    if (m_ctx == nullptr) {
        Logger::instance()->error("WebServer: Failed to start CivetWeb server on port %d.\n", m_port);
        m_running = false;
    } else {
        Logger::instance()->info("WebServer: CivetWeb server started successfully on port %d.\n", m_port);
        m_running = true;

        // Register handlers
        mg_set_request_handler(m_ctx, "/api/status", handleApiStatus, this);
        mg_set_request_handler(m_ctx, "/api/config", handleApiConfig, this);
        Logger::instance()->info("WebServer: Registered /api/status and /api/config handlers.\n");
    }
}

// Destructor
WebServer::~WebServer() {
    stop(); // Ensure server is stopped
}

// Start method (if server was stopped or failed to start in constructor)
bool WebServer::start() {
    if (m_running) {
        Logger::instance()->info("WebServer: Server is already running.\n");
        return true;
    }
    if (m_ctx != nullptr) { // Should not happen if m_running is false, but as a safeguard
         Logger::instance()->warn("WebServer: Context exists but server not marked running. Stopping first.\n");
         mg_stop(m_ctx);
         m_ctx = nullptr;
    }

    std::string port_str = std::to_string(m_port);
    // Re-apply options including auth for restart, assuming htpasswd file still exists or is recreated if needed.
    // For simplicity, we assume it exists if created by constructor.
    // A more robust solution might check and recreate it or have a dedicated setup method.
    const char* htpasswd_path_restart = "/tmp/aawg.htpasswd"; // Same path as in constructor
    const char *options[] = {
        "document_root", "/usr/share/aawg/www",
        "listening_ports", port_str.c_str(),
        "authentication_domain", "AAWG WebUI",
        "global_auth_file", htpasswd_path_restart,
        "enable_directory_listing", "no",
        nullptr
    };

    Logger::instance()->info("WebServer: Attempting to (re)start server on port %d with auth.\n", m_port);
    m_ctx = mg_start(nullptr, nullptr, options); // Pass options directly

    if (m_ctx == nullptr) {
        Logger::instance()->error("WebServer: Failed to (re)start CivetWeb server on port %s.\n", port_str.c_str());
        m_running = false;
        return false;
    } else {
        Logger::instance()->info("WebServer: CivetWeb server (re)started successfully on port %s.\n", port_str.c_str());
        m_running = true;
        // Re-register handlers if restart logic implies they are cleared
        mg_set_request_handler(m_ctx, "/api/status", handleApiStatus, this);
        mg_set_request_handler(m_ctx, "/api/config", handleApiConfig, this);
        Logger::instance()->info("WebServer: Re-registered /api/status and /api/config handlers.\n");
        return true;
    }
}

// Stop method
void WebServer::stop() {
    if (m_running && m_ctx != nullptr) {
        Logger::instance()->info("WebServer: Stopping CivetWeb server.\n");
        mg_stop(m_ctx);
        m_ctx = nullptr;
        m_running = false;
    } else {
        Logger::instance()->info("WebServer: Server is not running or context is null.\n");
    }
}

// --- Static Handler Implementations ---

void WebServer::sendJsonResponse(struct mg_connection *conn, const std::string& json_body, int http_status_code) {
    // Get standard HTTP status message text for the given code
    const char* status_message = mg_get_response_code_text(conn, http_status_code);
    if (status_message == nullptr) {
        // Fallback for unknown status codes, though CivetWeb should handle standard ones.
        status_message = http_status_code == 200 ? "OK" : "Error";
    }

    mg_printf(conn, "HTTP/1.1 %d %s\r\n"
                    "Content-Type: application/json\r\n"
                    "Connection: close\r\n"
                    "Content-Length: %d\r\n"
                    "\r\n",
                    http_status_code, status_message,
                    (int)json_body.length());
    mg_write(conn, json_body.c_str(), json_body.length());
}

int WebServer::handleApiStatus(struct mg_connection *conn, void *cbdata) {
    WebServer* server_instance = static_cast<WebServer*>(cbdata);
    if (!server_instance || !server_instance->m_config) {
        // Should not happen if cbdata is correctly passed `this`
        sendJsonResponse(conn, "{\"success\": false, \"message\": \"Internal server error: Config not available\"}", 500);
        return 1;
    }

    // Placeholders for actual status data - these would be fetched from relevant handlers
    bool bluetoothConnected = true; // TODO: Get actual Bluetooth status from BluetoothHandler
    std::string bluetoothDeviceName = "DummyPhone"; // TODO: Get actual device name
    std::string usbMode = "accessory"; // TODO: Get actual USB mode from UsbManager

    // Get IP address from Config instance (which might hold a cached or configured value)
    std::string ipAddress = server_instance->m_config->getIpAddress();

    std::ostringstream oss;
    oss << "{";
    oss << "\"bluetoothConnected\": " << (bluetoothConnected ? "true" : "false") << ",";
    oss << "\"bluetoothDeviceName\": \"" << bluetoothDeviceName << "\",";
    oss << "\"usbMode\": \"" << usbMode << "\",";
    oss << "\"ipAddress\": \"" << ipAddress << "\"";
    oss << "}";

    sendJsonResponse(conn, oss.str(), 200);
    return 1; // Indicates request has been handled
}

int WebServer::handleApiConfig(struct mg_connection *conn, void *cbdata) {
    WebServer* server_instance = static_cast<WebServer*>(cbdata);
    if (!server_instance || !server_instance->m_config) {
        sendJsonResponse(conn, "{\"success\": false, \"message\": \"Internal server error: Config not available\"}", 500);
        return 1;
    }

    Config* config = server_instance->m_config;
    const struct mg_request_info *ri = mg_get_request_info(conn);

    if (strcmp(ri->request_method, "GET") == 0) {
        std::ostringstream oss;
        oss << "{";
        oss << "\"wifiSsid\": \"" << config->getWifiInfo().ssid << "\","; // Assuming getWifiInfo().ssid is safe
        // Security Note: Exposing password like this is a major security risk.
        // This should be handled with extreme care in a real application (e.g., write-only, or not exposed at all).
        oss << "\"wifiPassword\": \"" << config->getWifiInfo().key << "\",";
        oss << "\"bluetoothDiscoverable\": " << (config->isBluetoothDiscoverable() ? "true" : "false") << ",";
        oss << "\"connectionStrategy\": \"" << Config::connectionStrategyToString(config->getConnectionStrategy()) << "\"";
        oss << "}";
        sendJsonResponse(conn, oss.str(), 200);
    } else if (strcmp(ri->request_method, "POST") == 0) {
        char post_data_buffer[1024]; // Buffer for POST data
        int post_data_len = mg_read(conn, post_data_buffer, sizeof(post_data_buffer) - 1);
        std::string post_data_str;

        if (post_data_len >= 0) {
            post_data_buffer[post_data_len] = '\0'; // Null-terminate the received data
            post_data_str = post_data_buffer;
        } else {
            sendJsonResponse(conn, "{\"success\": false, \"message\": \"Failed to read POST data\"}", 400);
            return 1;
        }

        Logger::instance()->info("WebServer: Received config POST data: %s\n", post_data_str.c_str());

        try {
            json received_json = json::parse(post_data_str);
            bool config_changed = false;

            if (received_json.contains("wifiSsid")) {
                if (!received_json["wifiSsid"].is_string()) {
                    sendJsonResponse(conn, "{\"success\": false, \"message\": \"Invalid type for wifiSsid\"}", 400);
                    return 1;
                }
                config->setWifiSsid(received_json["wifiSsid"].get<std::string>());
                config_changed = true;
            }
            if (received_json.contains("wifiPassword")) {
                if (!received_json["wifiPassword"].is_string()) {
                    sendJsonResponse(conn, "{\"success\": false, \"message\": \"Invalid type for wifiPassword\"}", 400);
                    return 1;
                }
                // Note: Password field might be empty if user doesn't want to change it.
                // setWifiPassword should handle empty string appropriately if it means "no change" or "clear password".
                config->setWifiPassword(received_json["wifiPassword"].get<std::string>());
                config_changed = true;
            }
            if (received_json.contains("bluetoothDiscoverable")) {
                if (!received_json["bluetoothDiscoverable"].is_boolean()) {
                    sendJsonResponse(conn, "{\"success\": false, \"message\": \"Invalid type for bluetoothDiscoverable\"}", 400);
                    return 1;
                }
                config->setBluetoothDiscoverable(received_json["bluetoothDiscoverable"].get<bool>());
                config_changed = true;
            }
            if (received_json.contains("connectionStrategy")) {
                if (!received_json["connectionStrategy"].is_string()) {
                    sendJsonResponse(conn, "{\"success\": false, \"message\": \"Invalid type for connectionStrategy\"}", 400);
                    return 1;
                }
                std::string strategyStr = received_json["connectionStrategy"].get<std::string>();
                // TODO: Add validation for strategyStr against known values if stringToConnectionStrategy doesn't throw/error handle.
                config->setConnectionStrategy(Config::stringToConnectionStrategy(strategyStr));
                config_changed = true;
            }

            if (config_changed) {
                // config->save(); // Setters now call save, so this might be redundant but ensures a save happens.
                                  // For now, relying on setters.
                sendJsonResponse(conn, "{\"success\": true, \"message\": \"Configuration updated successfully.\"}", 200);
            } else {
                sendJsonResponse(conn, "{\"success\": true, \"message\": \"No configuration values provided to update.\"}", 200);
            }

        } catch (json::parse_error& e) {
            Logger::instance()->error("WebServer: Failed to parse JSON from POST data: %s. Error: %s\n", post_data_str.c_str(), e.what());
            sendJsonResponse(conn, "{\"success\": false, \"message\": \"Invalid JSON format\"}", 400);
        } catch (json::exception& e) { // Catches other json exceptions like type errors if not checked before .get<T>()
            Logger::instance()->error("WebServer: JSON processing error: %s. Data: %s\n", e.what(), post_data_str.c_str());
            sendJsonResponse(conn, std::string("{\"success\": false, \"message\": \"JSON processing error: ") + e.what() + "\"}", 400);
        } catch (std::exception& e) {
            Logger::instance()->error("WebServer: Standard exception processing POST data: %s\n", e.what());
            sendJsonResponse(conn, std::string("{\"success\": false, \"message\": \"Server error: ") + e.what() + "\"}", 500);
        }
    } else {
        // Method not allowed
        mg_printf(conn, "HTTP/1.1 405 Method Not Allowed\r\n"
                        "Content-Type: application/json\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "{\"success\": false, \"message\": \"Method Not Allowed\"}");
    }
    return 1; // Indicates request has been handled
}
