#include "core/image_stream_bridge.hpp"

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
#include <cstring>
#include <vector>

#include "network/socket_utils.hpp"

namespace core {

ImageStreamBridge::ImageStreamBridge(const settings::BridgeSettings& cfg, logging::Logger& logger)
    : config_(cfg), logger_(logger) {}

ImageStreamBridge::~ImageStreamBridge() { stop(); }

void ImageStreamBridge::start() {
    if (running_) return;
    running_ = true;

    udp_socket_ = network::create_udp_socket();
    sockaddr_in addr = network::make_address(config_.ip, static_cast<std::uint16_t>(config_.udp_port));
    if (bind(udp_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        logger_.error("Failed to bind UDP socket for image stream");
        network::close_socket(udp_socket_);
        udp_socket_ = -1;
        running_ = false;
        return;
    }

    tcp_socket_ = network::create_tcp_socket();
    int opt = 1;
#ifdef _WIN32
    setsockopt(tcp_socket_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(tcp_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    sockaddr_in tcp_addr = network::make_address(config_.ip, static_cast<std::uint16_t>(config_.tcp_port));
    if (bind(tcp_socket_, reinterpret_cast<sockaddr*>(&tcp_addr), sizeof(tcp_addr)) < 0 ||
        listen(tcp_socket_, 5) < 0) {
        logger_.error("Failed to bind TCP socket for image stream");
        network::close_socket(tcp_socket_);
        tcp_socket_ = -1;
        running_ = false;
        return;
    }

    udp_thread_ = std::thread(&ImageStreamBridge::udp_loop, this);
    tcp_thread_ = std::thread(&ImageStreamBridge::tcp_loop, this);
    logger_.infof("Image stream bridge started on UDP {} and TCP {}", config_.udp_port, config_.tcp_port);
}

void ImageStreamBridge::stop() {
    if (!running_) return;
    running_ = false;

    if (udp_socket_ >= 0) {
        network::close_socket(udp_socket_);
        udp_socket_ = -1;
    }
    if (tcp_socket_ >= 0) {
        network::close_socket(tcp_socket_);
        tcp_socket_ = -1;
    }

    if (udp_thread_.joinable()) udp_thread_.join();
    if (tcp_thread_.joinable()) tcp_thread_.join();
    logger_.info("Image stream bridge stopped");
}

ImageStreamStatus ImageStreamBridge::status() const {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    ImageStreamStatus st;
    st.udp_running = running_ && udp_socket_ >= 0;
    st.tcp_running = running_ && tcp_socket_ >= 0;
    st.last_frame_bytes = last_frame_.size();
    st.last_frame_time = last_frame_time_;
    st.clients = tcp_clients_;
    return st;
}

void ImageStreamBridge::udp_loop() {
    std::vector<std::uint8_t> buffer(2 * 1024 * 1024);
    while (running_) {
        sockaddr_in src{};
        socklen_t len = sizeof(src);
        ssize_t received = recvfrom(udp_socket_, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()),
                                    0, reinterpret_cast<sockaddr*>(&src), &len);
        if (received < 0) {
            if (!running_) break;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            auto bytes = static_cast<std::size_t>(received);
            last_frame_.assign(buffer.begin(), buffer.begin() + bytes);
            last_frame_time_ = std::chrono::system_clock::now();
        }
    }
}

void ImageStreamBridge::tcp_loop() {
    while (running_) {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);
        int client_fd = accept(tcp_socket_, reinterpret_cast<sockaddr*>(&cli), &len);
        if (client_fd < 0) {
            if (!running_) break;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            ++tcp_clients_;
        }

        std::thread([this, client_fd]() {
            logger_.info("TCP viewer connected");
            while (running_) {
                std::vector<std::uint8_t> frame_copy;
                {
                    std::lock_guard<std::mutex> lock(frame_mutex_);
                    frame_copy = last_frame_;
                }
                if (frame_copy.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                ssize_t sent = send(client_fd, reinterpret_cast<const char*>(frame_copy.data()),
                                     static_cast<int>(frame_copy.size()), 0);
                if (sent < 0) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
            network::close_socket(client_fd);
            {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                if (tcp_clients_ > 0) --tcp_clients_;
            }
            logger_.info("TCP viewer disconnected");
        }).detach();
    }
}

}  // namespace core
