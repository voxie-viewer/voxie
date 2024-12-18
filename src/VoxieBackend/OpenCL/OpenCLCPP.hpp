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

#ifndef OPENCL_OPENCLCPP_HPP_DEFINED
#define OPENCL_OPENCLCPP_HPP_DEFINED

#ifdef __CL_VERSION_H
#warning cl_version.h already included
#endif
#ifdef __OPENCL_CL_H
#warning cl.h already included
#endif

// Use CL/opencl.h even on MacOS because the OpenCL headers should come from a
// local copy of the headers anyway.
#define CL_HPP_OPENCL_HEADER_FILE <CL/opencl.h>

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS 1
#define CL_TARGET_OPENCL_VERSION 300
#define CL_HPP_TARGET_OPENCL_VERSION 300

// TODO: Remove
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
// TODO: Remove
#define CL_HPP_ENABLE_SIZE_T_COMPATIBILITY 1

// TODO: ?
#define CL_HPP_CL_1_2_DEFAULT_BUILD 1

// TODO: Use exceptions for error handling
//#include <VoxieBackend/OpenCL/Error.hpp>
//#define CL_HPP_ENABLE_EXCEPTIONS 1
//#define CL_HPP_CUSTOM_EXCEPTION_TYPE vx::OpenCLError

#define CL_HPP_NO_DEFAULT_OBJECTS 1

#include <QtCore/QtGlobal>
#if defined(VOXIEBACKEND_LIBRARY)
#define OPENCL_LAZY_EXPORT Q_DECL_EXPORT
#else
#define OPENCL_LAZY_EXPORT Q_DECL_IMPORT
#endif
#include <VoxieBackend/OpenCL/LazySymbols.hpp>
#define CL_HPP_OPENCL_API_WRAPPER OPENCL_LAZY

#include <VoxieBackend/OpenCL/opencl-patched.hpp>

#endif  // !OPENCL_OPENCLCPP_HPP_DEFINED
