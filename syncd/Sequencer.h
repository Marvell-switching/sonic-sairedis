#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <map>
#include <mutex>
#include <iostream>
#include <functional>
#include <chrono>
#include <limits>


#include <string>
#include <fstream>
#include <sys/stat.h>
#include <cstdio>
#include "Logger.h"


namespace syncd {

    #define MAX_SEQUENCE_NUMBER 1000000
    #define INVALID_SEQUENCE_NUMBER std::numeric_limits<int>::min()
    using AnyTask = std::function<void()>;

    class Sequencer {
    public:
        // Public constructor
        Sequencer() : current_seq(0), next_seq_to_send(0), last_update_time(std::chrono::steady_clock::now()) {}

        // Public destructor
        ~Sequencer() {}

        // Get sequence number
        int allocateSequenceNumber();

        // Add/Execute sequence function
        void executeFuncInSequence(int seq, std::function<void()> response_lambda);

    private:
        // Reset the sequence number to avoid overflow
        void resetSequence();

        // Watchdog function to monitor inactivity
        void performWatchdogCheck();

        // Helper function to execute all ready responses in order
        void executeReadyResponses();

        int current_seq;  // Tracks the latest sequence number assigned to a task
        int next_seq_to_send;  // The next sequence number that should be sent
        std::mutex mtx;  // Protects shared data
        std::map<int, std::function<void()>> responses;  // Stores responses by sequence number
        std::chrono::steady_clock::time_point last_update_time;  // Time of the last sequence update
    };
    
}


#endif // SEQUENCER_H