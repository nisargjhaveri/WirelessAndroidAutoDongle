#ifndef WEBSERVER_H
#define WEBSERVER_H

// Placeholder for CivetWeb header
// This will cause a compilation error until CivetWeb is properly integrated.
#include "civetweb.h"

#include <string>

// Forward declaration for CivetWeb connection structure
struct mg_connection;

class Config; // Forward declaration

class WebServer {
public:
    WebServer(int port);
    ~WebServer();

    bool start(); // Returns true on success, false on failure
    void stop();

private:
    // Static handler methods for API endpoints
    static int handleApiStatus(struct mg_connection *conn, void *cbdata);
    static int handleApiConfig(struct mg_connection *conn, void *cbdata);

    // Helper method for sending JSON responses
    static void sendJsonResponse(struct mg_connection *conn, const std::string& json_body, int http_status_code);

    struct mg_context *m_ctx;
    int m_port;
    bool m_running;
    Config* m_config; // Pointer to the global Config instance
};

#endif // WEBSERVER_H
