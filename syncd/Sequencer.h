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
#include <vector>
#include "Logger.h"

namespace sequencer {
    
    #define MAX_SEQUENCE_NUMBER 1024
    #define INVALID_SEQUENCE_NUMBER std::numeric_limits<int>::min() // allow user to choose
    using AnyTask = std::function<void()>;

    class Sequencer {
    public:
        // Public constructor
        Sequencer() : 
        buffer(MAX_SEQUENCE_NUMBER),
        current_seq(0), 
        next_seq_to_send(0), 
        max_seq_num(MAX_SEQUENCE_NUMBER), 
        max_num_of_executed_tasks_in_sequence(0),
        total_num_of_executed_tasks_in_sequence(0),
        current_num_of_executed_tasks_in_sequence(0),
        num_of_null_functions(0), 
        num_of_out_of_sequence_functions(0), 
        sequencer_exited(false) {}

        // Public destructor
        ~Sequencer() {sequencer_exited=true;}

        enum SequenceStatus {
            FAILURE = -1,
            SUCCESS = 0,
            BUFFER_FULL = 1,
            NULL_PTR = 2,
        };
        
        // Get sequence number
        bool allocateSequenceNumber(int *seq_num);

        // Add/Execute sequence function
        bool executeFuncInSequence(int seq, std::function<void()> response_lambda);

        SequenceStatus showStatistics();

        SequenceStatus clearStatistics();

    private:
        // Watchdog function to monitor inactivity
        SequenceStatus performWatchdogCheck();

        // Helper function to execute all ready responses in order
        SequenceStatus executeReadyResponses();

        bool IsFull();
        bool IsEmpty();
        bool push(std::function<void()> ringEntry);
        std::function<void()>& HeadEntry();
        bool pop(std::function<void()>& ringEntry);

        std::vector<std::function<void()>> buffer;
        int current_seq;  // Tracks the latest sequence number assigned to a task
        int next_seq_to_send;  // The next sequence number that should be sent
        int max_seq_num;  // The maximum sequence number, ring buffer size
        int max_num_of_executed_tasks_in_sequence; // The maximum number of executed tasks in sequence
        int total_num_of_executed_tasks_in_sequence; // The total number of executed tasks in sequence
        int current_num_of_executed_tasks_in_sequence; // The current number of executed tasks in sequence
        int num_of_null_functions; // The number of null functions
        int num_of_out_of_sequence_functions; // The number of out of sequence functions
        std::mutex mtx;  // Protects shared data
        bool sequencer_exited;  // Indicates if the sequencer has exited
    };
}


#endif // SEQUENCER_H