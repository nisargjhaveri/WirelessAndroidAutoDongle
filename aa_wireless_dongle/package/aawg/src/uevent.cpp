#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "common.h"
#include "uevent.h"

constexpr ssize_t NETLINK_MSG_SIZE = 8 * 1024;

UeventMonitor& UeventMonitor::instance() {
    static UeventMonitor instance;
    return instance;
}

void UeventMonitor::monitorLoop(int nl_socket) {
    char msg[NETLINK_MSG_SIZE + 1];

    while (true) {
        ssize_t len = read(nl_socket, msg, NETLINK_MSG_SIZE);

        if (len < 0) {
            Logger::instance()->info("Read from netlink socket failed: %s\n", strerror(errno));
            continue;
        }
        else if (len == 0) {
            continue;
        }

        char* end = msg + len;
        *end = '\0';

        // printf("length: %ld msg: %s\n", len, msg);

        // Parse the env from message
        UeventEnv envMap;
        char* current = msg;
        while (current < end) {
            if (char* split = strchr(current, '='); split != NULL && split > current) {
                std::string envName(current, split - current);
                std::string envValue(split + 1);

                envMap.emplace(envName, envValue);
                // printf("%s = %s\n", envName.c_str(), envValue.c_str());
            }

            current += strlen(current) + 1;
        }

        // Call the handlers
        for (auto it = handlers.cbegin(); it != handlers.cend(); ++it) {
            if ((*it)(envMap)) {
                it = handlers.erase(it);
            }
        }
    }
}

void UeventMonitor::addHandler(std::function<bool(UeventEnv)> handler) {
    handlers.push_back(handler);
}

std::optional<std::thread> UeventMonitor::start() {
    Logger::instance()->info("Starting uevent monitoring\n");

    int nl_sock;
    if ((nl_sock = socket(AF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT)) < 0) {
        Logger::instance()->info("creating socket failed for netlink socket: %s\n", strerror(errno));
        return std::nullopt;
    }

    struct sockaddr_nl address = {
        .nl_family = AF_NETLINK,
        .nl_pid = (unsigned int)getpid(),
        .nl_groups = -1u
    };

    if (bind(nl_sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        Logger::instance()->info("bind failed for netlink socket: %s\n", strerror(errno));
        return std::nullopt;
    }

    int opt = 1;
    if (setsockopt(nl_sock, SOL_SOCKET, SO_PASSCRED, &opt, sizeof(opt))) {
        Logger::instance()->info("setsockopt failed to set SO_PASSCRED for netlink socket: %s\n", strerror(errno));
        return std::nullopt;
    }

    Logger::instance()->info("Uevent monitoring started\n");

    return std::thread(&UeventMonitor::monitorLoop, this, nl_sock);
}
