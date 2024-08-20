#include <iostream>
#include <mutex>
#include <condition_variable>
#include <map>

class Sequencer {
public:
    Sequencer() : current_seq(0), next_seq_to_send(1) {}

    // Receive a lambda and wrap it with a sequence number
    std::function<void()> addTask(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(mtx);

        // Automatically reset sequence numbers if they approach the limit
        if (current_seq >= std::numeric_limits<int>::max()) {
            resetSequence();
        }

        int seq = ++current_seq;

        // Return a new lambda with the sequence number embedded
        return [this, seq, task]() {
            task();  // Execute the original task

            std::lock_guard<std::mutex> lock(mtx);
            responses[seq] = "Response for sequence " + std::to_string(seq);  // Mock response
            //cv.notify_one();  // Notify the thread waiting to send the response
        };
    }

    // Wait and send responses in the correct order
    void sendResponses() {
        std::unique_lock<std::mutex> lock(mtx);
        while (true) {
            cv.wait(lock, [this] { return responses.count(next_seq_to_send) > 0; });
            std::cout << "Sending: " << responses[next_seq_to_send] << std::endl;
            responses.erase(next_seq_to_send);
            ++next_seq_to_send;
        }
    }

private:
    int current_seq;  // Tracks the latest sequence number assigned to a task
    int next_seq_to_send;  // The next sequence number that should be sent
    std::mutex mtx;  // Protects shared data
    std::condition_variable cv;  // Notifies when a response is ready to be sent
    std::map<int, std::string> responses;  // Stores responses by sequence number

    // Reset the sequence number to avoid overflow
    void resetSequence() {
        current_seq = 0;
        std::cout << "Sequence numbers have been reset." << std::endl;
    }
};
