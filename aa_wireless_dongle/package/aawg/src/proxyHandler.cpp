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

#include "proxyHandler.h"


void AAWProxy::forward(ProxyDirection direction, std::atomic<bool>& should_exit) {
    size_t buffer_len = 16384;
    unsigned char buffer[buffer_len];

    int read_fd, write_fd;
    std::string read_name, write_name;
    switch (direction) {
        case ProxyDirection::TCP_to_USB:
            read_fd = m_tcp_fd;
            read_name = "TCP";

            write_fd = m_usb_fd;
            write_name = "USB";
            break;
        case ProxyDirection::USB_to_TCP:
            read_fd = m_usb_fd;
            read_name = "USB";

            write_fd = m_tcp_fd;
            write_name = "TCP";
            break;
    }

    while (!should_exit) {
        ssize_t len = read(read_fd, buffer, buffer_len);
        // printf("%d bytes read from %s\n", len, read_name.c_str());
        if (len < 0) {
            printf("Read from %s failed: %s\n", read_name.c_str(), strerror(errno));
            break;
        }
        else if (len == 0) {
            break;
        }

        ssize_t wlen = write(write_fd, buffer, len);
        // printf("%d bytes written to %s\n", wlen, write_name.c_str());
        if (wlen < 0) {
            printf("Write to %s failed: %s\n", write_name.c_str(), strerror(errno));
            break;
        }
    }

    should_exit = true;
}

void AAWProxy::handleClient(int server_sock) {
    struct sockaddr client_address;
    socklen_t client_addresslen;
    if ((m_tcp_fd = accept(server_sock, &client_address, &client_addresslen)) < 0) {
        printf("accept failed: %s\n", strerror(errno));
        return;
    }

    printf("Tcp server accepted connection\n");

    printf("Opening usb accessory\n");
    if ((m_usb_fd = open("/dev/usb_accessory", O_RDWR)) < 0) {
        printf("error opening /dev/usb_accessory: %s\n", strerror(errno));
        return;
    }

    printf("Frowarding data between TCP and USB\n");
    std::atomic<bool> should_exit = false;
    std::thread usb_tcp(&AAWProxy::forward, this, ProxyDirection::USB_to_TCP, std::ref(should_exit));
    std::thread tcp_usb(&AAWProxy::forward, this, ProxyDirection::TCP_to_USB, std::ref(should_exit));

    usb_tcp.join();
    tcp_usb.join();

    close(m_usb_fd);
    m_usb_fd = -1;

    close(m_tcp_fd);
    m_tcp_fd = -1;

    printf("Forwarding stopped\n");
}

std::optional<std::thread> AAWProxy::startServer() {
    printf("Starting tcp server\n");
    int server_sock;
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("creating socket failed: %s\n", strerror(errno));
        return std::nullopt;
    }

    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("setsockopt failed: %s\n", strerror(errno));
        return std::nullopt;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5288);

    if (bind(server_sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        printf("bind failed: %s\n", strerror(errno));
        return std::nullopt;
    }

    if (listen(server_sock, 3) < 0) {
        printf("listen failed: %s\n", strerror(errno));
        return std::nullopt;
    }

    printf("Tcp server listening\n");

    return std::thread(&AAWProxy::handleClient, this, server_sock);
}
