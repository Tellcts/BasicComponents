#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>

#include "thread_pool.h"

std::mutex mtx;

int main() {
    auto& pool = ThreadPool::getInstance();
    std::vector<std::future<int>> results;
    for (int i = 0; i < 8; ++i) {
        results.emplace_back(pool.enqueue([i]() {
            mtx.lock();
            std::cout << "hello " << i << '\n';
            mtx.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            mtx.lock();
            std::cout << "world " << i << '\n';
            mtx.unlock();
            return i * i;
        }));
    }
    std::cout << '\n';
    for (auto& result : results) {
        std::cout << result.get() << ' ';
    }
    std::cout << '\n';
    return 0;
}
