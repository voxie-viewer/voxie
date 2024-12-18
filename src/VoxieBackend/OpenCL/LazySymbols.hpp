/*
 * Copyright (c) 2018 Steffen Kieß
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

#ifndef OPENCL_CL_LAZY_HPP_DEFINED
#define OPENCL_CL_LAZY_HPP_DEFINED

#if defined(CL_HPP_OPENCL_HEADER_FILE)
#include CL_HPP_OPENCL_HEADER_FILE
#elif defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif  // !__APPLE__

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

// Helpers for loading OpenCL library lazily at runtime

// clang-format off
// cat lib/OpenCL-Headers/CL/cl{,_gl}.h | tr '\n' ' ' | sed 's/;/;\n/g' | grep 'extern CL_API_ENTRY' | grep CL_API_CALL | sed 's/.*CL_API_CALL *//'  | while read a b; do V=1_0; if echo "$b" | grep -q "CL_API_SUFFIX__VERSION_"; then V="$(echo "$b" | sed 's/.*CL_API_SUFFIX__VERSION_//;s/;//')"; fi; echo "$V ${a%%(*}"; done | sed 's/^[0-9]\+_[0-9]\+_DEPRECATED/1_0/' | sort | grep -v 'clSetCommandQueueProperty' | (V=; while read v a; do if [ "$V" != "$v" ]; then V="$v"; echo; echo "#define OPENCL_LAZY_SYMBOLS_$V(SYM)\\"; fi; echo "SYM($a) \\"; done ) | clang-format
// clang-format on

#define OPENCL_LAZY_SYMBOLS_1_0(SYM)       \
  SYM(clBuildProgram)                      \
  SYM(clCreateBuffer)                      \
  SYM(clCreateCommandQueue)                \
  SYM(clCreateContext)                     \
  SYM(clCreateContextFromType)             \
  SYM(clCreateFromGLBuffer)                \
  SYM(clCreateFromGLRenderbuffer)          \
  SYM(clCreateFromGLTexture2D)             \
  SYM(clCreateFromGLTexture3D)             \
  SYM(clCreateImage2D)                     \
  SYM(clCreateImage3D)                     \
  SYM(clCreateKernel)                      \
  SYM(clCreateKernelsInProgram)            \
  SYM(clCreateProgramWithBinary)           \
  SYM(clCreateProgramWithSource)           \
  SYM(clCreateSampler)                     \
  SYM(clEnqueueAcquireGLObjects)           \
  SYM(clEnqueueBarrier)                    \
  SYM(clEnqueueCopyBuffer)                 \
  SYM(clEnqueueCopyBufferToImage)          \
  SYM(clEnqueueCopyImage)                  \
  SYM(clEnqueueCopyImageToBuffer)          \
  SYM(clEnqueueMapBuffer)                  \
  SYM(clEnqueueMapImage)                   \
  SYM(clEnqueueMarker)                     \
  SYM(clEnqueueNativeKernel)               \
  SYM(clEnqueueNDRangeKernel)              \
  SYM(clEnqueueReadBuffer)                 \
  SYM(clEnqueueReadImage)                  \
  SYM(clEnqueueReleaseGLObjects)           \
  SYM(clEnqueueTask)                       \
  SYM(clEnqueueUnmapMemObject)             \
  SYM(clEnqueueWaitForEvents)              \
  SYM(clEnqueueWriteBuffer)                \
  SYM(clEnqueueWriteImage)                 \
  SYM(clFinish)                            \
  SYM(clFlush)                             \
  SYM(clGetCommandQueueInfo)               \
  SYM(clGetContextInfo)                    \
  SYM(clGetDeviceIDs)                      \
  SYM(clGetDeviceInfo)                     \
  SYM(clGetEventInfo)                      \
  SYM(clGetEventProfilingInfo)             \
  SYM(clGetExtensionFunctionAddress)       \
  SYM(clGetGLContextInfoKHR)               \
  SYM(clGetGLObjectInfo)                   \
  SYM(clGetGLTextureInfo)                  \
  SYM(clGetImageInfo)                      \
  SYM(clGetKernelInfo)                     \
  SYM(clGetKernelWorkGroupInfo)            \
  SYM(clGetMemObjectInfo)                  \
  SYM(clGetPlatformIDs)                    \
  SYM(clGetPlatformInfo)                   \
  SYM(clGetProgramBuildInfo)               \
  SYM(clGetProgramInfo)                    \
  SYM(clGetSamplerInfo)                    \
  SYM(clGetSupportedGLTextureFormatsINTEL) \
  SYM(clGetSupportedImageFormats)          \
  SYM(clReleaseCommandQueue)               \
  SYM(clReleaseContext)                    \
  SYM(clReleaseEvent)                      \
  SYM(clReleaseKernel)                     \
  SYM(clReleaseMemObject)                  \
  SYM(clReleaseProgram)                    \
  SYM(clReleaseSampler)                    \
  SYM(clRetainCommandQueue)                \
  SYM(clRetainContext)                     \
  SYM(clRetainEvent)                       \
  SYM(clRetainKernel)                      \
  SYM(clRetainMemObject)                   \
  SYM(clRetainProgram)                     \
  SYM(clRetainSampler)                     \
  SYM(clSetKernelArg)                      \
  SYM(clSetProgramReleaseCallback)         \
  SYM(clUnloadCompiler)                    \
  SYM(clWaitForEvents)

