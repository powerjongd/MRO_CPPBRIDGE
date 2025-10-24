#include "ui/main_window.hpp"
#include <utility>

#include <chrono>
#include <QtGlobal>

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "utils/settings.hpp"

namespace ui {

MainWindow::MainWindow(settings::ConfigManager& config_manager,
                       settings::AppConfig config,
                       core::ImageStreamBridge& image_bridge,
                       core::GimbalControl& gimbal_control,
                       core::UdpRelay& udp_relay,
                       QWidget* parent)
    : QMainWindow(parent),
      config_manager_(config_manager),
      config_(std::move(config)),
      image_bridge_(image_bridge),
      gimbal_control_(gimbal_control),
      udp_relay_(udp_relay) {
    setWindowTitle(tr("MRO 토탈브리지 제어판"));
    setMinimumSize(960, 640);
    setCentralWidget(build_central_widget());

    status_timer_ = new QTimer(this);
    status_timer_->setInterval(1000);
    connect(status_timer_, &QTimer::timeout, this, &MainWindow::refresh_status);
    status_timer_->start();

    statusBar()->showMessage(tr("상태 정보를 수집하는 중입니다."), 2000);
    refresh_status();
}

QWidget* MainWindow::build_central_widget() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    layout->addWidget(build_image_panel());
    layout->addWidget(build_status_panel());
    layout->addWidget(build_navigation_panel());

    return central;
}

QWidget* MainWindow::build_image_panel() {
    auto* group = new QGroupBox(tr("이미지 스트리밍 모듈"), this);
    auto* layout = new QVBoxLayout(group);
    image_preview_ = new QLabel(tr("이미지 스트림이 준비되는 대로 미리보기가 표시됩니다."), group);
    image_preview_->setAlignment(Qt::AlignCenter);
    image_preview_->setMinimumHeight(260);
    image_preview_->setStyleSheet("background-color: #202020; color: #f0f0f0; border: 1px solid #404040;");

    last_frame_info_ = new QLabel(tr("마지막 프레임: 수신 대기"), group);
    last_frame_info_->setAlignment(Qt::AlignCenter);

    layout->addWidget(image_preview_);
    layout->addWidget(last_frame_info_);
    return group;
}

QWidget* MainWindow::build_status_panel() {
    auto* group = new QGroupBox(tr("현재 상황판"), this);
    auto* form = new QFormLayout(group);
    bridge_status_ = new QLabel(tr("대기 중"), group);
    gimbal_status_ = new QLabel(tr("대기 중"), group);
    relay_status_ = new QLabel(tr("대기 중"), group);

    bridge_status_->setObjectName("bridgeStatusLabel");
    gimbal_status_->setObjectName("gimbalStatusLabel");
    relay_status_->setObjectName("relayStatusLabel");

    form->addRow(tr("이미지 브리지"), bridge_status_);
    form->addRow(tr("짐벌 제어"), gimbal_status_);
    form->addRow(tr("UDP 릴레이"), relay_status_);

    group->setLayout(form);
    return group;
}

QWidget* MainWindow::build_navigation_panel() {
    auto* group = new QGroupBox(tr("주요 모듈 설정"), this);
    auto* layout = new QHBoxLayout(group);
    layout->setSpacing(12);

    auto* image_button = new QPushButton(tr("이미지 모듈"), group);
    auto* gimbal_button = new QPushButton(tr("짐벌 제어"), group);
    auto* relay_button = new QPushButton(tr("릴레이"), group);

    image_button->setMinimumHeight(48);
    gimbal_button->setMinimumHeight(48);
    relay_button->setMinimumHeight(48);

    connect(image_button, &QPushButton::clicked, this, &MainWindow::open_image_settings);
    connect(gimbal_button, &QPushButton::clicked, this, &MainWindow::open_gimbal_settings);
    connect(relay_button, &QPushButton::clicked, this, &MainWindow::open_relay_settings);

    layout->addWidget(image_button, 1);
    layout->addWidget(gimbal_button, 1);
    layout->addWidget(relay_button, 1);
    return group;
}

