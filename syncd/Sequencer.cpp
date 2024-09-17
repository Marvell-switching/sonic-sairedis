#include "Sequencer.h"
#include "swss/logger.h"

using namespace sequencer;

// Helper function to execute all ready responses in order
// check how many consecutive responses were executed in sucession and log it
Sequencer::SequenceStatus Sequencer::executeReadyResponses() {
    
    SequenceStatus status = FAILURE;
 
    LogToModuleFile("1", "Checking for ready responses in queue...");

    while (!sequencer_exited) {
        // Check if the next sequence number is in the map
        auto seq_data = responses.find(next_seq_to_send);
        if (seq_data == responses.end()) {
            LogToModuleFile("1", "No next sequence found in queue");
            status = SUCCESS;
            break;  // Exit loop if the next sequence is not in the map
        }

        // Execute the stored lambda
        auto func = seq_data->second;
        if(func) {
            LogToModuleFile("1", "before execute lambda with sequenceNumber: {}", next_seq_to_send);
            //SWSS_LOG_INFO("Before execute lambda with sequenceNumber: %d", next_seq_to_send);
            func();            
            LogToModuleFileMp("1", "after execute lambda with sequenceNumber: {}", next_seq_to_send);
            status = SUCCESS;
        }
        else {
            LogToModuleFile("1", "multithreaded: response lambda is null {}", next_seq_to_send);
            num_of_null_functions++;
            status = NULL_PTR;
        }
        
        // Increment the number of executed tasks in sequence
        total_num_of_executed_tasks_in_sequence++; 
        current_num_of_executed_tasks_in_sequence++;

        // Safely erase the entry
        responses.erase(seq_data);  
        
        // Increment the sequence number
        ++next_seq_to_send; 

        LogToModuleFile("1", "Next sequence found! Executed lambda with seq: {}", next_seq_to_send);

        if (next_seq_to_send >= MAX_SEQUENCE_NUMBER) {
            LogToModuleFile("1", "Resetting next sequence number to send needs to be reset to avoid overflow");
            next_seq_to_send = 0;
        }
    }

    return status;
}

// Get sequence number
// if sequencer is full, wait
bool Sequencer::allocateSequenceNumber(int *seq_num) {
    std::unique_lock<std::mutex> lock(mtx);
   
    while(isFull()) {
        LogToModuleFileHp("1", "Sequencer is full, cannot allocate sequence number {}", current_seq);
        //SWSS_LOG_INFO("Sequencer is full, cannot allocate sequence number %d", current_seq);
        //TODO: add sleep, and timeout error after X seconds
    }

    // update recieved param
    *seq_num = current_seq;
    // increment the sequence number
    current_seq++;
   
    LogToModuleFile("1", "allocate seq num {}", *seq_num);

    // reset number to avoid overflow
    if (current_seq >= MAX_SEQUENCE_NUMBER) {
        LogToModuleFile("1", "Resetting allocated sequence number to avoid overflow");
        current_seq = 0;
    }

    return true;
}

// Add/Execute sequence function
bool Sequencer::executeFuncInSequence(int seq, std::function<void()> response_lambda) {
   
   std::unique_lock<std::mutex> lock(mtx);
   // internal status
   SequenceStatus status = FAILURE;
    
   LogToModuleFile("1", "Enter executeFuncInSequence with seq: {}", seq);
   current_num_of_executed_tasks_in_sequence = 0;

    if (seq == next_seq_to_send) {
        // If the received sequence is the next one to send, execute it immediately
        
        // execute response lambda
        if(response_lambda) {
            LogToModuleFile("1", "start execute response_lambda with sequenceNumber: {}", next_seq_to_send);
            response_lambda();
            LogToModuleFileMp("1", "end execute response_lambda with sequenceNumber: {}", next_seq_to_send);
            status = SUCCESS;
        }
        else {
            LogToModuleFile("1", "response lambda is null ");
            num_of_null_functions++;
            status = SUCCESS; //NULL_PTR; ???
        }

        // increment the number of executed tasks in sequence
        total_num_of_executed_tasks_in_sequence++;
        current_num_of_executed_tasks_in_sequence++;

        // Increment the next sequence to send
        ++next_seq_to_send;
        
        // reset number to avoid overflow
        if (next_seq_to_send >= MAX_SEQUENCE_NUMBER) {
            LogToModuleFile("1", "Resetting next sequence number to send needs to be reset to avoid overflow");
            next_seq_to_send = 0;
        }

        // Continue sending any subsequent responses that are ready
        LogToModuleFile("1", "start execute executeReadyResponses");
        executeReadyResponses();
        LogToModuleFile("1", "end execute executeReadyResponses");
    } else {
        // If the sequence is not the next to send, store it in the map
        responses[seq] = response_lambda;       
        LogToModuleFileHp("1", "storing lambda with seq: {} next to send: {}", seq, next_seq_to_send);
        //SWSS_LOG_INFO("storing lambda with seq: %d next to send: %d", seq, next_seq_to_send);
        status = SUCCESS;
        num_of_out_of_sequence_functions++;
    }
    
    if(current_num_of_executed_tasks_in_sequence > max_num_of_executed_tasks_in_sequence) {
        max_num_of_executed_tasks_in_sequence = current_num_of_executed_tasks_in_sequence;
    }

    if(status == SUCCESS)
        return true;
    else
        return false;
}

Sequencer::SequenceStatus Sequencer::showStatistics() {
    std::unique_lock<std::mutex> lock(mtx);
    std::string logMsg = "STATISTICS: \n";
    logMsg = "multithreaded: total number of executed tasks in sequence: " + std::to_string(total_num_of_executed_tasks_in_sequence) + " \n";
    logMsg += "multithreaded: number of null functions: " + std::to_string(num_of_null_functions) + " \n";
    logMsg += "multithreaded: number of out of sequence functions: " + std::to_string(num_of_out_of_sequence_functions) + " \n";
    logMsg += std::to_string(current_seq) + " out of " + std::to_string(max_seq_num) + "used";
    LogToModuleFile("2", logMsg);
    return SUCCESS;
}

Sequencer::SequenceStatus Sequencer::clearStatistics() {
    std::unique_lock<std::mutex> lock(mtx);
    total_num_of_executed_tasks_in_sequence = 0;
    num_of_null_functions = 0;
    num_of_out_of_sequence_functions = 0;
    LogToModuleFile("2", "CLEANED STATISTICS \n");
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