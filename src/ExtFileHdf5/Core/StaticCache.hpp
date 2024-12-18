/*
 * Copyright (c) 2021 Steffen Kieß
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CORE_STATICCACHE_HPP_INCLUDED
#define CORE_STATICCACHE_HPP_INCLUDED

#include <Core/Util.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <typeinfo>

#include <cstdlib>

#ifndef CORE_FORCE_LEAKY_MUTEX
#define CORE_FORCE_LEAKY_MUTEX 0
#endif

namespace Core {
// TODO: Move somewhere else?
// A mutex, similar to std::mutex, but will always have a trivial destructor
// Note that this will leak resources on destruction
class LeakyMutex {
  // TODO: This would probably be nicer if it were possible to just skip the dtor, but aligned_storage + placement new does not work because placement new does not work in a constexpr context
  //std::aligned_storage<sizeof(std::mutex), alignof(std::mutex)>::type storage;
  //alignas(std::mutex) std::uint8_t storage[sizeof(std::mutex)];
  std::atomic<std::mutex*> ptr;

 public:
  constexpr LeakyMutex() : ptr(nullptr) {}

  std::mutex* getMutex() {
    auto value = ptr.load();
    if (value) return value;

    // A new mutex has to be created
    auto newPtr = new std::mutex;  // Can throw
    std::mutex* expected = nullptr;
    if (ptr.compare_exchange_strong(expected, newPtr)) {
      // Exchange worked, old value was still nullptr
      return newPtr;
    }

    // Exchange failed, someone else already created a mutex
    delete newPtr;
    // expected should contain the actual value
    return expected;
  }

  operator std::mutex&() { return *getMutex(); }
};
static_assert(std::is_trivially_destructible<LeakyMutex>::value,
              "LeakyMutex is not trivially destructible");
const bool useLeakyMutex = CORE_FORCE_LEAKY_MUTEX ||
                           !std::is_trivially_destructible<std::mutex>::value;
using TrivialDTorMutex =
    std::conditional<useLeakyMutex, LeakyMutex, std::mutex>::type;

template <typename T, bool allowStatefulLambda = false, typename F>
T staticCache(const F& f);
class StaticCachePrivate {
  STATIC_CLASS(StaticCachePrivate);

  template <typename T, bool allowStatefulLambda, typename F>
  friend T staticCache(const F& f);

  static NORETURN throwAtexitFailed(const std::type_info& retType,
                                    const std::type_info& lambdaType);
  static NORETURN throwDeadlock(const std::type_info& retType,
                                const std::type_info& lambdaType);
  static NORETURN abortInitializingThreadNotEmpty();
};

/**
 * This function allows caching a value until the program terminates.
 *
 * Unlike block-scope objects with static storage duration, it should
 * handle being used during the destruction of static storage objects
 * correctly.
 *
 * F must be different for different caches (should be automatically the case
 * for lambdas).
 */
template <typename T, bool allowStatefulLambda, typename F>
T staticCache(const F& f) {
  // Note return a copy instead of a reference to make sure that the return value remains valid even if the atexit handler is running in parallel

  static_assert(!std::is_reference<T>::value, "T must not be a reference");

  // Try to make sure that the callback is a stateless lambda
  // Note that this is not fool-proof: You can e.g. pass in different function pointers on different invocations here.
  using LambdaRetType =
      decltype(f());  // Might be different from T (but must be convertible to T)
  using FunctionPtr = LambdaRetType (*)();
  using CastPtr =
      typename std::conditional<allowStatefulLambda, F, FunctionPtr>::type;
  CastPtr ptr = f;
  (void)ptr;

  // This data is stored statically. It must not use dynamic initialization and
  // must have a trivial destructor.
  struct Data {
    // must not be acquired while initializingThreadMutex is held
    TrivialDTorMutex mutex;
    T* ptr;

    // Simple deadlock detection
    TrivialDTorMutex initializingThreadMutex;
    std::thread::id* initializingThread;

    constexpr Data() : ptr(nullptr), initializingThread(nullptr) {}
  };

  // Try to make sure the type actually does not use dynamic initialization

  static_assert(std::is_trivially_destructible<Data>::value,
                "staticCache()::Data is not trivially destructible");

  // This hopefully should make sure that the constructor is constexpr etc., i.e. that only static initialization is used.
  [[gnu::unused]] constexpr Data fakeData;

  // Note: Starting with C++20 this should use constinit (+ the check for the trivial destructor)
  // https://en.cppreference.com/w/cpp/language/constinit
  static Data data;

  struct InitializingThreadCleanup {
    ~InitializingThreadCleanup() {
      std::lock_guard<std::mutex> guard(data.initializingThreadMutex);
      auto ptr = data.initializingThread;
      data.initializingThread = nullptr;
      delete ptr;
    }
  };

  {
    // Check for deadlocks
    // Note: This will not detect deadlocks involving more than one thread
    std::lock_guard<std::mutex> guardTM(data.initializingThreadMutex);
    if (data.initializingThread &&
        *data.initializingThread == std::this_thread::get_id()) {
      StaticCachePrivate::throwDeadlock(typeid(T), typeid(F));
    }
  }

  std::lock_guard<std::mutex> guard1(data.mutex);
  if (data.ptr) return *data.ptr;

  // A new value has to be created

  {
    std::lock_guard<std::mutex> guardTM(data.initializingThreadMutex);
    if (data.initializingThread) {
      // Should never happen because we are holding data.mutex
      StaticCachePrivate::abortInitializingThreadNotEmpty();
    }
  }

  InitializingThreadCleanup cleanup;

  {
    std::lock_guard<std::mutex> guardTM(data.initializingThreadMutex);
    data.initializingThread = new std::thread::id(std::this_thread::get_id());
  }

  data.ptr = new T(f());
  if (std::atexit([]() {
        std::lock_guard<std::mutex> guard2(data.mutex);
        auto res = data.ptr;
        data.ptr = nullptr;
        delete res;
      })) {
    // atexit handler registration failed
    auto res = data.ptr;
    data.ptr = nullptr;
    delete res;
    StaticCachePrivate::throwAtexitFailed(typeid(T), typeid(F));
  }
  return *data.ptr;
}

template <bool allowStatefulLambda = false, typename F>
auto staticCache(const F& f) -> decltype(f()) {
  return staticCache<decltype(f()), allowStatefulLambda, F>(f);
}

}  // namespace Core

#endif  // !CORE_STATICCACHE_HPP_INCLUDED