void MainWindow::refresh_status() {
    const auto img = image_bridge_.status();
    const auto gimbal = gimbal_control_.status();
    const auto relay = udp_relay_.status();

    const bool bridge_online = img.udp_running || img.tcp_running;
    bridge_status_->setText(bridge_online ? tr("동작 중") : tr("중지"));
    bridge_status_->setToolTip(tr("UDP: %1, TCP: %2, 클라이언트: %3")
                                   .arg(img.udp_running ? tr("활성") : tr("비활성"))
                                   .arg(img.tcp_running ? tr("활성") : tr("비활성"))
                                   .arg(static_cast<qulonglong>(img.clients)));

    if (img.last_frame_bytes == 0 || img.last_frame_time.time_since_epoch().count() == 0) {
        image_preview_->setText(tr("아직 수신된 이미지가 없습니다."));
        last_frame_info_->setText(tr("마지막 프레임: 수신 대기"));
    } else {
        const auto now = std::chrono::system_clock::now();
        const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - img.last_frame_time).count();
        image_preview_->setText(tr("최근 프레임 크기: %1 바이트").arg(static_cast<qulonglong>(img.last_frame_bytes)));
        last_frame_info_->setText(tr("마지막 프레임: %1 ms 전")
                                      .arg(age >= 0 ? age : -1));
    }

    gimbal_status_->setText(gimbal.running ? tr("동작 중") : tr("중지"));
    gimbal_status_->setToolTip(tr("Yaw %1°, Pitch %2°, Roll %3°, Zoom %4x")
                                   .arg(QString::number(gimbal.yaw, 'f', 1))
                                   .arg(QString::number(gimbal.pitch, 'f', 1))
                                   .arg(QString::number(gimbal.roll, 'f', 1))
                                   .arg(QString::number(gimbal.zoom, 'f', 1)));

    relay_status_->setText(relay.running ? tr("동작 중") : tr("중지"));
    relay_status_->setToolTip(tr("전달된 패킷 %1개 / %2바이트")
                                  .arg(static_cast<qulonglong>(relay.forwarded_packets))
                                  .arg(static_cast<qulonglong>(relay.forwarded_bytes)));
}

void MainWindow::open_image_settings() {
    std::vector<FieldSpec> fields;
    fields.emplace_back(FieldSpec{"ip", tr("수신 IP"),
                                  QString::fromStdString(config_.bridge.ip),
                                  tr("예: 0.0.0.0"), FieldSpec::Type::IpAddress});
    fields.emplace_back(FieldSpec{"tcp", tr("TCP 포트"),
                                  QString::number(config_.bridge.tcp_port),
                                  tr("예: 9999"), FieldSpec::Type::Port});
    fields.emplace_back(FieldSpec{"udp", tr("UDP 포트"),
                                  QString::number(config_.bridge.udp_port),
                                  tr("예: 9998"), FieldSpec::Type::Port});
    fields.emplace_back(FieldSpec{"realtime", tr("실시간 이미지 경로"),
                                  QString::fromStdString(config_.bridge.realtime_dir),
                                  tr("예: savedata/realtime"), FieldSpec::Type::Text});
    fields.emplace_back(FieldSpec{"predefined", tr("사전 등록 이미지 경로"),
                                  QString::fromStdString(config_.bridge.predefined_dir),
                                  tr("예: PreDefinedImageSet"), FieldSpec::Type::Text});

    ModuleConfigDialog dialog(tr("이미지 스트리밍 설정"), std::move(fields), this);
    if (dialog.exec() == QDialog::Accepted) {
        const auto values = dialog.values();
        config_.bridge.ip = values.at("ip").toStdString();
        config_.bridge.tcp_port = values.at("tcp").toInt();
        config_.bridge.udp_port = values.at("udp").toInt();
        config_.bridge.realtime_dir = values.at("realtime").toStdString();
        config_.bridge.predefined_dir = values.at("predefined").toStdString();
        config_manager_.save(config_);
        show_status_message(tr("이미지 스트리밍 설정을 저장했습니다."));
    }
}

