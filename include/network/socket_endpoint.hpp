#pragma once

#include <cstdint>
#include <string>

namespace network {

struct Endpoint {
    std::string address;
    std::uint16_t port = 0;
};

}  // namespace network
