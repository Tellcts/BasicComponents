#pragma once
#ifndef _THREAD_POOL_H_
  #define _THREAD_POOL_H_

  #include <condition_variable>
  #include <functional>
  #include <future>
  #include <memory>
  #include <mutex>
  #include <queue>
  #include <stdexcept>
  #include <thread>
  #include <vector>

  #include "../DesignPattern/Singleton/singleton.h"

class ThreadPool : public Singleton<ThreadPool> {
    friend class Singleton<ThreadPool>;

public:
    template <typename Func, typename... Args>
    auto enqueue(Func&& __func, Args&&... __args)
        -> std::future<typename std::invoke_result<Func, Args...>::type>;

private:
    explicit ThreadPool(size_t __threads = 4);
    ~ThreadPool();
    // 工作线程容器
    std::vector<std::thread> workers_;
    // 任务队列
    std::queue<std::function<void()>> tasks_;
    // 用于实现同步的变量
    std::mutex queue_mtx_;
    std::condition_variable cv_;
    bool stop_ = false;
};

inline ThreadPool::ThreadPool(size_t __threads) : stop_(false) {
    for (size_t i = 0; i < __threads; ++i) {
        workers_.emplace_back([this]() -> void {
            while (1) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mtx_);
                    this->cv_.wait(lock, [this]() {
                        return this->stop_ || ! this->tasks_.empty();
                    });
                    if (this->stop_ && this->tasks_.empty()) {
                        return;
                    }
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                task();
            }
        });
    }
}

template <typename Func, typename... Args>
auto ThreadPool::enqueue(Func&& __func, Args&&... __args)
    -> std::future<typename std::invoke_result<Func, Args...>::type> {
    using return_type = typename std::invoke_result<Func, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<Func>(__func), std::forward<Args>(__args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mtx_);
        if (stop_) throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks_.emplace([task]() {
            (*task)();
        });
        cv_.notify_one();
        return res;
    }
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mtx_);
        stop_ = true;
    }
    cv_.notify_all();
    for (std::thread& worker : workers_) {
        worker.join();
    }
}
#endif
