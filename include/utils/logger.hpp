#pragma once

#include <chrono>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>

namespace logging {

enum class Level { Debug, Info, Warning, Error };

class Logger {
public:
    explicit Logger(std::string name);

    void set_level(Level level);
    Level level() const;

    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    template <typename... Args>
    void infof(const std::string& fmt, Args&&... args) {
        info(format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void warnf(const std::string& fmt, Args&&... args) {
        warn(format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void errorf(const std::string& fmt, Args&&... args) {
        error(format(fmt, std::forward<Args>(args)...));
    }

    template <typename... Args>
    void debugf(const std::string& fmt, Args&&... args) {
        debug(format(fmt, std::forward<Args>(args)...));
    }

private:
    template <typename... Args>
    std::string format(const std::string& fmt, Args&&... args) {
        std::ostringstream oss;
        format_impl(oss, fmt, std::forward<Args>(args)...);
        return oss.str();
    }

    void log(Level level, const std::string& message);

    void format_impl(std::ostringstream& oss, const std::string& fmt);

    template <typename T, typename... Args>
    void format_impl(std::ostringstream& oss, const std::string& fmt, T&& value, Args&&... rest) {
        std::size_t pos = fmt.find("{}");
        if (pos == std::string::npos) {
            oss << fmt;
            return;
        }
        oss << fmt.substr(0, pos);
        oss << std::forward<T>(value);
        format_impl(oss, fmt.substr(pos + 2), std::forward<Args>(rest)...);
    }

    std::string name_;
    Level level_ = Level::Info;
    std::mutex mutex_;
};

std::string level_to_string(Level level);

}  // namespace logging
