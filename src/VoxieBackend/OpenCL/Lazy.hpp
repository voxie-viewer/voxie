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

#include <VoxieBackend/OpenCL/CLUtil.hpp>

#include <VoxieBackend/OpenCL/OpenCLCPP.hpp>

namespace vx {
namespace opencl {

/**
 * Exception class for errors while loading the OpenCL library.
 */
class VOXIEBACKEND_EXPORT OpenCLLoadException : public CLException {
 private:
  QString message;
  bool onResolveSymbol_;
  QString symbol_;
  std::string messageLong;

 public:
  OpenCLLoadException(const QString& msg);
  OpenCLLoadException(const QString& msg, const QString& symb);
  ~OpenCLLoadException();

  /**
   * @return the error message returned by QLibrary::errorString()
   */
  const QString& getMessage() const { return this->message; }

  /**
   * @return false if the error occured while loading the library, true if the
   * error occured while resolving a symbol
   */
  bool onResolveSymbol() const { return this->onResolveSymbol_; }

  /**
   * @return the symbol which caused the error, an empty string if the error
   * occured while loading the library
   */
  const QString& symbol() const { return this->symbol_; }

  /**
   * @return clone of this exception
   */
  OpenCLLoadException* clone() const override;

  /**
   * @brief throws this exception
   */
  void raise() const override { throw *this; }

  const char* what() const NOEXCEPT override;
};

}  // namespace opencl
}  // namespace vx
