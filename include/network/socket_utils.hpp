#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

namespace network {

int create_udp_socket();
int create_tcp_socket();

void close_socket(int fd);

sockaddr_in make_address(const std::string& ip, std::uint16_t port);

std::string describe_endpoint(const std::string& ip, std::uint16_t port);

}  // namespace network
