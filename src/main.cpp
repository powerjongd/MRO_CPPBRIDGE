#include <atomic>
#include <csignal>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <stdexcept>
#include <QApplication>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "core/gimbal_control.hpp"
#include "core/image_stream_bridge.hpp"
#include "ui/main_window.hpp"
#include "core/udp_relay.hpp"
#include "utils/logger.hpp"
#include "utils/settings.hpp"

namespace {

std::atomic<bool> g_should_exit{false};

void signal_handler(int) {
    g_should_exit = true;
}

struct CliOptions {
    bool no_gui = false;
    std::optional<bool> console_hud;
    std::optional<double> hud_interval;

    std::optional<std::string> bridge_ip;
    std::optional<int> bridge_tcp;
    std::optional<int> bridge_udp;
    std::optional<std::string> realtime_dir;
    std::optional<std::string> predefined_dir;
    std::optional<std::string> image_mode;

    std::optional<std::string> gimbal_bind_ip;
    std::optional<int> gimbal_bind_port;
    std::optional<std::string> generator_ip;
    std::optional<int> generator_port;
    std::optional<int> sensor_type;
    std::optional<int> sensor_id;
    std::optional<std::string> control_method;
    std::optional<bool> show_packets;

    std::optional<std::string> relay_bind_ip;
    std::optional<int> relay_bind_port;
    std::optional<std::string> relay_raw_ip;
    std::optional<int> relay_raw_port;
    std::optional<std::string> relay_proc_ip;
    std::optional<int> relay_proc_port;
    std::optional<bool> relay_log;

    std::optional<bool> rover_logging;
};

bool parse_cli(int argc, char** argv, CliOptions& out, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto require_value = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                error = "Missing value for option " + name;
                throw std::runtime_error(error);
            }
            return argv[++i];
        };
        try {
            if (arg == "--no-gui") {
                out.no_gui = true;
            } else if (arg == "--console-hud") {
                out.console_hud = true;
            } else if (arg == "--no-console-hud") {
                out.console_hud = false;
            } else if (arg == "--hud-interval") {
                out.hud_interval = std::stod(require_value(arg));
            } else if (arg == "--bridge-ip") {
                out.bridge_ip = require_value(arg);
            } else if (arg == "--bridge-tcp") {
                out.bridge_tcp = std::stoi(require_value(arg));
            } else if (arg == "--bridge-udp") {
                out.bridge_udp = std::stoi(require_value(arg));
            } else if (arg == "--realtime-dir") {
                out.realtime_dir = require_value(arg);
            } else if (arg == "--predefined-dir") {
                out.predefined_dir = require_value(arg);
            } else if (arg == "--image-source-mode") {
                out.image_mode = require_value(arg);
            } else if (arg == "--gimbal-bind-ip") {
                out.gimbal_bind_ip = require_value(arg);
            } else if (arg == "--gimbal-bind-port") {
                out.gimbal_bind_port = std::stoi(require_value(arg));
            } else if (arg == "--gen-ip") {
                out.generator_ip = require_value(arg);
            } else if (arg == "--gen-port") {
                out.generator_port = std::stoi(require_value(arg));
            } else if (arg == "--sensor-type") {
                out.sensor_type = std::stoi(require_value(arg));
            } else if (arg == "--sensor-id") {
                out.sensor_id = std::stoi(require_value(arg));
            } else if (arg == "--gimbal-control-method") {
                out.control_method = require_value(arg);
            } else if (arg == "--show-gimbal-packets") {
                out.show_packets = true;
            } else if (arg == "--relay-bind-ip") {
                out.relay_bind_ip = require_value(arg);
            } else if (arg == "--relay-port") {
                out.relay_bind_port = std::stoi(require_value(arg));
            } else if (arg == "--relay-raw-ip") {
                out.relay_raw_ip = require_value(arg);
            } else if (arg == "--relay-raw-port") {
                out.relay_raw_port = std::stoi(require_value(arg));
            } else if (arg == "--relay-proc-ip") {
                out.relay_proc_ip = require_value(arg);
            } else if (arg == "--relay-proc-port") {
                out.relay_proc_port = std::stoi(require_value(arg));
            } else if (arg == "--relay-log") {
                out.relay_log = true;
            } else if (arg == "--no-relay-log") {
                out.relay_log = false;
            } else if (arg == "--enable-rover-logging") {
                out.rover_logging = true;
            } else if (arg == "--disable-rover-logging") {
                out.rover_logging = false;
            } else if (arg == "--help" || arg == "-h") {
                return false;
            } else {
                error = "Unknown argument: " + arg;
                return false;
            }
        } catch (const std::exception&) {
            if (error.empty()) error = "Invalid value for option " + arg;
            return false;
        }
    }
    return true;
}

