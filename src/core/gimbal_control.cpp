#include "core/gimbal_control.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>

#include "network/socket_utils.hpp"

namespace core {

namespace {
std::vector<std::uint8_t> build_packet(double yaw, double pitch, double roll, double zoom) {
    std::vector<std::uint8_t> packet(32, 0);
    auto encode = [](double value) {
        int32_t scaled = static_cast<int32_t>(value * 100);
        std::uint8_t bytes[4];
        bytes[0] = static_cast<std::uint8_t>((scaled >> 24) & 0xFF);
        bytes[1] = static_cast<std::uint8_t>((scaled >> 16) & 0xFF);
        bytes[2] = static_cast<std::uint8_t>((scaled >> 8) & 0xFF);
        bytes[3] = static_cast<std::uint8_t>(scaled & 0xFF);
        return std::array<std::uint8_t, 4>{bytes[0], bytes[1], bytes[2], bytes[3]};
    };
    auto yaw_bytes = encode(yaw);
    auto pitch_bytes = encode(pitch);
    auto roll_bytes = encode(roll);
    auto zoom_bytes = encode(zoom);
    std::copy(yaw_bytes.begin(), yaw_bytes.end(), packet.begin());
    std::copy(pitch_bytes.begin(), pitch_bytes.end(), packet.begin() + 4);
    std::copy(roll_bytes.begin(), roll_bytes.end(), packet.begin() + 8);
    std::copy(zoom_bytes.begin(), zoom_bytes.end(), packet.begin() + 12);
    return packet;
}
}

GimbalControl::GimbalControl(const settings::GimbalSettings& cfg, logging::Logger& logger)
    : config_(cfg), logger_(logger) {}

GimbalControl::~GimbalControl() { stop(); }

void GimbalControl::start() {
    if (running_) return;
    running_ = true;
    worker_thread_ = std::thread(&GimbalControl::worker, this);
    logger_.info("Gimbal control started");
}

void GimbalControl::stop() {
    if (!running_) return;
    running_ = false;
    if (worker_thread_.joinable()) worker_thread_.join();
    logger_.info("Gimbal control stopped");
}

void GimbalControl::update_pose(double yaw_deg, double pitch_deg, double roll_deg, double zoom_level) {
    std::lock_guard<std::mutex> lock(pose_mutex_);
    yaw_ = yaw_deg;
    pitch_ = pitch_deg;
    roll_ = roll_deg;
    zoom_ = zoom_level;
}

GimbalStatus GimbalControl::status() const {
    std::lock_guard<std::mutex> lock(pose_mutex_);
    return GimbalStatus{running_, yaw_, pitch_, roll_, zoom_};
}

void GimbalControl::worker() {
    int sock = -1;
    try {
        sock = network::create_udp_socket();
        sockaddr_in target = network::make_address(config_.generator_ip, static_cast<std::uint16_t>(config_.generator_port));
        while (running_) {
            std::vector<std::uint8_t> packet;
            {
                std::lock_guard<std::mutex> lock(pose_mutex_);
                packet = build_packet(yaw_, pitch_, roll_, zoom_);
            }
            sendto(sock, reinterpret_cast<const char*>(packet.data()), packet.size(), 0,
                   reinterpret_cast<sockaddr*>(&target), sizeof(target));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    } catch (const std::exception& ex) {
        logger_.error(std::string("Gimbal control error: ") + ex.what());
    }
    network::close_socket(sock);
}

}  // namespace core
