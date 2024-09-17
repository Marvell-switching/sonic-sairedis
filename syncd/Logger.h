#ifndef LOGGER_H
#define LOGGER_H

#define MODULE_NAME     "syncd"

#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <iomanip>
#include "fmt/format.h"
#include <ctime>
#include <thread>

#define SYNCD_LOGGER_VER " V9 "
#define MAX_LOG_SIZE    (10 *50 * 1024) /* 50 KB */
#define ENABLE_LOGGING_LOW_P   1
#define ENABLE_LOGGING_MED_P   1
#define ENABLE_LOGGING_HIGH_P  1

// Define a mutex for thread safety
static std::mutex logMutex;

// Improved logging function with thread safety
static void writeToLogFile(const std::string& funcName, const std::string& fileNum, const std::string& message) {
    // Lock the mutex to ensure thread safety
    std::lock_guard<std::mutex> lock(logMutex);

    std::string filePath = "/" + std::string(MODULE_NAME) + "_debugLog_" + fileNum + ".txt";
    std::string backupFilePath = filePath + ".history";
    struct stat fileStat;

    // Check if the log file exists and if its size exceeds the maximum limit
    if (stat(filePath.c_str(), &fileStat) == 0) {
        if (fileStat.st_size > MAX_LOG_SIZE) {
            // Remove the old backup file
            std::remove(backupFilePath.c_str());
            // Rename the current log file to the backup file
            if (std::rename(filePath.c_str(), backupFilePath.c_str()) != 0) {
                std::cerr << "Error: Could not rename file " << filePath << " to " << backupFilePath << std::endl;
                return;
            }
        }
    }

    // Open the log file in append mode
    std::ofstream logFile(filePath, std::ios_base::app);
    if (!logFile.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << milliseconds.count();
    std::string formatted_time = oss.str();

    std::thread::id this_id = std::this_thread::get_id();

    // Write the timestamp, function name, and message to the log file
    logFile << formatted_time << SYNCD_LOGGER_VER << this_id << " " << funcName << ": " << message << std::endl;

    
    logFile.close();
}

template<typename... Args>
static void logFormattedMessage(const std::string& funcName, const std::string& fileNum, const std::string& format, Args... messageArgs) {
    std::ostringstream oss;
   
    std::string remainingFormat = format;
    
    // Helper function to process a single argument
    auto processArg = [&oss, &remainingFormat](const auto& arg) {
        size_t pos = remainingFormat.find("{}");
        if (pos != std::string::npos) {
            oss << remainingFormat.substr(0, pos) << arg;
            remainingFormat = remainingFormat.substr(pos + 2);
        }
    };
    
    // Helper function to recursively process all arguments
    auto processAllArgs = [&](auto&&... innerArgs) -> void {
        (void)std::initializer_list<int>{(processArg(innerArgs), 0)...};
    };
    
    // Process all arguments
    processAllArgs(messageArgs...);
    
    // Add any remaining format string
    oss << remainingFormat;

    // Write the full message to the log file
    writeToLogFile(funcName, fileNum, oss.str());
}
#if ENABLE_LOGGING_LOW_P
// Macro for easy logging with formatting
#define LogToModuleFile(fileNum, format, ...) logFormattedMessage(__func__, fileNum, format, ##__VA_ARGS__)
#else
#define LogToModuleFile(fileNum, format, ...)
#endif

#if ENABLE_LOGGING_MED_P
// Macro for easy logging with formatting
#define LogToModuleFileMp(fileNum, format, ...) logFormattedMessage(__func__, fileNum, format, ##__VA_ARGS__)
#else
#define LogToModuleFileMp(fileNum, format, ...)
#endif

#if ENABLE_LOGGING_HIGH_P
// Macro for easy logging with formatting
#define LogToModuleFileHp(fileNum, format, ...) logFormattedMessage(__func__, fileNum, format, ##__VA_ARGS__)
#else
#define LogToModuleFileHp(fileNum, format, ...)
#endif


#endif // LOGGER_H