void print_usage() {
    std::cout << "Unified Bridge (C++)\n"
              << "Options:\n"
              << "  --no-gui                 Force headless mode\n"
              << "  --console-hud           Enable console HUD\n"
              << "  --no-console-hud        Disable console HUD\n"
              << "  --hud-interval <sec>    HUD update interval\n"
              << "  --bridge-ip <ip>        Bridge bind IP\n"
              << "  --bridge-tcp <port>     Bridge TCP port\n"
              << "  --bridge-udp <port>     Bridge UDP port\n"
              << "  --realtime-dir <path>   Directory for realtime captures\n"
              << "  --predefined-dir <path> Directory for predefined captures\n"
              << "  --image-source-mode <realtime|predefined>\n"
              << "  --gimbal-bind-ip <ip>   Gimbal listener IP\n"
              << "  --gimbal-bind-port <port>\n"
              << "  --gen-ip <ip>           Generator IP\n"
              << "  --gen-port <port>       Generator port\n"
              << "  --sensor-type <int>     Sensor type code\n"
              << "  --sensor-id <int>       Sensor identifier\n"
              << "  --gimbal-control-method <tcp|mavlink>\n"
              << "  --show-gimbal-packets   Print raw packets\n"
              << "  --relay-bind-ip <ip>    Relay bind IP\n"
              << "  --relay-port <port>     Relay bind port\n"
              << "  --relay-raw-ip <ip>     Relay RAW target IP\n"
              << "  --relay-raw-port <port> Relay RAW target port\n"
              << "  --relay-proc-ip <ip>    Relay PROC target IP\n"
              << "  --relay-proc-port <port> Relay PROC target port\n"
              << "  --relay-log             Enable Gazebo packet logging\n"
              << "  --no-relay-log          Disable Gazebo packet logging\n"
              << "  --enable-rover-logging  Enable rover relay logging\n"
              << "  --disable-rover-logging Disable rover relay logging\n"
              << std::endl;
}

void apply_cli(const CliOptions& cli, settings::AppConfig& cfg) {
    if (cli.console_hud) cfg.console_hud = *cli.console_hud;
    if (cli.hud_interval) cfg.hud_interval = *cli.hud_interval;

    if (cli.bridge_ip) cfg.bridge.ip = *cli.bridge_ip;
    if (cli.bridge_tcp) cfg.bridge.tcp_port = *cli.bridge_tcp;
    if (cli.bridge_udp) cfg.bridge.udp_port = *cli.bridge_udp;
    if (cli.realtime_dir) cfg.bridge.realtime_dir = *cli.realtime_dir;
    if (cli.predefined_dir) cfg.bridge.predefined_dir = *cli.predefined_dir;
    if (cli.image_mode) cfg.bridge.image_source_mode = *cli.image_mode;

    if (cli.gimbal_bind_ip) cfg.gimbal.bind_ip = *cli.gimbal_bind_ip;
    if (cli.gimbal_bind_port) cfg.gimbal.bind_port = *cli.gimbal_bind_port;
    if (cli.generator_ip) cfg.gimbal.generator_ip = *cli.generator_ip;
    if (cli.generator_port) cfg.gimbal.generator_port = *cli.generator_port;
    if (cli.sensor_type) cfg.gimbal.sensor_type = *cli.sensor_type;
    if (cli.sensor_id) cfg.gimbal.sensor_id = *cli.sensor_id;
    if (cli.control_method) cfg.gimbal.control_method = *cli.control_method;
    if (cli.show_packets) cfg.gimbal.show_packets = *cli.show_packets;

    if (cli.relay_bind_ip) cfg.relay.bind_ip = *cli.relay_bind_ip;
    if (cli.relay_bind_port) cfg.relay.bind_port = *cli.relay_bind_port;
    if (cli.relay_raw_ip) cfg.relay.raw_ip = *cli.relay_raw_ip;
    if (cli.relay_raw_port) cfg.relay.raw_port = *cli.relay_raw_port;
    if (cli.relay_proc_ip) cfg.relay.proc_ip = *cli.relay_proc_ip;
    if (cli.relay_proc_port) cfg.relay.proc_port = *cli.relay_proc_port;
    if (cli.relay_log) cfg.relay.log_packets = *cli.relay_log;

    if (cli.rover_logging) cfg.rover.enable_logging = *cli.rover_logging;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }
