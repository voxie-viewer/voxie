/*
 * Copyright (c) 2010-2012 Steffen Kieß
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

#ifndef CORE_UTIL_HPP_INCLUDED
#define CORE_UTIL_HPP_INCLUDED

// Various preprocessor macros for C++

#include <Core/Util.h>

// Make sure there is (at least some) support for C++11
#if !(defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L || (defined (_MSC_VER) && _MSC_VER >= 1800))
#warning "The C++ version seems to be before C++11, this will probably not work"
#endif
#define HAVE_CXX11 1

#define NO_COPY_CLASS(n)                        \
  private:                                      \
  n& operator= (const n &x) = delete;           \
  n (const n &x) = delete

#define STATIC_CLASS(n)                         \
  NO_COPY_CLASS (n);                            \
private:                                        \
 n () = delete;                                 \
 ~n () = delete

#ifdef __CUDACC__
#define NVCC_HOST_DEVICE __host__ __device__
#else
#define NVCC_HOST_DEVICE
#endif

#define DECLTYPE(...) decltype (__VA_ARGS__)

#endif // !CORE_UTIL_HPP_INCLUDED
