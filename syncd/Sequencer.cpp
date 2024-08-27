#include "Sequencer.h"
#include "swss/logger.h"

using namespace syncd;
// Macro for easy logging
#define LogToModuleFile(fileNum, msg) writeToLogFile(__func__, fileNum, msg)

// Define a mutex for thread safety
static std::mutex logMutex;

void writeToLogFile(const std::string& funcName, const std::string& fileNum, const std::string& message) {
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

    // Get the current time in microseconds since the epoch
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(epoch).count();

    // Write the timestamp, function name, and message to the log file
    logFile << microseconds << " " << funcName << ": " << message << std::endl;
    logFile.close();
}

// Helper function to execute all ready responses in order
void Sequencer::executeReadyResponses() {
    //printf("Checking for ready responses in queue...\n");
    //SWSS_LOG_NOTICE("multithreaded: Checking for ready responses in queue...");
    LogToModuleFile("1", "multithreaded: Checking for ready responses in queue...");
    while (true) {
        auto it = responses.find(next_seq_to_send);
        if (it == responses.end()) {
            LogToModuleFile("1", "multithreaded: No next sequence found in queue");
            break;  // Exit loop if the next sequence is not in the map
        }
        it->second();  // Execute the stored lambda
        responses.erase(it);  // Safely erase the entry
        ++next_seq_to_send; // Increment the sequence number
        std::string logMsg = "multithreaded: Next sequence found! Executed lambda with seq: " + std::to_string(next_seq_to_send);
        LogToModuleFile("1", logMsg);

        if (current_seq >= MAX_SEQUENCE_NUMBER) {
            LogToModuleFile("1", "multithreaded: Resetting next sequence number to send needs to be reset to avoid overflow");
            next_seq_to_send = 0;
        }
    }
}

// Get sequence number
int Sequencer::allocateSequenceNumber() {
    std::lock_guard<std::mutex> lock(mtx);
    int seq = current_seq;
    current_seq++;
    //SWSS_LOG_NOTICE("multithreaded: allocate seq num: %d", seq);
    std::string logMsg = "multithreaded: allocate seq num: " + std::to_string(seq);
    LogToModuleFile("1", logMsg);

    if (current_seq >= MAX_SEQUENCE_NUMBER) {
        LogToModuleFile("1", "multithreaded: Resetting allocated sequence number to avoid overflow");
        current_seq = 0;
    }

    return seq;
}

// Add/Execute sequence function
void Sequencer::executeFuncInSequence(int seq, std::function<void()> response_lambda) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (seq == next_seq_to_send) {
        // If the received sequence is the next one to send, execute it immediately
        //SWSS_LOG_NOTICE("multithreaded: executing reseponse lambda, seq num: %d", seq);
        std::string logMsg = "multithreaded: executing reseponse lambda, seq num: " + std::to_string(seq);
        LogToModuleFile("1", logMsg);
        response_lambda();
        // Increment the next sequence to send
        ++next_seq_to_send;

        if (current_seq >= MAX_SEQUENCE_NUMBER) {
            LogToModuleFile("1", "multithreaded: Resetting next sequence number to send needs to be reset to avoid overflow");
            next_seq_to_send = 0;
        }
        // Continue sending any subsequent responses that are ready
        executeReadyResponses();
    } else {
        // If the sequence is not the next to send, store it in the map
        responses[seq] = response_lambda;
        std::string logMsg = "multithreaded: storing lambda with seq: " + std::to_string(seq) + ", next to send: " + std::to_string(next_seq_to_send);
        LogToModuleFile("1", logMsg);
        //SWSS_LOG_NOTICE("Storing lambda with seq: %d, next to send: %d\n", seq, next_seq_to_send);
        //printf("Storing lambda with seq: %d, next to send: %d\n", seq, next_seq_to_send);
    }
}