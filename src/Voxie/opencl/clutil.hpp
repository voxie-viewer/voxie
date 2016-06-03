#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/lib/CL/cl.hpp>

#include <QtCore/QDebug>
#include <QtCore/QException>
#include <QtCore/QRectF>
#include <QtCore/QVector>

#include <QtGui/QVector2D>
#include <QtGui/QVector3D>

#ifdef _MSC_VER // define __func__ and NOEXCEPT for msvc
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



namespace voxie {
namespace opencl {

/**
 * Exception class for throwing openCL errors.
 * All methods in the voxie::opencl namespace that make calls to the cl api
 * that may fail will throw CLException in that case. The Exception wraps the
 * cl error code produced by the cl api call. See cl.h for possible error codes.
 * @brief Exception class for throwing openCL errors.
 */
class VOXIECORESHARED_EXPORT CLException: public QException
{
private:
    cl_int errorCode;
    QString message;
public:
    /**
     * @brief CLException constructor
     * @param errCode cl error code
     * @param msg error message
     */
    CLException(cl_int errCode, const QString& msg = "")
        : errorCode(errCode), message(msg){}

    /**
     * @return error message
     */
    const QString& getMessage() const
        {return this->message;}

    /**
     * @return cl error code (see cl.h)
     */
    cl_int getErrorCode() const
        {return this->errorCode;}

    /**
     * @return clone of this exception
     */
    CLException* clone() const { return new CLException(*this); }

    /**
     * @brief throws this exception
     */
    void raise() const { throw *this; }

