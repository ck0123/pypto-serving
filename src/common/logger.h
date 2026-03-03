#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace pypto {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void set_level(LogLevel level) { level_ = level; }
    LogLevel level() const { return level_; }

    void log(LogLevel level, const std::string& msg) {
        if (level < level_) return;
        
        std::ostream& out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
        out << "[" << level_to_string(level) << "] " << msg << std::endl;
    }

private:
    Logger() : level_(LogLevel::INFO) {}
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel level_;

    static const char* level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

// Convenience macros
#define LOG_DEBUG(msg) do { \
    std::ostringstream oss; \
    oss << msg; \
    pypto::Logger::instance().log(pypto::LogLevel::DEBUG, oss.str()); \
} while(0)

#define LOG_INFO(msg) do { \
    std::ostringstream oss; \
    oss << msg; \
    pypto::Logger::instance().log(pypto::LogLevel::INFO, oss.str()); \
} while(0)

#define LOG_WARNING(msg) do { \
    std::ostringstream oss; \
    oss << msg; \
    pypto::Logger::instance().log(pypto::LogLevel::WARNING, oss.str()); \
} while(0)

#define LOG_ERROR(msg) do { \
    std::ostringstream oss; \
    oss << msg; \
    pypto::Logger::instance().log(pypto::LogLevel::ERROR, oss.str()); \
} while(0)

} // namespace pypto