void MainWindow::open_gimbal_settings() {
    std::vector<FieldSpec> fields;
    fields.emplace_back(FieldSpec{"bind_ip", tr("수신 IP"),
                                  QString::fromStdString(config_.gimbal.bind_ip),
                                  tr("예: 0.0.0.0"), FieldSpec::Type::IpAddress});
    fields.emplace_back(FieldSpec{"bind_port", tr("수신 포트"),
                                  QString::number(config_.gimbal.bind_port),
                                  tr("예: 10705"), FieldSpec::Type::Port});
    fields.emplace_back(FieldSpec{"generator_ip", tr("제너레이터 IP"),
                                  QString::fromStdString(config_.gimbal.generator_ip),
                                  tr("예: 127.0.0.1"), FieldSpec::Type::IpAddress});
    fields.emplace_back(FieldSpec{"generator_port", tr("제너레이터 포트"),
                                  QString::number(config_.gimbal.generator_port),
                                  tr("예: 10706"), FieldSpec::Type::Port});

    ModuleConfigDialog dialog(tr("짐벌 제어 설정"), std::move(fields), this);
    if (dialog.exec() == QDialog::Accepted) {
        const auto values = dialog.values();
        config_.gimbal.bind_ip = values.at("bind_ip").toStdString();
        config_.gimbal.bind_port = values.at("bind_port").toInt();
        config_.gimbal.generator_ip = values.at("generator_ip").toStdString();
        config_.gimbal.generator_port = values.at("generator_port").toInt();
        config_manager_.save(config_);
        show_status_message(tr("짐벌 제어 설정을 저장했습니다."));
    }
}

void MainWindow::open_relay_settings() {
    std::vector<FieldSpec> fields;
    fields.emplace_back(FieldSpec{"bind_ip", tr("수신 IP"),
                                  QString::fromStdString(config_.relay.bind_ip),
                                  tr("예: 0.0.0.0"), FieldSpec::Type::IpAddress});
    fields.emplace_back(FieldSpec{"bind_port", tr("수신 포트"),
                                  QString::number(config_.relay.bind_port),
                                  tr("예: 10707"), FieldSpec::Type::Port});
    fields.emplace_back(FieldSpec{"raw_ip", tr("RAW 대상 IP"),
                                  QString::fromStdString(config_.relay.raw_ip),
                                  tr("예: 127.0.0.1"), FieldSpec::Type::IpAddress});
    fields.emplace_back(FieldSpec{"raw_port", tr("RAW 대상 포트"),
                                  QString::number(config_.relay.raw_port),
                                  tr("예: 10708"), FieldSpec::Type::Port});
    fields.emplace_back(FieldSpec{"proc_ip", tr("PROC 대상 IP"),
                                  QString::fromStdString(config_.relay.proc_ip),
                                  tr("예: 127.0.0.1"), FieldSpec::Type::IpAddress});
    fields.emplace_back(FieldSpec{"proc_port", tr("PROC 대상 포트"),
                                  QString::number(config_.relay.proc_port),
                                  tr("예: 10709"), FieldSpec::Type::Port});

    ModuleConfigDialog dialog(tr("UDP 릴레이 설정"), std::move(fields), this);
    if (dialog.exec() == QDialog::Accepted) {
        const auto values = dialog.values();
        config_.relay.bind_ip = values.at("bind_ip").toStdString();
        config_.relay.bind_port = values.at("bind_port").toInt();
        config_.relay.raw_ip = values.at("raw_ip").toStdString();
        config_.relay.raw_port = values.at("raw_port").toInt();
        config_.relay.proc_ip = values.at("proc_ip").toStdString();
        config_.relay.proc_port = values.at("proc_port").toInt();
        config_manager_.save(config_);
        show_status_message(tr("UDP 릴레이 설정을 저장했습니다."));
    }
}

void MainWindow::show_status_message(const QString& message) {
    if (auto* bar = statusBar()) {
        bar->showMessage(message, 3000);
    }
}

}  // namespace ui

