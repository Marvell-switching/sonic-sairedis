#include "Sequencer.h"
#include "swss/logger.h"
#include "Logger.h"

using namespace syncd;

// Helper function to execute all ready responses in order
void Sequencer::executeReadyResponses() {
    LogToModuleFile("1", "multithreaded: Checking for ready responses in queue...");
    while (true) {
        // Check if the next sequence number is in the map
        auto it = responses.find(next_seq_to_send);
        if (it == responses.end()) {
            LogToModuleFile("1", "multithreaded: No next sequence found in queue");
            break;  // Exit loop if the next sequence is not in the map
        }

        it->second();  // Execute the stored lambda
        responses.erase(it);  // Safely erase the entry
        
        {
            std::unique_lock<std::mutex> lock(mtx);
            ++next_seq_to_send; // Increment the sequence number
        }

        std::string logMsg = "multithreaded: Next sequence found! Executed lambda with seq: " + std::to_string(next_seq_to_send);
        LogToModuleFile("1", logMsg);

        if (current_seq >= MAX_SEQUENCE_NUMBER) {
            LogToModuleFile("1", "multithreaded: Resetting next sequence number to send needs to be reset to avoid overflow");
            std::unique_lock<std::mutex> lock(mtx);
            next_seq_to_send = 0;
        }
    }
}

// Get sequence number
int Sequencer::allocateSequenceNumber() {
    
    int seq = current_seq;
    
    {
        std::unique_lock<std::mutex> lock(mtx);
        current_seq++;
    }

    std::string logMsg = "multithreaded: allocate seq num: " + std::to_string(seq);
    LogToModuleFile("1", logMsg);

    if (current_seq >= MAX_SEQUENCE_NUMBER) {
        LogToModuleFile("1", "multithreaded: Resetting allocated sequence number to avoid overflow");
        std::unique_lock<std::mutex> lock(mtx);
        current_seq = 0;
    }

    return seq;
}

// Add/Execute sequence function
void Sequencer::executeFuncInSequence(int seq, std::function<void()> response_lambda) {
   
    if (seq == next_seq_to_send) {
        // If the received sequence is the next one to send, execute it immediately
        //SWSS_LOG_NOTICE("multithreaded: executing reseponse lambda, seq num: %d", seq);
        std::string logMsg = "multithreaded: executing reseponse lambda, seq num: " + std::to_string(seq);
        LogToModuleFile("1", logMsg);
        response_lambda();

        {
            std::unique_lock<std::mutex> lock(mtx);
            // Increment the next sequence to send
            ++next_seq_to_send;
        }
        
        if (current_seq >= MAX_SEQUENCE_NUMBER) {
            LogToModuleFile("1", "multithreaded: Resetting next sequence number to send needs to be reset to avoid overflow");
            std::unique_lock<std::mutex> lock(mtx);
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