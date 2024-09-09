#include "Sequencer.h"
#include "swss/logger.h"

using namespace sequencer;

// Helper function to execute all ready responses in order
// check how many consecutive responses were executed in succession and log it
// locked by calling function
sequencer::SequenceStatus Sequencer::executeNextSequences() {    
    SequenceStatus status = FAILURE; 
    while (!sequencer_exited) {
        // Check if the next sequence number is in the map
        auto seq_data = responses.find(next_seq_to_send);
        if (seq_data == responses.end()) {
            LogToModuleFile("1", "No next sequence found in queue");
            status = sequencer::SUCCESS;
            break;  // Exit loop if the next sequence is not in the map
        }
        LogToModuleFile("1", "Next sequence found in queue seq {}", next_seq_to_send);

        seq_data_t entry  = seq_data->second;
        // Execute the stored lambda
        callSeq(entry.response_lambda, entry.response_mutex);
        // Increment the number of executed tasks in sequence
        total_num_of_executed_tasks_in_sequence++; 
        current_num_of_executed_tasks_in_sequence++;
        // Safely erase the entry
        responses.erase(seq_data);          
        // Increment the sequence number
        next_seq_to_send=seqGetNext(next_seq_to_send); 
        // notify all waiting threads - on full or on wait specific sequence
        cv.notify_all();
    }
    return status;
}

bool Sequencer::isExit()
{
    return sequencer_exited;
}

// Get sequence number
// if sequencer is full, wait
sequencer::SequenceStatus Sequencer::allocateSequenceNumber(int *seq_num) {
    std::unique_lock<std::mutex> lock(mtx);
    // if queue is full wait on condition variable
    cv.wait(lock, [this] { return ( isExit() || isFull()==false) ; });
    // exit sequencer_exited
    if (sequencer_exited) {
        *seq_num = INVALID_SEQUENCE_NUMBER;
        return SEQUENCER_EXIT_SUCCESS;
    }
    // update receive param
    *seq_num = current_seq;
    // increment the sequence number
    current_seq = seqGetNext(current_seq);
    // return success  
    return sequencer::SUCCESS;
}

Sequencer::~Sequencer(){ 
    flush(); 
    sequencer_exited=true;
}

void Sequencer::callSeq(std::function<void()> response_lambda, std::shared_ptr<std::mutex> response_mutex, bool unlock) {
    // response lambda is not null
    if(response_lambda) {
        // unlock the mutex
        if ( unlock )
            mtx.unlock();
        // response mutex is not null
        if ( response_mutex )
            response_mutex->lock();
        response_lambda();
        // response mutex is not null
        if ( response_mutex )
            response_mutex->unlock();
        // lock the mutex           
        if ( unlock )
            mtx.lock();
    }
    else
        // update stats
        num_of_null_functions++;
        
}

// Add/Execute sequence function
sequencer::SequenceStatus Sequencer::executeFuncInSequence(seq_t seq, std::function<void()> response_lambda, std::shared_ptr<std::mutex> response_mutex) {
    // invalid seq so execute immediately
    if ( seq == INVALID_SEQUENCE_NUMBER) {
        callSeq(response_lambda, response_mutex, false);
        return sequencer::SUCCESS;
    }
    // internal status
    SequenceStatus status = FAILURE;
    current_num_of_executed_tasks_in_sequence = 0;
    // lock the mutex
    mtx.lock();
    if (seq == next_seq_to_send) {
        // If the received sequence is the next one to send, execute it immediately
        callSeq(response_lambda, response_mutex);    
        // increment the number of executed tasks in sequence
        total_num_of_executed_tasks_in_sequence++;
        current_num_of_executed_tasks_in_sequence++;
        // Increment the next sequence to send
        next_seq_to_send = seqGetNext(next_seq_to_send);
        // Notify all waiting threads - can be on alloc in full buffer case or wait on specific sequence
        cv.notify_all();
        // Continue sending any subsequent responses that are ready
        executeNextSequences();
    } else {
        // If the sequence is not the next to send, store it in the map
        seq_data_t seq_data;
        seq_data.response_lambda = response_lambda;
        seq_data.response_mutex = response_mutex;

        responses[seq] = seq_data;  
        status = sequencer::SUCCESS;
        LogToModuleFile("1", "Sequence not in order, stored in map seq {}", seq);
        num_of_out_of_sequence_functions++;
    }
    // stats max consecutive responses executed in sequence
    max_num_of_executed_tasks_in_sequence = (current_num_of_executed_tasks_in_sequence > max_num_of_executed_tasks_in_sequence)  ?
            current_num_of_executed_tasks_in_sequence : max_num_of_executed_tasks_in_sequence;
    // unlock the mutex
    mtx.unlock();
    return status;
}

sequencer::SequenceStatus Sequencer::showStatistics() {
    std::unique_lock<std::mutex> lock(mtx);
    std::string logMsg = "STATISTICS: \n";
    logMsg = "multithreaded: total number of executed tasks in sequence: " + std::to_string(total_num_of_executed_tasks_in_sequence) + " \n";
    logMsg += "multithreaded: number of null functions: " + std::to_string(num_of_null_functions) + " \n";
    logMsg += "multithreaded: number of out of sequence functions: " + std::to_string(num_of_out_of_sequence_functions) + " \n";
    logMsg += std::to_string(current_seq) + " out of " + std::to_string(max_seq_num) + "used";
    LogToModuleFile("2", logMsg);
    return sequencer::SUCCESS;
}

sequencer::SequenceStatus Sequencer::clearStatistics() {
    std::unique_lock<std::mutex> lock(mtx);
    total_num_of_executed_tasks_in_sequence = 0;
    num_of_null_functions = 0;
    num_of_out_of_sequence_functions = 0;
    LogToModuleFile("2", "CLEANED STATISTICS \n");
    return sequencer::SUCCESS;
}

sequencer::SequenceStatus Sequencer::waitSequenceNumber(int seq_num){
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [seq_num, this] { return (isExit() ||  amINext(seq_num)); });
    return sequencer::SUCCESS;
}

bool Sequencer::isFull() {
    bool ret;
    ret = (responses.size() < max_seq_num) ? false : true;
    return ret;
}

int Sequencer::seqGetNext(int seq_num) {
    int next = seq_num + 1;
    if (next >= MAX_SEQUENCE_NUMBER) {
        next = 0;
    }
    return next;
}

bool Sequencer::amINext(int seq_num) {
    bool ret = false;
    int next = seqGetNext(next_seq_to_send);
    ret =  (seq_num == next) ? true : false;
    return ret;
}

void Sequencer::flush() {
    // mutex lock
    std::unique_lock<std::mutex> lock(mtx);
    // mtx.lock();
    while (current_seq != next_seq_to_send) {
        // Check if the next sequence number is in the map
        auto seq_data = responses.find(next_seq_to_send);
        if (seq_data != responses.end()) {
            // Execute the stored lambda
            callSeq(seq_data->second.response_lambda, seq_data->second.response_mutex);
            // Safely erase the entry
            responses.erase(seq_data);  
        }
        // Increment the sequence number
        next_seq_to_send=seqGetNext(next_seq_to_send); 
    }
    // notify all waiting threads - on full or on wait specific sequence
    cv.notify_all();
    // mutex unlock
    // mtx.unlock();
    return;
}