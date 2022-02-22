/*
 * Copyright (c) 2014-2022 The Voxie Authors
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

#pragma once

#include <VoxieClient/VoxieClient.hpp>

#include <QtCore/QSharedPointer>

namespace vx {
namespace SharedFunPtrIntern {
VOXIECLIENT_EXPORT Q_NORETURN void throwEmptyFunctionPointer();
}

/**
 * SharedFunPtr is a type similar to std::function, but uses a shared pointer to
 * avoid copying the function object when the SharedFunPtr is copied.
 */
template <typename T>
class SharedFunPtr;
template <typename Ret, typename... Args>
class VOXIECLIENT_EXPORT SharedFunPtr<Ret(Args...)> {
  struct ImplBase {
    virtual ~ImplBase() {}

    virtual Ret call(Args... args) = 0;
  };

  template <typename T>
  struct Impl : ImplBase {
    T fun;

    ~Impl() {}

    template <typename F>
    Impl(const F& fun) : fun(fun) {}
    template <typename F>
    Impl(F&& fun) : fun(std::move(fun)) {}

    Ret call(Args... args) override { return fun(std::forward<Args>(args)...); }
  };

  QSharedPointer<ImplBase> ptr;

  // Do not allow T to be the same as the current type
  template <typename T>
  using DisallowSameType =
      std::enable_if_t<!std::is_same<SharedFunPtr, std::decay_t<T>>::value,
                       void>;

 public:
  using SignatureType = Ret(Args...);
  using ReturnType = Ret;

  SharedFunPtr() {}
  SharedFunPtr(std::nullptr_t) {}

  template <typename T, typename = DisallowSameType<T>>
  SharedFunPtr(const T& fun) : ptr(new Impl<std::decay_t<T>>(fun)) {}
  template <typename T, typename = DisallowSameType<T>>
  SharedFunPtr(T&& fun) : ptr(new Impl<std::decay_t<T>>(std::move(fun))) {}

  template <typename... OpArgs>
  Ret operator()(OpArgs&&... args) const {
    if (Q_UNLIKELY(!ptr)) SharedFunPtrIntern::throwEmptyFunctionPointer();
    return ptr->call(std::forward<OpArgs>(args)...);
  }

  explicit operator bool() const noexcept { return static_cast<bool>(ptr); }
};

template <typename Ret, typename... Args>
inline bool operator==(const SharedFunPtr<Ret(Args...)>& fun,
                       std::nullptr_t) noexcept {
  return !fun;
}
template <typename Ret, typename... Args>
inline bool operator==(std::nullptr_t,
                       const SharedFunPtr<Ret(Args...)>& fun) noexcept {
  return !fun;
}

template <typename Ret, typename... Args>
inline bool operator!=(const SharedFunPtr<Ret(Args...)>& fun,
                       std::nullptr_t) noexcept {
  return static_cast<bool>(fun);
}
template <typename Ret, typename... Args>
inline bool operator!=(std::nullptr_t,
                       const SharedFunPtr<Ret(Args...)>& fun) noexcept {
  return static_cast<bool>(fun);
}

}  // namespace vx