#define OPENCL_LAZY_SYMBOLS_1_1(SYM)    \
  SYM(clCreateEventFromGLsyncKHR)       \
  SYM(clCreateSubBuffer)                \
  SYM(clCreateUserEvent)                \
  SYM(clEnqueueCopyBufferRect)          \
  SYM(clEnqueueReadBufferRect)          \
  SYM(clEnqueueWriteBufferRect)         \
  SYM(clSetEventCallback)               \
  SYM(clSetMemObjectDestructorCallback) \
  SYM(clSetUserEventStatus)

#define OPENCL_LAZY_SYMBOLS_1_2(SYM)            \
  SYM(clCompileProgram)                         \
  SYM(clCreateFromGLTexture)                    \
  SYM(clCreateImage)                            \
  SYM(clCreateProgramWithBuiltInKernels)        \
  SYM(clCreateSubDevices)                       \
  SYM(clEnqueueBarrierWithWaitList)             \
  SYM(clEnqueueFillBuffer)                      \
  SYM(clEnqueueFillImage)                       \
  SYM(clEnqueueMarkerWithWaitList)              \
  SYM(clEnqueueMigrateMemObjects)               \
  SYM(clGetExtensionFunctionAddressForPlatform) \
  SYM(clGetKernelArgInfo)                       \
  SYM(clLinkProgram)                            \
  SYM(clReleaseDevice)                          \
  SYM(clRetainDevice)                           \
  SYM(clUnloadPlatformCompiler)

#define OPENCL_LAZY_SYMBOLS_2_0(SYM)      \
  SYM(clCreateCommandQueueWithProperties) \
  SYM(clCreatePipe)                       \
  SYM(clCreateSamplerWithProperties)      \
  SYM(clEnqueueSVMFree)                   \
  SYM(clEnqueueSVMMap)                    \
  SYM(clEnqueueSVMMemcpy)                 \
  SYM(clEnqueueSVMMemFill)                \
  SYM(clEnqueueSVMUnmap)                  \
  SYM(clGetPipeInfo)                      \
  SYM(clSetKernelArgSVMPointer)           \
  SYM(clSetKernelExecInfo)                \
  SYM(clSVMAlloc)                         \
  SYM(clSVMFree)

#define OPENCL_LAZY_SYMBOLS_2_1(SYM) \
  SYM(clCloneKernel)                 \
  SYM(clCreateProgramWithIL)         \
  SYM(clEnqueueSVMMigrateMem)        \
  SYM(clGetDeviceAndHostTimer)       \
  SYM(clGetHostTimer)                \
  SYM(clGetKernelSubGroupInfo)       \
  SYM(clSetDefaultDeviceCommandQueue)

#define OPENCL_LAZY_SYMBOLS_2_2(SYM) SYM(clSetProgramSpecializationConstant)

#define OPENCL_LAZY_SYMBOLS_3_0(SYM) \
  SYM(clCreateBufferWithProperties)  \
  SYM(clCreateImageWithProperties)   \
  SYM(clSetContextDestructorCallback)

