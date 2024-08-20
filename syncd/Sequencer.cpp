#include "Sequencer.h"
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <map>
#include <functional>

//TODO add locking mechanism 

class Sequencer {

private:
    int current_seq;  // Tracks the latest sequence number assigned to a task
    int next_seq_to_send;  // The next sequence number that should be sent
    std::mutex mtx;  // Protects shared data
    std::map<int, std::function<void()>> responses;  // Stores responses by sequence number

    // Private constructor 
    Sequencer() : current_seq(0), next_seq_to_send(0) {}

    // Delete the copy constructor and assignment operator
    Sequencer(const Sequencer&) = delete;
    Sequencer& operator=(const Sequencer&) = delete;

    // Reset the sequence number to avoid overflow
    void resetSequence() {
        current_seq = 0;
        std::cout << "Sequence numbers have been reset." << std::endl;
    }

public:
    // Static method to provide access to the single instance (reference)
    static Sequencer& getInstance() {
        static Sequencer instance;
        return instance;
    }

    // Get sequence number
    int getSequenceNumber() {
        std::lock_guard<std::mutex> lock(mtx);
        int seq = current_seq;
        current_seq++;
        return seq;
    }

    // Add sequence response
    void addSequenceResponse(std::function<void()> response_lambda, int seq) {
        std::lock_guard<std::mutex> lock(mtx);
        responses[seq] = response_lambda;
    }

    // Loop and send responses in the correct order
    void loopAndSendSequencedResponse() {
        while (true) {
            int seq_to_remove = -1;
            std::function<void()> response_to_execute = NULL;
            
            {
                std::lock_guard<std::mutex> lock(mtx);

                // Find the response with the next sequence number
                // If not found, loop for a response to be added
                auto it = responses.find(next_seq_to_send);
                if (it != responses.end()) {
                    // Save the response lambda
                    response_to_execute = it->second;
                    seq_to_remove = next_seq_to_send;
                    ++next_seq_to_send;
                }
            }

            // Execute the response lambda
            if(response_to_execute) {
                response_to_execute();
            }

            // Remove sent response from the map safely
            if(seq_to_remove != -1) {
                std::lock_guard<std::mutex> lock(mtx);
                responses.erase(seq_to_remove);
                seq_to_remove = -1;
            }
            
        }
        
    }
};
