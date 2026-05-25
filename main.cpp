#include "RingBuffer.h"

std::vector<std::string> scenario = {
    "P", "C", "C", "C", "P", "P", "P", "P", "P",
    "P", "P", "P", "C", "P", "P", "P", "C", "C", "C",
    "C", "C", "C", "C", "C"
};

int main() {
    RingBufferManager<std::string> rbm;
    std::vector<std::thread> workers;

    for (const auto& action : scenario) {
        if (action == "P") {
            workers.emplace_back(&RingBufferManager<std::string>::producer, &rbm);
        }   
        else
        {
            workers.emplace_back(&RingBufferManager<std::string>::consumer, &rbm);
        }
    
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }

    std::cout << "종료 완료" << std::endl;
    return 0;
}