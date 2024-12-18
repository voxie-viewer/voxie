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

#include "StaticCache.hpp"

#include <Core/Exception.hpp>
#include <Core/Type.hpp>

#include <cstdio>

// TODO: Make exceptions public?
class AtexitFailedException : public Core::Exception {
  const std::type_info& retType;
  const std::type_info& lambdaType;

 public:
  AtexitFailedException(const std::type_info& retType,
                        const std::type_info& lambdaType)
      : retType(retType), lambdaType(lambdaType) {}

  std::string message() const override {
    return "StaticCache: atexit() failed for return type " +
           Core::Type::getName(retType) + " with lambda " +
           Core::Type::getName(lambdaType);
  }
};
class DeadlockException : public Core::Exception {
  const std::type_info& retType;
  const std::type_info& lambdaType;

 public:
  DeadlockException(const std::type_info& retType,
                    const std::type_info& lambdaType)
      : retType(retType), lambdaType(lambdaType) {}

  std::string message() const override {
    return "StaticCache: deadlock for return type " +
           Core::Type::getName(retType) + " with lambda " +
           Core::Type::getName(lambdaType);
  }
};

NORETURN Core::StaticCachePrivate::throwAtexitFailed(
    const std::type_info& retType, const std::type_info& lambdaType) {
  throw AtexitFailedException(retType, lambdaType);
}

NORETURN Core::StaticCachePrivate::throwDeadlock(
    const std::type_info& retType, const std::type_info& lambdaType) {
  throw DeadlockException(retType, lambdaType);
}

NORETURN Core::StaticCachePrivate::abortInitializingThreadNotEmpty() {
  fprintf(stderr, "StaticCachePrivate: initializingThread not empty\n");
  fflush(stderr);
  abort();
}
