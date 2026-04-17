#include "logging.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace {
std::mutex g_logMutex;
std::ofstream g_logFile;
std::string g_logFilePath;

std::string timestampNow() {
    using clock = std::chrono::system_clock;
    std::time_t now = clock::to_time_t(clock::now());
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &now);
#else
    localtime_r(&now, &localTm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void writeLine(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (!g_logFile.is_open()) {
        return;
    }

    g_logFile << "[" << timestampNow() << "] [" << level << "] " << message << "\n";
    g_logFile.flush();
}
}  // namespace

namespace logging {

void init(const std::string& appName, const std::string& version) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    std::filesystem::path logDir = std::filesystem::current_path() / "logs";
    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);

    std::filesystem::path logPath = logDir / (appName + ".log");
    g_logFilePath = logPath.string();
    g_logFile.open(logPath, std::ios::out | std::ios::app);

    if (!g_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logPath << "\n";
        return;
    }

    g_logFile << "\n==== " << appName << " " << version << " session started at "
              << timestampNow() << " ====\n";
    g_logFile.flush();
}

void info(const std::string& message) {
    writeLine("INFO", message);
}

void error(const std::string& message) {
    writeLine("ERROR", message);
}

std::string getLogFilePath() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    return g_logFilePath;
}

}  // namespace logging
