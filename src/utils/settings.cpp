#include "utils/settings.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <iterator>

namespace fs = std::filesystem;

namespace settings {

namespace {

std::string& stored_program_dir() {
    static std::string dir = fs::current_path().string();
    return dir;
}

std::string safe_env(const char* name) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

mini_json::Value::Object object_or(const mini_json::Value& value) {
    if (value.is_object()) {
        return value.as_object();
    }
    return {};
}

std::string join_path(const std::string& a, const std::string& b) {
    return (fs::path(a) / b).string();
}

}  // namespace

mini_json::Value AppConfig::to_json() const {
    mini_json::Value::Object root;

    mini_json::Value::Object bridge_obj;
    bridge_obj["ip"] = bridge.ip;
    bridge_obj["tcp_port"] = static_cast<double>(bridge.tcp_port);
    bridge_obj["udp_port"] = static_cast<double>(bridge.udp_port);
    bridge_obj["realtime_dir"] = bridge.realtime_dir;
    bridge_obj["predefined_dir"] = bridge.predefined_dir;
    bridge_obj["image_source_mode"] = bridge.image_source_mode;
    bridge_obj["console_echo"] = bridge.console_echo;
    bridge_obj["show_hud"] = bridge.show_hud;

    mini_json::Value::Object gimbal_obj;
    gimbal_obj["bind_ip"] = gimbal.bind_ip;
    gimbal_obj["bind_port"] = static_cast<double>(gimbal.bind_port);
    gimbal_obj["generator_ip"] = gimbal.generator_ip;
    gimbal_obj["generator_port"] = static_cast<double>(gimbal.generator_port);
    gimbal_obj["sensor_type"] = static_cast<double>(gimbal.sensor_type);
    gimbal_obj["sensor_id"] = static_cast<double>(gimbal.sensor_id);
    gimbal_obj["gimbal_control_method"] = gimbal.control_method;
    gimbal_obj["show_packets"] = gimbal.show_packets;

    mini_json::Value::Object relay_obj;
    relay_obj["bind_ip"] = relay.bind_ip;
    relay_obj["bind_port"] = static_cast<double>(relay.bind_port);
    relay_obj["raw_ip"] = relay.raw_ip;
    relay_obj["raw_port"] = static_cast<double>(relay.raw_port);
    relay_obj["proc_ip"] = relay.proc_ip;
    relay_obj["proc_port"] = static_cast<double>(relay.proc_port);
    relay_obj["enable"] = relay.enable;
    relay_obj["log_packets"] = relay.log_packets;

    mini_json::Value::Object rover_obj;
    rover_obj["enable_logging"] = rover.enable_logging;
    rover_obj["log_directory"] = rover.log_directory;

    root["bridge"] = bridge_obj;
    root["gimbal"] = gimbal_obj;
    root["relay"] = relay_obj;
    root["rover"] = rover_obj;
    root["console_hud"] = console_hud;
    root["hud_interval"] = hud_interval;

    return root;
}

AppConfig AppConfig::from_json(const mini_json::Value& value, const std::string& base_dir) {
    AppConfig cfg;
    cfg.bridge.realtime_dir = join_path(base_dir, "SaveFile");
    cfg.bridge.predefined_dir = join_path(base_dir, "PreDefinedImageSet");
    cfg.rover.log_directory = join_path(base_dir, "savedata");
    if (!value.is_object()) {
        return cfg;
    }
    auto root = value.as_object();

    auto bridge_it = root.find("bridge");
    auto bridge_obj = bridge_it != root.end() ? object_or(bridge_it->second) : mini_json::Value::Object{};
    if (!bridge_obj.empty()) {
        if (bridge_obj.count("ip")) cfg.bridge.ip = bridge_obj["ip"].as_string(cfg.bridge.ip);
        if (bridge_obj.count("tcp_port")) cfg.bridge.tcp_port = static_cast<int>(bridge_obj["tcp_port"].as_number(cfg.bridge.tcp_port));
        if (bridge_obj.count("udp_port")) cfg.bridge.udp_port = static_cast<int>(bridge_obj["udp_port"].as_number(cfg.bridge.udp_port));
        if (bridge_obj.count("realtime_dir")) cfg.bridge.realtime_dir = bridge_obj["realtime_dir"].as_string(cfg.bridge.realtime_dir);
        if (bridge_obj.count("predefined_dir")) cfg.bridge.predefined_dir = bridge_obj["predefined_dir"].as_string(cfg.bridge.predefined_dir);
        if (bridge_obj.count("image_source_mode")) cfg.bridge.image_source_mode = bridge_obj["image_source_mode"].as_string(cfg.bridge.image_source_mode);
        if (bridge_obj.count("console_echo")) cfg.bridge.console_echo = bridge_obj["console_echo"].as_bool(cfg.bridge.console_echo);
        if (bridge_obj.count("show_hud")) cfg.bridge.show_hud = bridge_obj["show_hud"].as_bool(cfg.bridge.show_hud);
    }

    auto gimbal_it = root.find("gimbal");
    auto gimbal_obj = gimbal_it != root.end() ? object_or(gimbal_it->second) : mini_json::Value::Object{};
    if (!gimbal_obj.empty()) {
        if (gimbal_obj.count("bind_ip")) cfg.gimbal.bind_ip = gimbal_obj["bind_ip"].as_string(cfg.gimbal.bind_ip);
        if (gimbal_obj.count("bind_port")) cfg.gimbal.bind_port = static_cast<int>(gimbal_obj["bind_port"].as_number(cfg.gimbal.bind_port));
        if (gimbal_obj.count("generator_ip")) cfg.gimbal.generator_ip = gimbal_obj["generator_ip"].as_string(cfg.gimbal.generator_ip);
        if (gimbal_obj.count("generator_port")) cfg.gimbal.generator_port = static_cast<int>(gimbal_obj["generator_port"].as_number(cfg.gimbal.generator_port));
        if (gimbal_obj.count("sensor_type")) cfg.gimbal.sensor_type = static_cast<int>(gimbal_obj["sensor_type"].as_number(cfg.gimbal.sensor_type));
        if (gimbal_obj.count("sensor_id")) cfg.gimbal.sensor_id = static_cast<int>(gimbal_obj["sensor_id"].as_number(cfg.gimbal.sensor_id));
        if (gimbal_obj.count("gimbal_control_method")) cfg.gimbal.control_method = gimbal_obj["gimbal_control_method"].as_string(cfg.gimbal.control_method);
        if (gimbal_obj.count("show_packets")) cfg.gimbal.show_packets = gimbal_obj["show_packets"].as_bool(cfg.gimbal.show_packets);
    }

    auto relay_it = root.find("relay");
    auto relay_obj = relay_it != root.end() ? object_or(relay_it->second) : mini_json::Value::Object{};
    if (!relay_obj.empty()) {
        if (relay_obj.count("bind_ip")) cfg.relay.bind_ip = relay_obj["bind_ip"].as_string(cfg.relay.bind_ip);
        if (relay_obj.count("bind_port")) cfg.relay.bind_port = static_cast<int>(relay_obj["bind_port"].as_number(cfg.relay.bind_port));
        if (relay_obj.count("raw_ip")) cfg.relay.raw_ip = relay_obj["raw_ip"].as_string(cfg.relay.raw_ip);
        if (relay_obj.count("raw_port")) cfg.relay.raw_port = static_cast<int>(relay_obj["raw_port"].as_number(cfg.relay.raw_port));
        if (relay_obj.count("proc_ip")) cfg.relay.proc_ip = relay_obj["proc_ip"].as_string(cfg.relay.proc_ip);
        if (relay_obj.count("proc_port")) cfg.relay.proc_port = static_cast<int>(relay_obj["proc_port"].as_number(cfg.relay.proc_port));
        if (relay_obj.count("enable")) cfg.relay.enable = relay_obj["enable"].as_bool(cfg.relay.enable);
        if (relay_obj.count("log_packets")) cfg.relay.log_packets = relay_obj["log_packets"].as_bool(cfg.relay.log_packets);
    }

    auto rover_it = root.find("rover");
    auto rover_obj = rover_it != root.end() ? object_or(rover_it->second) : mini_json::Value::Object{};
    if (!rover_obj.empty()) {
        if (rover_obj.count("enable_logging")) cfg.rover.enable_logging = rover_obj["enable_logging"].as_bool(cfg.rover.enable_logging);
        if (rover_obj.count("log_directory")) cfg.rover.log_directory = rover_obj["log_directory"].as_string(cfg.rover.log_directory);
    }

    if (auto it = root.find("console_hud"); it != root.end()) cfg.console_hud = it->second.as_bool(cfg.console_hud);
    if (auto it = root.find("hud_interval"); it != root.end()) cfg.hud_interval = it->second.as_number(cfg.hud_interval);

    return cfg;
}

ConfigManager::ConfigManager(std::string base_dir) : base_dir_(std::move(base_dir)) {}

AppConfig ConfigManager::defaults(const std::string& base_dir) {
    AppConfig cfg;
    cfg.bridge.realtime_dir = join_path(base_dir, "SaveFile");
    cfg.bridge.predefined_dir = join_path(base_dir, "PreDefinedImageSet");
    cfg.rover.log_directory = join_path(base_dir, "savedata");
    return cfg;
}

void ConfigManager::ensure_directory(const std::string& path) {
    fs::create_directories(path);
}

AppConfig ConfigManager::load() {
    auto cfg = defaults(base_dir_);
    auto path = config_path();
    try {
        std::string text = load_text(path);
        if (!text.empty()) {
            cfg = AppConfig::from_json(mini_json::parse(text), base_dir_);
        }
    } catch (const std::exception&) {
        // corrupted config - keep defaults
    }
    ensure_directory(cfg.bridge.realtime_dir);
    ensure_directory(cfg.bridge.predefined_dir);
    ensure_directory(cfg.rover.log_directory);
    return cfg;
}

void ConfigManager::save(const AppConfig& config) const {
    ensure_directory(base_dir_);
    ensure_directory(config.bridge.realtime_dir);
    ensure_directory(config.bridge.predefined_dir);
    ensure_directory(config.rover.log_directory);

    std::string json = config.to_json().dump(2);
    atomic_write(config_path(), json);
}

std::string ConfigManager::config_path() const {
    return (fs::path(base_dir_) / "savedata" / "config.json").string();
}

std::string program_directory() {
    return stored_program_dir();
}

void set_program_directory(const std::string& dir) {
    if (!dir.empty()) {
        stored_program_dir() = dir;
    }
}

std::string default_save_directory() {
    return (fs::path(program_directory()) / "savedata").string();
}

bool has_display() {
#ifdef _WIN32
    return true;
#else
    return !safe_env("DISPLAY").empty();
#endif
}

std::string prompt(const std::string& text, const std::string& default_value) {
    std::cout << text << " [" << default_value << "]: ";
    std::string line;
    if (!std::getline(std::cin, line) || line.empty()) {
        return default_value;
    }
    return line;
}

void atomic_write(const std::string& path, const std::string& data) {
    fs::path dst(path);
    fs::create_directories(dst.parent_path());
    auto tmp = dst;
    tmp += ".tmp";
    {
        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        ofs << data;
    }
    std::error_code ec;
    fs::remove(dst, ec);
    fs::rename(tmp, dst, ec);
    if (ec) {
        throw std::runtime_error("Failed to persist config: " + ec.message());
    }
}

std::string load_text(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) return {};
    return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

}  // namespace settings
