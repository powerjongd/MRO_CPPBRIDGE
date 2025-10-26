# MRO TotalBridge (C++ Edition)

이 저장소는 MORAI 시뮬레이터와 외부 시스템 사이의 데이터 허브를 C++17로 재구현한 버전입니다. 기존 파이썬 구현과 동일하게 이미지
스트림, 짐벌 제어, UDP 릴레이(가제보/로버) 및 패킷 로깅을 제공하며, 모든 설정은 실행 파일과 같은 디렉터리의 `savedata/config.json`
으로 관리됩니다.

## 구성 요소

| 모듈 | 설명 |
| --- | --- |
| `ImageStreamBridge` | UDP 로 JPEG 프레임을 수신하고 최신 프레임을 TCP 뷰어에게 송신합니다. |
| `GimbalControl` | 짐벌 목표 자세/줌 값을 UDP 패킷으로 주기적으로 송신합니다. |
| `UdpRelay` | Gazebo/센서 데이터 UDP를 RAW/PROC 두 목적지로 중계하고, 필요 시 패킷을 로깅합니다. |
| `RoverRelayLogger` | 로버/가제보 패킷을 타임스탬프와 함께 로그 파일로 저장합니다. |
| `ConfigManager` | `savedata/config.json`을 원자적으로 읽고/저장하며 기본값과 마이그레이션을 관리합니다. |

## 빌드 방법

### 요구 사항

- CMake 3.16 이상
- C++17 컴파일러 (GCC 9+, Clang 10+, MSVC 2019 이상)
- Qt 6 Widgets 모듈 (예: Qt 6.5+)

### 빌드 절차

#### Linux / macOS

```bash
# Qt 설치 후 (예: qtbase, qttools 패키지) CMake 가 Qt6Config.cmake 를 찾을 수 있도록 준비합니다.
export CMAKE_PREFIX_PATH="/opt/Qt/6.5.0/gcc_64"  # Qt 가 설치된 경로에 맞게 수정

cmake -S . -B build
cmake --build build
```

#### Windows (Visual Studio 포함)

1. [Qt Online Installer](https://www.qt.io/download) 로 Qt 6.x (MSVC 툴체인 대상) 과 "MSVC 2019 64-bit" 컴포넌트를 설치합니다.
2. "x64 Native Tools Command Prompt" 또는 Visual Studio Developer PowerShell 을 열고, Qt 설치 경로를 `CMAKE_PREFIX_PATH` 로 지정합니다.

   ```powershell
   set CMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64
   ```

3. CMake 를 Visual Studio 제너레이터로 실행합니다.

   ```powershell
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```

Visual Studio IDE 를 통해 빌드하려면 3단계에서 생성된 `build/unified_bridge.sln` 을 열어 원하는 구성(Debug/Release)을 선택하고 빌드하면 됩니다. 빌드가 완료되면 `build/Release/unified_bridge.exe` (또는 선택한 구성 폴더) 가 생성됩니다.

Linux/macOS 에서는 `build/unified_bridge`, Windows 에서는 `build/Release/unified_bridge.exe` 가 기본 실행 파일 경로입니다.

## 실행 방법

```bash
./build/unified_bridge [옵션]
```

### 주요 옵션

- `--console-hud` / `--no-console-hud` : 콘솔 HUD 활성/비활성
- `--hud-interval <초>` : HUD 업데이트 간격 (기본 1.0s)
- `--bridge-ip`, `--bridge-tcp`, `--bridge-udp` : 이미지 스트림 바인드 설정
- `--realtime-dir`, `--predefined-dir` : 이미지 저장 디렉터리 지정
- `--gimbal-*`, `--gen-*`, `--sensor-*` : 짐벌 UDP 파라미터
- `--relay-*` : UDP 릴레이 소스/목적지 설정
- `--relay-log` / `--no-relay-log` : Gazebo 패킷 로깅 On/Off
- `--enable-rover-logging` / `--disable-rover-logging` : 로버 패킷 로깅 On/Off

실행 중 `Ctrl+C` 로 종료할 수 있으며, 모든 모듈은 종료 시 자동으로 정리됩니다.

## 설정 파일

`ConfigManager` 는 실행 디렉터리 아래의 `savedata/config.json` 을 사용합니다. 파일이 없으면 기본값을 생성하며, 경로는 플랫폼에 관계없이
실행 파일과 동일한 폴더를 기준으로 합니다. 로그 파일은 `savedata/rover/` 또는 `savedata/gazebo/` 에 저장됩니다.

## 로깅 정책

가제보 패킷 로깅(`--relay-log`)과 로버 패킷 로깅(`--enable-rover-logging`)은 동시에 활성화할 수 없습니다. 두 옵션이 모두 켜지면 가제보
로깅이 자동으로 비활성화됩니다. 로그 파일 형식은 아래와 같습니다.

```
YYYY-MM-DD HH:MM:SS.mmm\tlen=<payload length>\t<hex payload>
```

## 라이선스

이 프로젝트는 조직 내부 사용을 전제로 하며, 별도의 라이선스 문서가 없다면 배포 전에 담당자에게 문의하세요.
