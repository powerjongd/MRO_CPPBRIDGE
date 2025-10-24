#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "network/socket_endpoint.hpp"
#include "utils/logger.hpp"
#include "utils/settings.hpp"

namespace core {

struct ImageStreamStatus {
    bool udp_running = false;
    bool tcp_running = false;
    std::size_t last_frame_bytes = 0;
    std::chrono::system_clock::time_point last_frame_time{};
    std::size_t clients = 0;
};

class ImageStreamBridge {
public:
    ImageStreamBridge(const settings::BridgeSettings& cfg, logging::Logger& logger);
    ~ImageStreamBridge();

    void start();
    void stop();

    ImageStreamStatus status() const;

private:
    void udp_loop();
    void tcp_loop();

    settings::BridgeSettings config_;
    logging::Logger& logger_;

    std::atomic<bool> running_{false};
    int udp_socket_ = -1;
    int tcp_socket_ = -1;
    std::thread udp_thread_;
    std::thread tcp_thread_;

    mutable std::mutex frame_mutex_;
    std::vector<std::uint8_t> last_frame_;
    std::chrono::system_clock::time_point last_frame_time_{};
    std::size_t tcp_clients_ = 0;
};

}  // namespace core
