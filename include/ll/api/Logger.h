#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace ll {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

namespace detail {
    inline void logLine(const std::string& line) {
        static std::mutex sMutex;
        std::lock_guard<std::mutex> lock(sMutex);
        const char* p = "/sdcard/Android/media/org.levimc.launcher/minecraft/levilamroid.log";
        FILE* fp = fopen(p, "a");
        if (fp) {
            fputs(line.c_str(), fp);
            fputc('\n', fp);
            fflush(fp);
            fclose(fp);
        }
    }
}

class Logger {
private:
    std::string mTitle;

public:
    Logger(std::string title = "LeviLamroid") : mTitle(std::move(title)) {}

    static Logger& getInstance(std::string title = "LeviLamroid") {
        static Logger instance(std::move(title));
        return instance;
    }

    template <typename... Args>
    void log(LogLevel level, Args&&... args) {
        std::ostringstream ss;
        (ss << ... << std::forward<Args>(args));
        std::string msg = ss.str();

        const char* levelStr = "[INFO]";
        switch (level) {
            case LogLevel::Debug: levelStr = "[DEBUG]"; break;
            case LogLevel::Info:  levelStr = "[INFO]";  break;
            case LogLevel::Warn:  levelStr = "[WARN]";  break;
            case LogLevel::Error: levelStr = "[ERROR]"; break;
        }

        std::string line = std::string(levelStr) + " [" + mTitle + "] " + msg;
        std::cout << line << std::endl;
        detail::logLine(line);
    }

    template <typename... Args>
    void info(Args&&... args) {
        log(LogLevel::Info, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(Args&&... args) {
        log(LogLevel::Warn, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(Args&&... args) {
        log(LogLevel::Error, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(Args&&... args) {
        log(LogLevel::Debug, std::forward<Args>(args)...);
    }
};

} // namespace ll
