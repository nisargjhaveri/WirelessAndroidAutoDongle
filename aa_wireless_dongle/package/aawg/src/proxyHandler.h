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

    ssize_t readFully(int fd, unsigned char *buf, size_t nbyte);
    ssize_t readMessage(int fd, unsigned char *buf, size_t nbyte);

    bool openUsbAccessory();

    bool shouldDoEarlyVersionExchange();
    bool performUsbVersionExchange();
    bool performTcpVersionExchange();

    int m_usb_fd = -1;
    int m_tcp_fd = -1;

    uint16_t usb_version_major = 0;
    uint16_t usb_version_minor = 0;
};
