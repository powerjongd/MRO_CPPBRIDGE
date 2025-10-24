#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "utils/logger.hpp"
#include "utils/settings.hpp"

namespace core {

struct GimbalStatus {
    bool running = false;
    double yaw = 0.0;
    double pitch = 0.0;
    double roll = 0.0;
    double zoom = 1.0;
};

class GimbalControl {
public:
    GimbalControl(const settings::GimbalSettings& cfg, logging::Logger& logger);
    ~GimbalControl();

    void start();
    void stop();

    void update_pose(double yaw_deg, double pitch_deg, double roll_deg, double zoom_level);

    GimbalStatus status() const;

private:
    void worker();

    settings::GimbalSettings config_;
    logging::Logger& logger_;

    std::atomic<bool> running_{false};
    std::thread worker_thread_;

    mutable std::mutex pose_mutex_;
    double yaw_ = 0.0;
    double pitch_ = 0.0;
    double roll_ = 0.0;
    double zoom_ = 1.0;
};

}  // namespace core
