#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "utils/json.hpp"

namespace settings {

struct BridgeSettings {
    std::string ip = "0.0.0.0";
    int tcp_port = 9999;
    int udp_port = 9998;
    std::string realtime_dir;
    std::string predefined_dir;
    std::string image_source_mode = "realtime";  // realtime | predefined
    bool console_echo = true;
    bool show_hud = true;
};

struct GimbalSettings {
    std::string bind_ip = "0.0.0.0";
    int bind_port = 10705;
    std::string generator_ip = "127.0.0.1";
    int generator_port = 10706;
    int sensor_type = 0;
    int sensor_id = 1;
    std::string control_method = "tcp";  // tcp | mavlink
    bool show_packets = false;
};

struct RelaySettings {
    std::string bind_ip = "0.0.0.0";
    int bind_port = 10707;
    std::string raw_ip = "127.0.0.1";
    int raw_port = 10708;
    std::string proc_ip = "127.0.0.1";
    int proc_port = 10709;
    bool enable = true;
    bool log_packets = false;
};

struct RoverSettings {
    bool enable_logging = false;
    std::string log_directory;
};

struct AppConfig {
    BridgeSettings bridge;
    GimbalSettings gimbal;
    RelaySettings relay;
    RoverSettings rover;
    bool console_hud = true;
    double hud_interval = 1.0;

    mini_json::Value to_json() const;
    static AppConfig from_json(const mini_json::Value& value, const std::string& base_dir);
};

class ConfigManager {
public:
    explicit ConfigManager(std::string base_dir);

    AppConfig load();
    void save(const AppConfig& config) const;

    const std::string& base_dir() const { return base_dir_; }
    std::string config_path() const;

private:
    static AppConfig defaults(const std::string& base_dir);
    static void ensure_directory(const std::string& path);

    std::string base_dir_;
};

std::string program_directory();
void set_program_directory(const std::string& dir);
std::string default_save_directory();
bool has_display();
std::string prompt(const std::string& text, const std::string& default_value);
void atomic_write(const std::string& path, const std::string& data);
std::string load_text(const std::string& path);

}  // namespace settings
