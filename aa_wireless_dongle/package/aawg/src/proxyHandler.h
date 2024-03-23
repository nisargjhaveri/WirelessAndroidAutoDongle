#pragma once

#include <atomic>
#include <optional>
#include <thread>

class AAWProxy {
public:
    std::optional<std::thread> startServer(int32_t port);

private:
    enum class ProxyDirection {
        TCP_to_USB,
        USB_to_TCP
    };

    void handleClient(int server_fd);
    void forward(ProxyDirection direction, std::atomic<bool>& should_exit);
    void stopForwarding(std::atomic<bool>& should_exit);

    ssize_t readFully(int fd, unsigned char *buf, size_t nbyte);
    ssize_t readMessage(int fd, unsigned char *buf, size_t nbyte);

    int m_usb_fd = -1;
    int m_tcp_fd = -1;

    std::optional<std::thread> m_usb_tcp_thread = std::nullopt;
    std::optional<std::thread> m_tcp_usb_thread = std::nullopt;

    std::atomic<bool> m_log_communication = false;
};
