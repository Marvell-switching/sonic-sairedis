#include "Sequencer.h"

// Helper function to execute all ready responses in order
void Sequencer::executeReadyResponses() {
    printf("Checking for ready responses in queue...\n");
    while (true) {
        auto it = responses.find(next_seq_to_send);
        if (it == responses.end()) {
            break;  // Exit loop if the next sequence is not in the map
        }
        it->second();  // Execute the stored lambda
        responses.erase(it);  // Safely erase the entry
        ++next_seq_to_send; // Increment the sequence number
    }
}

// Get sequence number
int Sequencer::allocateSequenceNumber() {
    std::lock_guard<std::mutex> lock(mtx);
    int seq = current_seq;
    current_seq++;
    return seq;
}

// Add/Execute sequence function
void Sequencer::executeFuncInSequence(int seq, std::function<void()> response_lambda) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (seq == next_seq_to_send) {
        // If the received sequence is the next one to send, execute it immediately
        printf("Executing lambda\n");
        response_lambda();
        // Increment the next sequence to send
        ++next_seq_to_send;
        // Continue sending any subsequent responses that are ready
        executeReadyResponses();
    } else {
        // If the sequence is not the next to send, store it in the map
        responses[seq] = response_lambda;
        printf("Storing lambda with seq: %d, next to send: %d\n", seq, next_seq_to_send);
    }
}