#include "core/udp_relay.hpp"

#ifdef _WIN32
#include <BaseTsd.h>
#include <winsock2.h>
#include <ws2tcpip.h>
using ssize_t = SSIZE_T;
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>

#include "network/socket_utils.hpp"

namespace core {

namespace {
std::string hex_dump(const std::vector<std::uint8_t>& data) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i > 0) oss << ' ';
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string timestamp_string() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << buf << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}
}

UdpRelay::UdpRelay(const settings::RelaySettings& cfg, logging::Logger& logger, RoverRelayLogger* rover_logger)
    : config_(cfg), logger_(logger), rover_logger_(rover_logger) {}

UdpRelay::~UdpRelay() { stop(); }

void UdpRelay::start() {
    if (running_ || !config_.enable) return;
    running_ = true;
    worker_thread_ = std::thread(&UdpRelay::worker, this);
    logger_.info("UDP relay started");
}

void UdpRelay::stop() {
    if (!running_) return;
    running_ = false;
    if (worker_thread_.joinable()) worker_thread_.join();
    logger_.info("UDP relay stopped");
}

RelayStatus UdpRelay::status() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return RelayStatus{running_, forwarded_packets_, forwarded_bytes_};
}

void UdpRelay::worker() {
    int sock = -1;
    try {
        sock = network::create_udp_socket();
        sockaddr_in bind_addr = network::make_address(config_.bind_ip, static_cast<std::uint16_t>(config_.bind_port));
        if (bind(sock, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
            logger_.error("Failed to bind UDP relay socket");
            running_ = false;
            network::close_socket(sock);
            return;
        }

        sockaddr_in raw_addr = network::make_address(config_.raw_ip, static_cast<std::uint16_t>(config_.raw_port));
        sockaddr_in proc_addr = network::make_address(config_.proc_ip, static_cast<std::uint16_t>(config_.proc_port));

        std::vector<std::uint8_t> buffer(64 * 1024);
        while (running_) {
            sockaddr_in src{};
            socklen_t len = sizeof(src);
            ssize_t received = recvfrom(sock, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0,
                                        reinterpret_cast<sockaddr*>(&src), &len);
            if (received < 0) {
                if (!running_) break;
                continue;
            }

            auto bytes = static_cast<std::size_t>(received);
            std::vector<std::uint8_t> packet(buffer.begin(), buffer.begin() + bytes);
            sendto(sock, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()), 0,
                   reinterpret_cast<sockaddr*>(&raw_addr), sizeof(raw_addr));
            sendto(sock, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()), 0,
                   reinterpret_cast<sockaddr*>(&proc_addr), sizeof(proc_addr));

            if (rover_logger_ && rover_logger_->active()) {
                rover_logger_->log_packet(packet);
            }

            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                forwarded_packets_++;
                forwarded_bytes_ += static_cast<std::size_t>(packet.size());
            }
        }
    } catch (const std::exception& ex) {
        logger_.error(std::string("UDP relay error: ") + ex.what());
    }
    network::close_socket(sock);
}

RoverRelayLogger::RoverRelayLogger(logging::Logger& logger, std::string directory)
    : logger_(logger), directory_(std::move(directory)) {}

RoverRelayLogger::~RoverRelayLogger() { stop(); }

void RoverRelayLogger::start() {
    if (active_) return;
    active_ = true;
    open_file();
}

void RoverRelayLogger::stop() {
    if (!active_) return;
    active_ = false;
    close_file();
}

void RoverRelayLogger::log_packet(const std::vector<std::uint8_t>& packet) {
    if (!active_ || !file_) return;
    file_ << timestamp_string() << "\tlen=" << packet.size() << '\t' << hex_dump(packet) << '\n';
    ++lines_;
}

bool RoverRelayLogger::active() const { return active_; }
std::size_t RoverRelayLogger::lines_written() const { return lines_.load(); }

void RoverRelayLogger::open_file() {
    namespace fs = std::filesystem;
    fs::create_directories(directory_);
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "rover-%Y%m%d-%H%M%S.log", &tm);
    fs::path path = fs::path(directory_) / buf;
    file_.open(path, std::ios::out | std::ios::app);
    if (!file_) {
        logger_.error("Failed to open rover relay log file");
        active_ = false;
    } else {
        logger_.info(std::string("Logging rover relay to ") + path.string());
    }
}

void RoverRelayLogger::close_file() {
    if (file_) {
        logger_.info("Rover relay logging stopped");
        file_.close();
    }
}

}  // namespace core
