#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <map>
#include <mutex>
#include <iostream>
#include <functional>
#include <chrono>
#include <limits>

#include <condition_variable>


#include <string>
#include <fstream>
#include <sys/stat.h>
#include <cstdio>
#include "Logger.h"

namespace sequencer {

    #define MAX_SEQUENCE_NUMBER 1024
    #define INVALID_SEQUENCE_NUMBER std::numeric_limits<int>::min() // allow user to choose
    using AnyTask = std::function<void()>;
    // Public types
    typedef int seq_t;

    enum SequenceStatus {
        FAILURE = -1,
        SUCCESS = 0,
        BUFFER_FULL = 1,
        NULL_PTR = 2,
        SEQUENCER_EXIT_SUCCESS  = 3
    };

    class Sequencer {
    public:
        // Public constructor
        Sequencer() : 
        sequencer_exited(false),
        current_seq(0), 
        next_seq_to_send(0), 
        max_seq_num(MAX_SEQUENCE_NUMBER), 
        max_num_of_executed_tasks_in_sequence(0),
        total_num_of_executed_tasks_in_sequence(0),
        current_num_of_executed_tasks_in_sequence(0),
        num_of_null_functions(0),  
        num_of_out_of_sequence_functions(0){}

        // Public destructor
        ~Sequencer();
        
        // Get sequence number
        SequenceStatus allocateSequenceNumber(seq_t *seq_num);

        // Add/Execute sequence function
        SequenceStatus executeFuncInSequence(seq_t seq, std::function<void()> response_lambda = nullptr, std::shared_ptr<std::mutex> mutex = nullptr);

        SequenceStatus waitSequenceNumber(seq_t seq_num);

        SequenceStatus showStatistics();

        SequenceStatus clearStatistics();

        bool isExit();

        bool sequencer_exited;  // Indicates if the sequencer has exited
        std::mutex mtx;  // Protects shared data
        bool amINext(int seq_num);
        bool isFull();


    private:
        // Watchdog function to monitor inactivity
        SequenceStatus performWatchdogCheck();

        // Helper function to execute all ready responses in order
        SequenceStatus executeNextSequences();

        bool isMyTurn(seq_t seq_num);

        int seqGetNext(seq_t seq_num);

        void callSeq(std::function<void()> response_lambda, std::shared_ptr<std::mutex> response_mutex, bool unlock = true);

        void flush();

        typedef struct seq_data_t_ {
            std::function<void()> response_lambda;
            std::shared_ptr<std::mutex> response_mutex;
        } seq_data_t;

        int current_seq;  // Tracks the latest sequence number assigned to a task
        seq_t next_seq_to_send;  // The next sequence number that should be sent
        long unsigned int max_seq_num;  // The maximum sequence number
        int max_num_of_executed_tasks_in_sequence; // The maximum number of executed tasks in sequence
        int total_num_of_executed_tasks_in_sequence; // The total number of executed tasks in sequence
        int current_num_of_executed_tasks_in_sequence; // The current number of executed tasks in sequence
        int num_of_null_functions; // The number of null functions
        int num_of_out_of_sequence_functions; // The number of out of sequence functions
        std::map<int, seq_data_t> responses;  // Stores responses by sequence number
        std::condition_variable cv;
    };
    
}


#endif // SEQUENCER_H