    const char* what() const NOEXCEPT;
};

/** qdebug streaming operator for CLException */
VOXIECORESHARED_EXPORT QDebug operator<<(QDebug dbg, const CLException& ex);

#define raiseCLException(errorCode, message) do{CLException theException((errorCode), (message)); theException.raise();}while(0)
#define file_func_line() (QString(__FILE__) + QString(" Function: ") + QString(__func__) + QString( " Line: ") + QString::number(__LINE__))


// ----- ----- ----- ----- ----- ----- -----

/**
 * @return available platforms
 */
VOXIECORESHARED_EXPORT QVector<cl::Platform> getPlatforms() EXCEPT;

/**
 * @throws CLException if type is not a valid type (CL_INVALID_DEVICE_TYPE)
 * or platform is invalid (CL_INVALID_PLATFORM)
 * @return available devices of given type on given platform
 */
VOXIECORESHARED_EXPORT QVector<cl::Device> getDevices(const cl::Platform& platform, cl_device_type type = CL_DEVICE_TYPE_ALL) EXCEPT;

/**
 * @throws CLException if type is not a valid type (CL_INVALID_DEVICE_TYPE)
 * @return available devices of given type on all available platforms
 */
VOXIECORESHARED_EXPORT QVector<cl::Device> getDevices(cl_device_type type = CL_DEVICE_TYPE_ALL) EXCEPT;

/**
 * @throws CLException if platform is invalid (CL_INVALID_PLATFORM)
 * @return name of platform
 */
VOXIECORESHARED_EXPORT QString getName(const cl::Platform& platform) EXCEPT;

/**
 * @throws CLException if platform is invalid (CL_INVALID_PLATFORM)
 * @return name of platforms vendor
 */
VOXIECORESHARED_EXPORT QString getVendor(const cl::Platform& platform) EXCEPT;

/**
 * @return platforms matching given name
 */
VOXIECORESHARED_EXPORT QVector<cl::Platform> getPlatformByName(const QString& platformName) NOEXCEPT;

/**
 * @return devices on given platform that match given name
 */
VOXIECORESHARED_EXPORT QVector<cl::Device> getDeviceByName(const cl::Platform& platform, const QString& deviceName) NOEXCEPT;


// ---

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return name of device
 */
VOXIECORESHARED_EXPORT QString getName(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return platform of device
 */
VOXIECORESHARED_EXPORT cl::Platform getPlatform(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return device's type
 */
VOXIECORESHARED_EXPORT cl_device_type getType(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE),
 * device is currently not available (CL_DEVICE_NOT_AVAILABLE) or
 * if there is a failure to allocate resources required by the OpenCL
 * implementation on the host (CL_OUT_OF_HOST_MEMORY)
 * @return context consisting of given device
 */
VOXIECORESHARED_EXPORT cl::Context createContext(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if a device is invalid (CL_INVALID_DEVICE),
 * a found device is currently not available (CL_DEVICE_NOT_AVAILABLE) or
 * if there is a failure to allocate resources required by the OpenCL
 * implementation on the host (CL_OUT_OF_HOST_MEMORY)
 * @return context consisting of all gpu and cpu devices found on the platform.
 * If no GPU or CPU devices are present on the platform, all other devices are used.
 */
VOXIECORESHARED_EXPORT cl::Context createContext(const cl::Platform& platform, cl_device_type deviceType = CL_DEVICE_TYPE_ALL) EXCEPT;

/**
  * @throws CLException if devices contains an invalid device (CL_INVALID_DEVICE),
  * if devices is empty (CL_INVALID_VALUE), a device is currently not available
  * (CL_DEVICE_NOT_AVAILABLE) or if there is a failure to allocate resources
  * required by the OpenCL implementation on the host (CL_OUT_OF_HOST_MEMORY)
  * @returns context consisting of all devices in given vector
  */
VOXIECORESHARED_EXPORT cl::Context createContext(QVector<cl::Device> devices) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return global memory size of device
 */
VOXIECORESHARED_EXPORT size_t getGlobalMemorySize(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return clock frequency of device's compute units
 */
VOXIECORESHARED_EXPORT size_t getClockFrequency(const cl::Device& device) EXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE)
 * @return number of device's compute units
 */
VOXIECORESHARED_EXPORT size_t getNumComputeUnits(const cl::Device& device) EXCEPT;

// ---

/**
 * @throws CLException if context is invalid (CL_INVALID_CONTEXT)
 * @return devices associated with given context
 */
VOXIECORESHARED_EXPORT QVector<cl::Device> getDevices(const cl::Context& context, cl_device_type types = CL_DEVICE_TYPE_ALL) EXCEPT;

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
VOXIECORESHARED_EXPORT cl::CommandQueue createCommandQueue(const cl::Context& context, const cl::Device &device, cl_command_queue_properties properties = 0) EXCEPT;

/**
 * @return name of error code
 */
VOXIECORESHARED_EXPORT QString clErrorToString(cl_int err) NOEXCEPT;

/**
 * @throws CLException if device is invalid (CL_INVALID_DEVICE) or if infoname
 * is no supported cl_device_info (CL_INVALID_VALUE). Will not throw if err param
 * is provided.
 * @param T type of awaited return type (see opencl spec for return values for infonames)
 * @param device
 * @param infoname (see opencl spec for available info names)
 * @param err error variable set to error code or CL_SUCCESS.
 * when provided, method will not throw
 * @return desired info value for given device
 */
template <typename T>
T deviceInfo(const cl::Device& device, cl_device_info infoname, cl_int* err = nullptr) EXCEPT
{
    T val;
    cl_int error = device.getInfo<T>(infoname, &val);
    if(err != nullptr){
        *err = error;
    } else if(error){
        raiseCLException(error, file_func_line());
    }
    return val;
}

inline cl_float3 qVec3_To_clVec3f(const QVector3D& vec)
{
    cl_float3 clVec = {{ (cl_float)vec.x(), (cl_float)vec.y(), (cl_float)vec.z(), (cl_float)0.0f }}; // cl_float3 is cl_float4
    return clVec;
}

inline cl_float2 qVec2_To_clVec2f(const QVector2D& vec)
{
    cl_float2 clVec = {{ (cl_float)vec.x(), (cl_float)vec.y()}};
    return clVec;
}

inline cl_int2 qPoint_To_clVec2i(const QPoint& point)
{
    cl_int2 clVec = {{(cl_int)point.x(), (cl_int)point.y()}};
    return clVec;
}

inline cl_float4 qRectF_To_clVec4f(const QRectF& rect)
{
    cl_float4 clVec = {{ (cl_float)rect.left(), (cl_float)rect.top(), (cl_float)rect.width(), (cl_float)rect.height() }};
    return clVec;
}

inline cl_float4 clVec4f(float x, float y, float z, float w)
{
    cl_float4 clVec = {{ (cl_float)x, (cl_float)y, (cl_float)z, (cl_float)w }};
    return clVec;
}


}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
