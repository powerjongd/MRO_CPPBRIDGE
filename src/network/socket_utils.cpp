#include "network/socket_utils.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <sstream>
#include <stdexcept>

namespace network {

int create_udp_socket() {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }
    return fd;
}

int create_tcp_socket() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("Failed to create TCP socket");
    }
    return fd;
}

void close_socket(int fd) {
    if (fd >= 0) {
#ifdef _WIN32
        ::closesocket(fd);
#else
        ::close(fd);
#endif
    }
}

sockaddr_in make_address(const std::string& ip, std::uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        throw std::runtime_error("Invalid IPv4 address: " + ip);
    }
    return addr;
}

std::string describe_endpoint(const std::string& ip, std::uint16_t port) {
    std::ostringstream oss;
    oss << ip << ':' << port;
    return oss.str();
}

}  // namespace network
