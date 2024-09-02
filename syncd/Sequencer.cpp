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
        {
            AnyTask func;
            if(pop(func)) {
                func();
                
                
            }
            else {
                break;
            }
        }
        
        {
            std::unique_lock<std::mutex> lock(mtx);
            // Increment the number of executed tasks in sequence
            total_num_of_executed_tasks_in_sequence++; 
            current_num_of_executed_tasks_in_sequence++;
        }

        LogToModuleFile("1", "Next sequence found! Executed lambda with seq: {}", next_seq_to_send);
    }

    return status;
}

// Get sequence number
// if sequencer is full, wait
bool Sequencer::allocateSequenceNumber(int *seq_num) {
    while(IsFull()) {
        LogToModuleFile("1", "Sequencer is full, cannot allocate sequence number {}", current_seq);
        //TODO: add sleep, and timeout error after X seconds
    }

    LogToModuleFile("1", "before lock");
    std::unique_lock<std::mutex> lock(mtx);
    LogToModuleFile("1", "after lock");
    // update recieved param
    *seq_num = current_seq;
    // increment the sequence number
    current_seq = (current_seq + 1) % max_seq_num;

    lock.unlock();
    LogToModuleFile("1", "allocate seq num {}", *seq_num);

    return true;
}

// Add/Execute sequence function
bool Sequencer::executeFuncInSequence(int seq, std::function<void()> response_lambda) {
   
   SequenceStatus status;
   {
        std::unique_lock<std::mutex> lock(mtx);
        current_num_of_executed_tasks_in_sequence = 0; 
   }
   
   LogToModuleFile("1", "Enter executeFuncInSequence with seq: {}", seq);
   
    if (seq == next_seq_to_send) {
        // If the received sequence is the next one to send, execute it immediately
        
        // execute response lambda
        if(response_lambda) {
            LogToModuleFile("1", "start execute response_lambda with sequenceNumber: {}", next_seq_to_send);
            response_lambda();
            LogToModuleFile("1", "end execute response_lambda with sequenceNumber: {}", next_seq_to_send);
            status = SUCCESS;
        }
        else {
            LogToModuleFile("1", "response lambda is null ");
            num_of_null_functions++;
            status = SUCCESS; //NULL_PTR; ???
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            // increment the number of executed tasks in sequence
            total_num_of_executed_tasks_in_sequence++;
            current_num_of_executed_tasks_in_sequence++;

            // Increment the next sequence to send
            next_seq_to_send = (next_seq_to_send + 1) % max_seq_num;
        }
        
        // Continue sending any subsequent responses that are ready
        LogToModuleFile("1", "start execute executeReadyResponses");
        executeReadyResponses();
        LogToModuleFile("1", "end execute executeReadyResponses");
    } else {
        // If the sequence is not the next to send, store it in the map
        push(response_lambda);
        LogToModuleFile("1", "storing lambda with seq: {} next to send: {}", seq, next_seq_to_send);
        status = SUCCESS;

        {
            std::unique_lock<std::mutex> lock(mtx);
            num_of_out_of_sequence_functions++;
        }
        
    }
    
    {
        std::unique_lock<std::mutex> lock(mtx);
        if(current_num_of_executed_tasks_in_sequence > max_num_of_executed_tasks_in_sequence) {
            max_num_of_executed_tasks_in_sequence = current_num_of_executed_tasks_in_sequence;
        }
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

bool Sequencer::IsFull()
{
    std::unique_lock<std::mutex> lock(mtx);
    return (current_seq + 1) % max_seq_num == next_seq_to_send;
}
bool Sequencer::IsEmpty()
{
    std::unique_lock<std::mutex> lock(mtx);
    return current_seq == next_seq_to_send;
}

bool Sequencer::push(std::function<void()> ringEntry)
{
    if (IsFull())
        return false;

    std::unique_lock<std::mutex> lock(mtx);
    buffer[current_seq] = std::move(ringEntry);
    return true;
}

std::function<void()>& Sequencer::HeadEntry() {
    return buffer[next_seq_to_send];
}
bool Sequencer::pop(std::function<void()>& ringEntry)
{
    if (IsEmpty())
        return false;

    std::unique_lock<std::mutex> lock(mtx);
    ringEntry = std::move(buffer[next_seq_to_send]);

    if(ringEntry) {
        next_seq_to_send = (next_seq_to_send + 1) % max_seq_num;
        return true;
    }
    else {
        LogToModuleFile("1", "multithreaded: response lambda is null {}", next_seq_to_send);
        num_of_null_functions++;
        return false;
    }
}