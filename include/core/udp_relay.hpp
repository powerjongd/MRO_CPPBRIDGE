#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <fstream>

#include "network/socket_endpoint.hpp"
#include "utils/logger.hpp"
#include "utils/settings.hpp"

namespace core {

struct RelayStatus {
    bool running = false;
    std::size_t forwarded_packets = 0;
    std::size_t forwarded_bytes = 0;
};

class RoverRelayLogger;

class UdpRelay {
public:
    UdpRelay(const settings::RelaySettings& cfg, logging::Logger& logger, RoverRelayLogger* rover_logger);
    ~UdpRelay();

    void start();
    void stop();

    RelayStatus status() const;

private:
    void worker();

    settings::RelaySettings config_;
    logging::Logger& logger_;
    RoverRelayLogger* rover_logger_;

    std::atomic<bool> running_{false};
    std::thread worker_thread_;

    mutable std::mutex stats_mutex_;
    std::size_t forwarded_packets_ = 0;
    std::size_t forwarded_bytes_ = 0;
};

class RoverRelayLogger {
public:
    RoverRelayLogger(logging::Logger& logger, std::string directory);
    ~RoverRelayLogger();

    void start();
    void stop();
    void log_packet(const std::vector<std::uint8_t>& packet);

    bool active() const;
    std::size_t lines_written() const;

private:
    void open_file();
    void close_file();

    logging::Logger& logger_;
    std::string directory_;
    std::ofstream file_;
    std::atomic<bool> active_{false};
    std::atomic<std::size_t> lines_{0};
};

}  // namespace core
