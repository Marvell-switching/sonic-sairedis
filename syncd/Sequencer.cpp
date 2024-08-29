#include "Sequencer.h"
#include "swss/logger.h"

using namespace syncd;

// Helper function to execute all ready responses in order
// check how many consecutive responses were executed in sucession and log it
Sequencer::SequenceStatus Sequencer::executeReadyResponses() {
    
    std::string logMsg;
    SequenceStatus status = FAILURE;
 
    logMsg = "multithreaded: Checking for ready responses in queue... \n";

    while (true) {
        // Check if the next sequence number is in the map
        auto seq_data = responses.find(next_seq_to_send);
        if (seq_data == responses.end()) {
            logMsg += "multithreaded: No next sequence found in queue \n";
            status = SUCCESS;
            break;  // Exit loop if the next sequence is not in the map
        }

        // Execute the stored lambda
        auto func = seq_data->second;
        if(func) {
            func();  
            logMsg += "multithreaded: Executing lambda with seq: " + std::to_string(next_seq_to_send) + " \n";
            status = NULL_PTR;
        }
        else {
            logMsg += "multithreaded: response lambda is null \n";
            num_of_null_functions++;
            status = SUCCESS; //?????
        }
        
        // Increment the number of executed tasks in sequence
        max_num_of_executed_tasks_in_sequence++; 

        // Safely erase the entry
        responses.erase(seq_data);  
        
        // Increment the sequence number
        ++next_seq_to_send; 

        logMsg += "multithreaded: Next sequence found! Executed lambda with seq: " + std::to_string(next_seq_to_send) + " \n";

        if (next_seq_to_send >= MAX_SEQUENCE_NUMBER) {
            logMsg += "multithreaded: Resetting next sequence number to send needs to be reset to avoid overflow \n";
            next_seq_to_send = 0;
        }
    }

    LogToModuleFile("1", logMsg);
    return status;
}

// Get sequence number
// if sequencer is full, reset the sequence number to avoid overflow
// wait for, throw full failure
// return if there is an issue or success (0-succss, 1-failure)
// add private param to see how many lambdas are stored, 
// for each function, return a status code
// mux on everything
Sequencer::SequenceStatus Sequencer::allocateSequenceNumber(int *seq_num) {
    std::unique_lock<std::mutex> lock(mtx);
    std::string logMsg;
    SequenceStatus status = FAILURE;
    
    if(isFull()) {
        logMsg = "multithreaded: Sequencer is full, cannot allocate sequence number" + std::to_string(current_seq);
        LogToModuleFile("1", logMsg);
        return BUFFER_OVERFLOW;
    }

    // update recieved param
    *seq_num = current_seq;
    // increment the sequence number
    current_seq++;

    logMsg = "multithreaded: allocate seq num: " + std::to_string(*seq_num) + ", ";

    // reset number to avoid overflow
    if (current_seq >= MAX_SEQUENCE_NUMBER) {
        logMsg += "multithreaded: Resetting allocated sequence number to avoid overflow, ";
        current_seq = 0;
    }

    status = SUCCESS;
    
    LogToModuleFile("1", logMsg);

    return status;
}

// Add/Execute sequence function
Sequencer::SequenceStatus Sequencer::executeFuncInSequence(int seq, std::function<void()> response_lambda) {
   
   std::unique_lock<std::mutex> lock(mtx);
   std::string logMsg;
   SequenceStatus status = FAILURE;
    
    if (seq == next_seq_to_send) {
        // If the received sequence is the next one to send, execute it immediately
        logMsg = "multithreaded: executing reseponse lambda, seq num: " + std::to_string(seq) + " \n";
        
        // execute response lambda
        if(response_lambda) {
            response_lambda();
            logMsg += "multithreaded: execute response lambda \n";
            status = SUCCESS;
        }
        else {
            logMsg += "multithreaded: response lambda is null \n";
            num_of_null_functions++;
            status = SUCCESS; //NULL_PTR; ???
        }

        // increment the number of executed tasks in sequence
        max_num_of_executed_tasks_in_sequence++;

        // Increment the next sequence to send
        ++next_seq_to_send;
        
        // reset number to avoid overflow
        if (next_seq_to_send >= MAX_SEQUENCE_NUMBER) {
            logMsg += "multithreaded: Resetting next sequence number to send needs to be reset to avoid overflow \n";
            next_seq_to_send = 0;
        }

        // Continue sending any subsequent responses that are ready
        status = executeReadyResponses();
    } else {
        // If the sequence is not the next to send, store it in the map
        responses[seq] = response_lambda;
        logMsg = "multithreaded: storing lambda with seq: " + std::to_string(seq) + ", next to send: " + std::to_string(next_seq_to_send) + " \n";
        status = SUCCESS;
        num_of_out_of_sequence_functions++;
    }
    
    LogToModuleFile("1", logMsg);
    return status;
}

Sequencer::SequenceStatus Sequencer::showStatistics() {
    std::unique_lock<std::mutex> lock(mtx);
    std::string logMsg = "STATISTICS: \n";
    logMsg = "multithreaded: max number of executed tasks in sequence: " + std::to_string(max_num_of_executed_tasks_in_sequence) + " \n";
    logMsg += "multithreaded: number of null functions: " + std::to_string(num_of_null_functions) + " \n";
    logMsg += "multithreaded: number of out of sequence functions: " + std::to_string(num_of_out_of_sequence_functions) + " \n";
    logMsg += std::to_string(current_seq) + " out of " + std::to_string(max_seq_num) + "used";
    LogToModuleFile("1", logMsg);
    return SUCCESS;
}

Sequencer::SequenceStatus Sequencer::clearStatistics() {
    std::unique_lock<std::mutex> lock(mtx);
    max_num_of_executed_tasks_in_sequence = 0;
    num_of_null_functions = 0;
    num_of_out_of_sequence_functions = 0;
    LogToModuleFile("1", "CLEANED STATISTICS \n");
    return SUCCESS;
}

bool Sequencer::isFull() {
    if(responses.size() < max_seq_num) {
        LogToModuleFile("1", "is not full");
        return false;
    }
    else {
        LogToModuleFile("1", "is full");
        return true;
    }
}