#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <thread>
#include <optional>
#include <atomic>
#include <string>

#include "common.h"
#include "usb.h"
#include "proxyHandler.h"

ssize_t AAWProxy::readFully(int fd, unsigned char *buffer, size_t nbyte) {
    size_t remaining_bytes = nbyte;
    while (remaining_bytes > 0) {
        ssize_t len = read(fd, buffer, remaining_bytes);

        if (len <= 0) {
            // Error, cannot read more.
            return len;
        }

        buffer += len;
        remaining_bytes -= len;
    }

    return nbyte;
}

ssize_t AAWProxy::readMessage(int fd, unsigned char *buffer, size_t buffer_len) {
    size_t header_length = 4;
    if (ssize_t len = readFully(fd, buffer, header_length); len <= 0) {
        return len;
    }

    size_t message_length = (buffer[2] << 8) + buffer[3];

    constexpr char FRAME_TYPE_FIRST = 1 << 0;
    constexpr char FRAME_TYPE_LAST = 1 << 1;
    constexpr char FRAME_TYPE_MASK = FRAME_TYPE_FIRST | FRAME_TYPE_LAST;
    if ((buffer[1] & FRAME_TYPE_MASK) == FRAME_TYPE_FIRST) { // This means the header is 8 bytes long, we need to read four more bytes.
        message_length += 4;
    }

    if ((header_length + message_length) > buffer_len) {
        // Not enough space in the buffer. This is unexpected.
        errno = EMSGSIZE;
        return -1;
    }

    if (ssize_t len = readFully(fd, buffer + header_length, message_length); len <= 0) {
        return len;
    }

    return header_length + message_length;
}

void AAWProxy::forward(ProxyDirection direction, std::atomic<bool>& should_exit) {
    size_t buffer_len = 16384;
    unsigned char buffer[buffer_len];

    bool read_message;
    int read_fd, write_fd;
    std::string read_name, write_name;
    switch (direction) {
        case ProxyDirection::TCP_to_USB:
            read_message = true;

            read_fd = m_tcp_fd;
            read_name = "TCP";

            write_fd = m_usb_fd;
            write_name = "USB";
            break;
        case ProxyDirection::USB_to_TCP:
            read_message = false;

            read_fd = m_usb_fd;
            read_name = "USB";

            write_fd = m_tcp_fd;
            write_name = "TCP";
            break;
    }

    while (!should_exit) {
        ssize_t len = read_message ? readMessage(read_fd, buffer, buffer_len) : read(read_fd, buffer, buffer_len);
        Logger::instance()->info("%d bytes read from %s\n", len, read_name.c_str());
        if (len < 0) {
            Logger::instance()->info("Read from %s failed: %s\n", read_name.c_str(), strerror(errno));
            break;
        }
        else if (len == 0) {
            break;
        }

        ssize_t wlen = write(write_fd, buffer, len);
        Logger::instance()->info("%d bytes written to %s\n", wlen, write_name.c_str());
        if (wlen < 0) {
            Logger::instance()->info("Write to %s failed: %s\n", write_name.c_str(), strerror(errno));
            break;
        }
    }

    should_exit = true;
}

void AAWProxy::handleClient(int server_sock) {
    struct sockaddr client_address;
    socklen_t client_addresslen = sizeof(client_address);
    if ((m_tcp_fd = accept(server_sock, &client_address, &client_addresslen)) < 0) {
        close(server_sock);
        Logger::instance()->info("accept failed: %s\n", strerror(errno));
        return;
    }

    close(server_sock);

    Logger::instance()->info("Tcp server accepted connection\n");

    UsbManager::instance().enableDefaultAndWaitForAccessroy();

    Logger::instance()->info("Opening usb accessory\n");
    if ((m_usb_fd = open("/dev/usb_accessory", O_RDWR)) < 0) {
        Logger::instance()->info("error opening /dev/usb_accessory: %s\n", strerror(errno));
        return;
    }

    Logger::instance()->info("Frowarding data between TCP and USB\n");
    std::atomic<bool> should_exit = false;
    std::thread usb_tcp(&AAWProxy::forward, this, ProxyDirection::USB_to_TCP, std::ref(should_exit));
    std::thread tcp_usb(&AAWProxy::forward, this, ProxyDirection::TCP_to_USB, std::ref(should_exit));

    usb_tcp.join();
    tcp_usb.join();

    close(m_usb_fd);
    m_usb_fd = -1;

    close(m_tcp_fd);
    m_tcp_fd = -1;

    Logger::instance()->info("Forwarding stopped\n");
}

std::optional<std::thread> AAWProxy::startServer(int32_t port) {
    Logger::instance()->info("Starting tcp server\n");
    int server_sock;
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        Logger::instance()->info("creating socket failed: %s\n", strerror(errno));
        return std::nullopt;
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        Logger::instance()->info("setsockopt failed: %s\n", strerror(errno));
        return std::nullopt;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::instance()->info("bind failed: %s\n", strerror(errno));
        return std::nullopt;
    }

    if (listen(server_sock, 3) < 0) {
        Logger::instance()->info("listen failed: %s\n", strerror(errno));
        return std::nullopt;
    }

    Logger::instance()->info("Tcp server listening on %d\n", port);

    return std::thread(&AAWProxy::handleClient, this, server_sock);
}
