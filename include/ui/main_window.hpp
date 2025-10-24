#pragma once

#include <memory>

#include <QMainWindow>

#include "core/gimbal_control.hpp"
#include "core/image_stream_bridge.hpp"
#include "core/udp_relay.hpp"
#include "ui/module_config_dialog.hpp"
#include "utils/settings.hpp"

class QLabel;
class QTimer;

namespace settings {
class ConfigManager;
}

namespace ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(settings::ConfigManager& config_manager,
               settings::AppConfig config,
               core::ImageStreamBridge& image_bridge,
               core::GimbalControl& gimbal_control,
               core::UdpRelay& udp_relay,
               QWidget* parent = nullptr);

private slots:
    void refresh_status();
    void open_image_settings();
    void open_gimbal_settings();
    void open_relay_settings();

private:
    QWidget* build_central_widget();
    QWidget* build_image_panel();
    QWidget* build_status_panel();
    QWidget* build_navigation_panel();

    void show_status_message(const QString& message);

    settings::ConfigManager& config_manager_;
    settings::AppConfig config_;
    core::ImageStreamBridge& image_bridge_;
    core::GimbalControl& gimbal_control_;
    core::UdpRelay& udp_relay_;

    QLabel* image_preview_ = nullptr;
    QLabel* bridge_status_ = nullptr;
    QLabel* gimbal_status_ = nullptr;
    QLabel* relay_status_ = nullptr;
    QLabel* last_frame_info_ = nullptr;

    QTimer* status_timer_ = nullptr;
};

}  // namespace ui