// clang-format off
// ( X="1_0 1_1 1_2 2_0 2_1 2_2 3_0"; for v in $X; do echo "#ifdef CL_VERSION_$v"; echo "#define OPENCL_LAZY_SYMBOLS_${v}_COND(SYM) OPENCL_LAZY_SYMBOLS_${v}(SYM)"; echo "#else"; echo "#define OPENCL_LAZY_SYMBOLS_${v}_COND(SYM)"; echo "#endif"; echo; done; echo "#define OPENCL_LAZY_SYMBOLS(SYM) \\"; for v in $X; do echo "OPENCL_LAZY_SYMBOLS_${v}_COND(SYM) \\"; done; echo ) | clang-format
// clang-format on

#ifdef CL_VERSION_1_0
#define OPENCL_LAZY_SYMBOLS_1_0_COND(SYM) OPENCL_LAZY_SYMBOLS_1_0(SYM)
#else
#define OPENCL_LAZY_SYMBOLS_1_0_COND(SYM)
#endif

#ifdef CL_VERSION_1_1
#define OPENCL_LAZY_SYMBOLS_1_1_COND(SYM) OPENCL_LAZY_SYMBOLS_1_1(SYM)
#else
#define OPENCL_LAZY_SYMBOLS_1_1_COND(SYM)
#endif

#ifdef CL_VERSION_1_2
#define OPENCL_LAZY_SYMBOLS_1_2_COND(SYM) OPENCL_LAZY_SYMBOLS_1_2(SYM)
#else
#define OPENCL_LAZY_SYMBOLS_1_2_COND(SYM)
#endif

#ifdef CL_VERSION_2_0
#define OPENCL_LAZY_SYMBOLS_2_0_COND(SYM) OPENCL_LAZY_SYMBOLS_2_0(SYM)
#else
#define OPENCL_LAZY_SYMBOLS_2_0_COND(SYM)
#endif

#ifdef CL_VERSION_2_1
#define OPENCL_LAZY_SYMBOLS_2_1_COND(SYM) OPENCL_LAZY_SYMBOLS_2_1(SYM)
#else
#define OPENCL_LAZY_SYMBOLS_2_1_COND(SYM)
#endif

#ifdef CL_VERSION_2_2
#define OPENCL_LAZY_SYMBOLS_2_2_COND(SYM) OPENCL_LAZY_SYMBOLS_2_2(SYM)
#else
#define OPENCL_LAZY_SYMBOLS_2_2_COND(SYM)
#endif

#ifdef CL_VERSION_3_0
#define OPENCL_LAZY_SYMBOLS_3_0_COND(SYM) OPENCL_LAZY_SYMBOLS_3_0(SYM)
#else
#define OPENCL_LAZY_SYMBOLS_3_0_COND(SYM)
#endif

#define OPENCL_LAZY_SYMBOLS(SYM)    \
  OPENCL_LAZY_SYMBOLS_1_0_COND(SYM) \
  OPENCL_LAZY_SYMBOLS_1_1_COND(SYM) \
  OPENCL_LAZY_SYMBOLS_1_2_COND(SYM) \
  OPENCL_LAZY_SYMBOLS_2_0_COND(SYM) \
  OPENCL_LAZY_SYMBOLS_2_1_COND(SYM) \
  OPENCL_LAZY_SYMBOLS_2_2_COND(SYM) \
  OPENCL_LAZY_SYMBOLS_3_0_COND(SYM)

// clang-format off
// sed 's/::\(cl[A-Z][a-zA-Z_0-9]*\)/OPENCL_LAZY(\1)/g;s/\([^0-9a-z_(]\)\(cl[a-zA-Z0-9_]*\)(\([^)]\)/\1OPENCL_LAZY(\2)(\3/g;s/&OPENCL_LAZY(/OPENCL_LAZY(/g'
// clang-format on

#ifndef OPENCL_LAZY_EXPORT
#define OPENCL_LAZY_EXPORT
#endif

#define OPENCL_LAZY_DECLARE(n) OPENCL_LAZY_EXPORT decltype(::n)* n();

namespace cl {
namespace OpenCLLibLazy {
OPENCL_LAZY_SYMBOLS(OPENCL_LAZY_DECLARE)
}
}  // namespace cl

#define OPENCL_LAZY(n) (::cl::OpenCLLibLazy::n())

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif  // !OPENCL_CL_LAZY_HPP_DEFINED
