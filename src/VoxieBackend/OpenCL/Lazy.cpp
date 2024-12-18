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

#include "Lazy.hpp"

#include <QtCore/QLibrary>

using namespace vx::opencl;

OpenCLLoadException::OpenCLLoadException(const QString& msg)
    : CLException(0, msg) {
  this->message = msg;
  this->onResolveSymbol_ = false;
  this->symbol_ = "";
  this->messageLong = ("Error loading OpenCL library: " + msg).toStdString();
}
OpenCLLoadException::OpenCLLoadException(const QString& msg,
                                         const QString& symb)
    : CLException(0, msg) {
  this->message = msg;
  this->onResolveSymbol_ = true;
  this->symbol_ = symb;
  this->messageLong =
      ("Error while resolving OpenCL symbol: " + symb + ": " + msg)
          .toStdString();
}
OpenCLLoadException::~OpenCLLoadException() {}

OpenCLLoadException* OpenCLLoadException::clone() const {
  return new OpenCLLoadException(*this);
}

const char* OpenCLLoadException::what() const NOEXCEPT {
  return messageLong.c_str();
}

namespace {
typedef QFunctionPointer SymbolType;

template <typename T>
inline T toFunPtr(SymbolType ptr) {
  static_assert(sizeof(SymbolType) == sizeof(T),
                "T has to have the same size as QFunctionPointer");
  T res;
  memcpy(&res, &ptr, sizeof(SymbolType));
  return res;
}

QLibrary* loadOpenLibrary() {
#if defined(Q_OS_LINUX)
  // This is needed if the OpenCL lib dev files (ocl-icd-opencl-dev on Debian)
  // are not installed.
  QScopedPointer<QLibrary> lib1(new QLibrary("OpenCL.so.1"));
  if (lib1->load()) {
    return lib1.take();
  }
#endif
  QScopedPointer<QLibrary> lib(new QLibrary("OpenCL"));
  if (!lib->load()) {
    throw OpenCLLoadException(lib->errorString());
  }
  return lib.take();
}
QLibrary* getOpenLibrary() {
  // Note: the loaded library will never be freed (to avoid problems when the
  // OpenCL library is unloaded)
  static QLibrary* lib = loadOpenLibrary();
  return lib;
}

bool haveOpenCLSymbol(const char* name) {
  auto lib = getOpenLibrary();
  auto symb = lib->resolve(name);
  return symb != nullptr;
}
bool lazyHaveClRetainDevice() {
  static bool haveRetainDevice = haveOpenCLSymbol("clRetainDevice");
  return haveRetainDevice;
}

static cl_int fakeRetainReleaseDevice(cl_device_id device) {
  // Before OpenCL 1.2 there is no clRetainDevice() / clReleaseDevice(), all
  // those calls can be ignored there.
  (void)device;
  return CL_SUCCESS;
}

SymbolType loadOpenCLSymbol(const char* name) {
  auto lib = getOpenLibrary();

  if (strcmp(name, "clRetainDevice") == 0 ||
      strcmp(name, "clReleaseDevice") == 0) {
    if (!lazyHaveClRetainDevice()) {
      return reinterpret_cast<SymbolType>(fakeRetainReleaseDevice);
    }
  }

  auto symb = lib->resolve(name);
  if (!symb) {
    throw OpenCLLoadException(lib->errorString(), name);
  }
  return symb;
}
}  // namespace

// Using STRINGIFY(x) instead of just #x makes sure that macros are expanded
#define STRINGIFY(x) #x

#define OPENCL_LAZY_DEFINE(n)                                          \
  decltype(::n)* n() {                                                 \
    typedef decltype(::n)* Ptr;                                        \
    static Ptr cached = toFunPtr<Ptr>(loadOpenCLSymbol(STRINGIFY(n))); \
    return cached;                                                     \
  }

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
// Ignore deprecated warnings for old OpenCL functions
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace cl {
namespace OpenCLLibLazy {
OPENCL_LAZY_SYMBOLS(OPENCL_LAZY_DEFINE)
}
}  // namespace cl

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
