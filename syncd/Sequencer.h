#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <condition_variable>
#include <map>
#include <mutex>
#include <iostream>

class Sequencer {
public:
    // Static method to provide access to the single instance
    static Sequencer& getInstance();

    // Get sequence number
    int getSequenceNumber();

    // Add sequence response
    void addSequenceResponse(const void* response_lambda, int seq);

private:
    // Private constructor
    Sequencer();

    // Delete the copy constructor and assignment operator
    Sequencer(const Sequencer&) = delete;
    Sequencer& operator=(const Sequencer&) = delete;

    // Reset the sequence number to avoid overflow
    void resetSequence();

    int current_seq;  // Tracks the latest sequence number assigned to a task
    int next_seq_to_send;  // The next sequence number that should be sent
    std::mutex mtx;  // Protects shared data
    std::condition_variable cv;  // Notifies when a response is ready to be sent
    std::map<int, const void*> responses;  // Stores responses by sequence number
};

#endif // SEQUENCER_H