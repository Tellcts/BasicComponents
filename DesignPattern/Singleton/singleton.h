#pragma once

#ifndef _SINGLETON_SINGLETON_H_
#define _SINGLETON_SINGLETON_H_

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <mutex>

namespace V1 {
// 线程安全的双检查版本
class Singleton {
 public:
  static Singleton* getInstance() {
    Singleton* tmp = instance_.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    if (tmp == nullptr) {
      std::lock_guard<std::mutex> lock(mutex_);
      tmp = instance_.load(std::memory_order_relaxed);
      if (tmp == nullptr) {
        tmp = new Singleton();
        std::atomic_thread_fence(std::memory_order_release);
        instance_.store(tmp, std::memory_order_relaxed);
        std::atexit(Destructor);
      }
    }
    return tmp;
  }

 private:
  Singleton() { std::cout << "V1:Singleton()\n"; }
  ~Singleton() { std::cout << "V1:~Singleton()\n"; }
  static void Destructor() {
    Singleton* tmp = instance_.load(std::memory_order_relaxed);
    if (tmp != nullptr) {
      delete tmp;
    }
  }

  Singleton(const Singleton&) = delete;
  Singleton(Singleton&&) noexcept = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton& operator=(Singleton&&) noexcept = delete;

  static std::mutex mutex_;
  static std::atomic<Singleton*> instance_;
};
}  // namespace V1

namespace V2 {
// 天生线程安全版本
class Singleton {
 public:
  static Singleton& getInstance() {
    static Singleton instance{};
    return instance;
  }

 private:
  Singleton() { std::cout << "V2:Singleton()\n"; }
  ~Singleton() { std::cout << "V2:~Singleton()\n"; }
  Singleton(const Singleton&) = delete;
  Singleton(Singleton&&) noexcept = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton& operator=(Singleton&&) noexcept = delete;
};
}  // namespace V2

inline namespace V3 {
// 封装单例通用逻辑，后去每个单例类只须继承该类即可
template <typename T>
class Singleton {
 public:
  static T& getInstance() {
    static T instance{};
    return instance;
  }

 protected:
  Singleton() { std::cout << "V3:Singleton()\n"; }
  virtual ~Singleton() { std::cout << "V3:~Singleton()\n"; }

 private:
  Singleton(const Singleton&) = delete;
  Singleton(Singleton&&) noexcept = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton& operator=(Singleton&&) noexcept = delete;
};

class Foo : public Singleton<Foo> {
  friend class Singleton<Foo>;

 public:
  // ...
 private:
  Foo() { std::cout << "V3:Foo()\n"; }
  ~Foo() { std::cout << "V3:~Foo()\n"; }
};
}  // namespace V3

#endif