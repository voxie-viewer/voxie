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

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieBackend/lib/CL/cl-patched.hpp>

#include <QtCore/QDebug>
#include <QtCore/QException>
#include <QtCore/QRectF>
#include <QtCore/QVector>

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>

#ifdef _MSC_VER  // define __func__ and NOEXCEPT for msvc
#ifndef __func__
#define __func__ __FUNCTION__
#endif
#ifndef EXCEPT
#define EXCEPT
#endif
#ifndef NOEXCEPT
#define NOEXCEPT
#endif
#else
#define EXCEPT
#define NOEXCEPT noexcept
#endif

namespace vx {
namespace opencl {

/**
 * Exception class for throwing openCL errors.
 * All methods in the vx::opencl namespace that make calls to the cl api
 * that may fail will throw CLException in that case. The Exception wraps the
 * cl error code produced by the cl api call. See cl.h for possible error codes.
 * @brief Exception class for throwing openCL errors.
 */
class VOXIEBACKEND_EXPORT CLException : public QException {
 private:
  cl_int errorCode;
  QString message;
  std::string messageLong;

 public:
  /**
   * @brief CLException constructor
   * @param errCode cl error code
   * @param msg error message
   */
  CLException(cl_int errCode, const QString& msg = "");

  /**
   * @return error message
   */
  const QString& getMessage() const { return this->message; }

  /**
   * @return cl error code (see cl.h)
   */
  cl_int getErrorCode() const { return this->errorCode; }

  /**
   * @return clone of this exception
   */
  CLException* clone() const override { return new CLException(*this); }

  /**
   * @brief throws this exception
   */
  void raise() const override { throw *this; }

