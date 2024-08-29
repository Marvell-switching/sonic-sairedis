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

    #define MAX_SEQUENCE_NUMBER 1024
    #define INVALID_SEQUENCE_NUMBER std::numeric_limits<int>::min() // allow user to choose
    using AnyTask = std::function<void()>;

    class Sequencer {
    public:
        // Public constructor
        Sequencer() : current_seq(0), next_seq_to_send(0), max_seq_num(MAX_SEQUENCE_NUMBER), max_num_of_executed_tasks_in_sequence(0), num_of_null_functions(0), num_of_out_of_sequence_functions(0) {}

        // Public destructor
        ~Sequencer() {}

        enum SequenceStatus {
            FAILURE = -1,
            SUCCESS = 0,
            BUFFER_OVERFLOW = 1,
            NULL_PTR = 2,
        };
        
        // Get sequence number
        SequenceStatus allocateSequenceNumber(int *seq_num);

        // Add/Execute sequence function
        SequenceStatus executeFuncInSequence(int seq, std::function<void()> response_lambda);

    private:
        // Watchdog function to monitor inactivity
        SequenceStatus performWatchdogCheck();

        // Helper function to execute all ready responses in order
        SequenceStatus executeReadyResponses();

        SequenceStatus showStatistics();

        SequenceStatus clearStatistics();

        bool isFull();

        int current_seq;  // Tracks the latest sequence number assigned to a task
        int next_seq_to_send;  // The next sequence number that should be sent
        long unsigned int max_seq_num;  // The maximum sequence number
        int max_num_of_executed_tasks_in_sequence; // The maximum number of executed tasks in sequence
        int num_of_null_functions; // The number of null functions
        int num_of_out_of_sequence_functions; // The number of out of sequence functions
        std::mutex mtx;  // Protects shared data
        std::map<int, std::function<void()>> responses;  // Stores responses by sequence number
    };
    
}


#endif // SEQUENCER_H