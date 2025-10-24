#include "utils/logger.hpp"

#include <ctime>
#include <iomanip>
#include <iostream>

namespace logging {

namespace {
std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}
}

Logger::Logger(std::string name) : name_(std::move(name)) {}

void Logger::set_level(Level level) { level_ = level; }
Level Logger::level() const { return level_; }

void Logger::debug(const std::string& message) { log(Level::Debug, message); }
void Logger::info(const std::string& message) { log(Level::Info, message); }
void Logger::warn(const std::string& message) { log(Level::Warning, message); }
void Logger::error(const std::string& message) { log(Level::Error, message); }

void Logger::log(Level level, const std::string& message) {
    if (level < level_) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << '[' << timestamp() << "] [" << level_to_string(level) << "] "
              << name_ << ": " << message << std::endl;
}

void Logger::format_impl(std::ostringstream& oss, const std::string& fmt) {
    oss << fmt;
}

std::string level_to_string(Level level) {
    switch (level) {
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warning:
            return "WARN";
        case Level::Error:
            return "ERROR";
    }
    return "INFO";
}

}  // namespace logging