  const char* what() const NOEXCEPT override;
};

/** qdebug streaming operator for CLException */
VOXIEBACKEND_EXPORT QDebug operator<<(QDebug dbg, const CLException& ex);

#define raiseCLException(errorCode, message)          \
  do {                                                \
    CLException theException((errorCode), (message)); \
    theException.raise();                             \
  } while (0)
#define file_func_line()                                            \
  (QString(__FILE__) + QString(" Function: ") + QString(__func__) + \
   QString(" Line: ") + QString::number(__LINE__))

// ----- ----- ----- ----- ----- ----- -----

/**
 * @return available platforms
 */
VOXIEBACKEND_EXPORT QVector<cl::Platform> getPlatforms() EXCEPT;

/**
 * @throws CLException if type is not a valid type (CL_INVALID_DEVICE_TYPE)
 * or platform is invalid (CL_INVALID_PLATFORM)
 * @return available devices of given type on given platform
 */
VOXIEBACKEND_EXPORT QVector<cl::Device> getDevices(
    const cl::Platform& platform,
    cl_device_type type = CL_DEVICE_TYPE_ALL) EXCEPT;

/**
 * @throws CLException if type is not a valid type (CL_INVALID_DEVICE_TYPE)
 * @return available devices of given type on all available platforms
 */
VOXIEBACKEND_EXPORT QVector<cl::Device> getDevices(
    cl_device_type type = CL_DEVICE_TYPE_ALL) EXCEPT;

/**
 * @throws CLException if platform is invalid (CL_INVALID_PLATFORM)
 * @return name of platform
 */
VOXIEBACKEND_EXPORT QString getName(const cl::Platform& platform) EXCEPT;

/**
 * @throws CLException if platform is invalid (CL_INVALID_PLATFORM)
 * @return name of platforms vendor
 */
VOXIEBACKEND_EXPORT QString getVendor(const cl::Platform& platform) EXCEPT;

/**
 * @return platforms matching given name
 */
VOXIEBACKEND_EXPORT QVector<cl::Platform> getPlatformByName(
    const QString& platformName) NOEXCEPT;

/**
 * @return devices on given platform that match given name
 */
VOXIEBACKEND_EXPORT QVector<cl::Device> getDeviceByName(
    const cl::Platform& platform, const QString& deviceName) NOEXCEPT;

// ---

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return name of device
 */
VOXIEBACKEND_EXPORT QString getName(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return platform of device
 */
VOXIEBACKEND_EXPORT cl::Platform getPlatform(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return device's type
 */
VOXIEBACKEND_EXPORT cl_device_type getType(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE),
 * device is currently not available (CL_DEVICE_NOT_AVAILABLE) or
 * if there is a failure to allocate resources required by the OpenCL
 * implementation on the host (CL_OUT_OF_HOST_MEMORY)
 * @return context consisting of given device
 */
VOXIEBACKEND_EXPORT cl::Context createContext(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if a device is invalid (CL_INVALID_DEVICE),
 * a found device is currently not available (CL_DEVICE_NOT_AVAILABLE) or
 * if there is a failure to allocate resources required by the OpenCL
 * implementation on the host (CL_OUT_OF_HOST_MEMORY)
 * @return context consisting of all gpu and cpu devices found on the platform.
 * If no GPU or CPU devices are present on the platform, all other devices are
 * used.
 */
VOXIEBACKEND_EXPORT cl::Context createContext(
    const cl::Platform& platform,
    cl_device_type deviceType = CL_DEVICE_TYPE_ALL) EXCEPT;

/**
 * @throws CLException if devices contains an invalid device
 * (CL_INVALID_DEVICE), if devices is empty (CL_INVALID_VALUE), a device is
 * currently not available (CL_DEVICE_NOT_AVAILABLE) or if there is a failure to
 * allocate resources required by the OpenCL implementation on the host
 * (CL_OUT_OF_HOST_MEMORY)
 * @returns context consisting of all devices in given vector
 */
VOXIEBACKEND_EXPORT cl::Context createContext(
    QVector<cl::Device> devices) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return global memory size of device
 */
VOXIEBACKEND_EXPORT size_t getGlobalMemorySize(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return clock frequency of device's compute units
 */
VOXIEBACKEND_EXPORT size_t getClockFrequency(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return number of device's compute units
 */
VOXIEBACKEND_EXPORT size_t getNumComputeUnits(const cl::Device& device) EXCEPT;

// ---

/**
 * @throws CLException if context is invalid (CL_INVALID_CONTEXT)
 * @return devices associated with given context
 */
VOXIEBACKEND_EXPORT QVector<cl::Device> getDevices(
    const cl::Context& context,
    cl_device_type types = CL_DEVICE_TYPE_ALL) EXCEPT;

/**
 * @throws CLException (CL_INVALID_CONTEXT, CL_INVALID_DEVICE) or
 * if values specified in properties are valid but not supported
 * by the device (CL_INVALID_QUEUE_PROPERTIES),
 * or if there is a failure to allocate resources required by the
 * OpenCL implementation on the device (CL_OUT_OF_RESOURCES),
 * or if there is a failure to allocate resources required by the
 * OpenCL implementation on the host (CL_OUT_OF_HOST_MEMORY)
 * @return command queue for given context
 */
VOXIEBACKEND_EXPORT cl::CommandQueue createCommandQueue(
    const cl::Context& context, const cl::Device& device,
    cl_command_queue_properties properties = 0) EXCEPT;

/**
 * @return name of error code
 */
VOXIEBACKEND_EXPORT QString clErrorToString(cl_int err) NOEXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE) or if infoname
 * is no supported cl_device_info (CL_INVALID_VALUE). Will not throw if err
 * param is provided.
 * @param T type of awaited return type (see opencl spec for return values for
 * infonames)
 * @param device
 * @param infoname (see opencl spec for available info names)
 * @param err error variable set to error code or CL_SUCCESS.
 * when provided, method will not throw
 * @return desired info value for given device
 */
template <typename T>
T deviceInfo(const cl::Device& device, cl_device_info infoname,
             cl_int* err = nullptr) EXCEPT {
  T val;
  cl_int error = device.getInfo<T>(infoname, &val);
  if (err != nullptr) {
    *err = error;
  } else if (error) {
    raiseCLException(error, file_func_line());
  }
  return val;
}

inline cl_float3 qVec3_To_clVec3f(const QVector3D& vec) {
  cl_float3 clVec = {{(cl_float)vec.x(), (cl_float)vec.y(), (cl_float)vec.z(),
                      (cl_float)0.0f}};  // cl_float3 is cl_float4
  return clVec;
}
inline cl_double4 qVec3_To_clVec3d(const QVector3D& vec) {
  cl_double4 clVec = {{(cl_double)vec.x(), (cl_double)vec.y(),
                       (cl_double)vec.z(),
                       (cl_double)0.0}};  // cl_float3 is cl_float4
  return clVec;
}

inline cl_float2 qVec2_To_clVec2f(const QVector2D& vec) {
  cl_float2 clVec = {{(cl_float)vec.x(), (cl_float)vec.y()}};
  return clVec;
}

inline cl_int2 qPoint_To_clVec2i(const QPoint& point) {
  cl_int2 clVec = {{(cl_int)point.x(), (cl_int)point.y()}};
  return clVec;
}

inline cl_float4 qRectF_To_clVec4f(const QRectF& rect) {
  cl_float4 clVec = {{(cl_float)rect.left(), (cl_float)rect.top(),
                      (cl_float)rect.width(), (cl_float)rect.height()}};
  return clVec;
}

inline cl_double4 qRectF_To_clVec4d(const QRectF& rect) {
  cl_double4 clVec = {{(cl_double)rect.left(), (cl_double)rect.top(),
                       (cl_double)rect.width(), (cl_double)rect.height()}};
  return clVec;
}

inline cl_float4 clVec4f(float x, float y, float z, float w) {
  cl_float4 clVec = {{(cl_float)x, (cl_float)y, (cl_float)z, (cl_float)w}};
  return clVec;
}
inline cl_double4 clVec4d(double x, double y, double z, double w) {
  cl_double4 clVec = {{(cl_double)x, (cl_double)y, (cl_double)z, (cl_double)w}};
  return clVec;
}

}  // namespace opencl
}  // namespace vx
