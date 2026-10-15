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

class Logger {
private:
    std::string mTitle;
    std::mutex mMutex;
    std::ofstream mLogFile;

public:
    Logger(std::string title = "LeviLamroid") : mTitle(std::move(title)) {
        // Open log file in Minecraft folder
        const char* paths[] = {
            "/sdcard/Android/media/org.levimc.launcher/minecraft/levilamroid.log",
            "/sdcard/Android/media/org.levimc.launcher/minecraft/com.mojang.minecraftpe/levilamroid.log",
            "/sdcard/levilamroid.log",
            "/sdcard/Android/media/org.levimc.launcher/minecraft/levilamroid.log"
        };
        for (const char* p : paths) {
            mLogFile.open(p, std::ios::app);
            if (mLogFile.is_open()) break;
        }
    }

    static Logger& getInstance(std::string title = "LeviLamroid") {
        static Logger instance(std::move(title));
        return instance;
    }

    template <typename... Args>
    void log(LogLevel level, Args&&... args) {
        std::lock_guard<std::mutex> lock(mMutex);
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
        if (mLogFile.is_open()) {
            mLogFile << line << std::endl;
            mLogFile.flush();
        }
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