#endif

    CliOptions cli;
    std::string error;
    if (!parse_cli(argc, argv, cli, error)) {
        if (!error.empty()) {
            std::cerr << error << std::endl;
        }
        print_usage();
        return error.empty() ? 0 : 1;
    }

    std::filesystem::path program_dir;
    if (argc > 0) {
        try {
            program_dir = std::filesystem::absolute(argv[0]).parent_path();
        } catch (...) {
            program_dir = std::filesystem::current_path();
        }
    } else {
        program_dir = std::filesystem::current_path();
    }

    settings::set_program_directory(program_dir.string());

    logging::Logger logger("UnifiedBridge");
    if (!settings::has_display()) {
        logger.warn("DISPLAY not detected. Running in headless mode.");
        cli.no_gui = true;
    }

    settings::ConfigManager config_manager(program_dir.string());
    auto config = config_manager.load();
    apply_cli(cli, config);

    if (config.relay.log_packets && config.rover.enable_logging) {
        logger.warn("Relay logging and rover logging cannot be enabled simultaneously. Disabling relay logging.");
        config.relay.log_packets = false;
    }

    config_manager.save(config);

    std::unique_ptr<core::RoverRelayLogger> packet_logger;
    if (config.relay.log_packets || config.rover.enable_logging) {
        std::filesystem::path base = config.rover.log_directory.empty()
                                          ? program_dir / "savedata"
                                          : std::filesystem::path(config.rover.log_directory);
        if (config.relay.log_packets) {
            base /= "gazebo";
        } else {
            base /= "rover";
        }
        packet_logger = std::make_unique<core::RoverRelayLogger>(logger, base.string());
        packet_logger->start();
    }

    core::ImageStreamBridge image_bridge(config.bridge, logger);
    core::GimbalControl gimbal(config.gimbal, logger);
    core::UdpRelay relay(config.relay, logger, packet_logger.get());

    if (config.bridge.show_hud || config.console_hud) {
        logger.info("Console HUD enabled");
    }

    image_bridge.start();
    gimbal.start();
    relay.start();

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::atomic<bool> hud_running{false};
    std::thread hud_thread;
    if (config.console_hud) {
        hud_running = true;
        hud_thread = std::thread([&]() {
            while (!g_should_exit && hud_running.load()) {
                auto img = image_bridge.status();
                auto gib = gimbal.status();
                auto rel = relay.status();

                auto now = std::chrono::system_clock::now();
                double age_ms = 0.0;
                if (img.last_frame_time.time_since_epoch().count() > 0) {
                    age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - img.last_frame_time).count();
                } else {
                    age_ms = -1.0;
                }

                std::ostringstream oss;
                oss << "HUD | UDP:" << (img.udp_running ? "on" : "off")
                    << " TCP:" << (img.tcp_running ? "on" : "off")
                    << " Clients:" << img.clients
                    << " LastFrame:" << img.last_frame_bytes << "B";
                if (age_ms >= 0) {
                    oss << " (" << static_cast<int>(age_ms) << "ms ago)";
                } else {
                    oss << " (n/a)";
                }
                oss << " | Gimbal yaw:" << gib.yaw << " pitch:" << gib.pitch
                    << " roll:" << gib.roll << " zoom:" << gib.zoom
                    << " | Relay packets:" << rel.forwarded_packets
                    << " bytes:" << rel.forwarded_bytes;

                std::cout << oss.str() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(config.hud_interval * 1000)));
            }
        });
    }

    int exit_code = 0;
    if (cli.no_gui) {
        logger.info("Unified Bridge running. Press Ctrl+C to exit.");
        while (!g_should_exit) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } else {
        QApplication app(argc, argv);
        ui::MainWindow window(config_manager, config, image_bridge, gimbal, relay);
        window.show();
        logger.info("Unified Bridge GUI initialized. Close the window to exit.");
        exit_code = app.exec();
        g_should_exit = true;
    }

    hud_running = false;
    if (hud_thread.joinable()) hud_thread.join();

    relay.stop();
    gimbal.stop();
    image_bridge.stop();

    if (packet_logger) {
        packet_logger->stop();
    }

#ifdef _WIN32
    WSACleanup();
#endif
    logger.info("Shutdown complete");
    return exit_code;
}
