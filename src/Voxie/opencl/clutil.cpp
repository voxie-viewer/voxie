#include "clutil.hpp"

#include <QtCore/QScopedPointer>

using namespace voxie;
using namespace voxie::opencl;

QVector<cl::Platform>
voxie::opencl::getPlatforms() EXCEPT
{
    std::vector<cl::Platform> platformsVec;
    cl_int error = cl::Platform::get(&platformsVec);
    if(error){
        // will produce error if no platforms are found. In this case we will
        // return an empty vector
        // raiseCLException(error, file_func_line());
        Q_UNUSED(error);
    }
    return QVector<cl::Platform>::fromStdVector(platformsVec);
}


QString
voxie::opencl::getName(const cl::Platform& platform) EXCEPT
{
    std::string infostr;
    cl_int error = platform.getInfo(CL_PLATFORM_NAME, &infostr);
	if(error){
		raiseCLException(error, file_func_line());
	}
	return QString::fromStdString(infostr);
}


QString
voxie::opencl::getVendor(const cl::Platform& platform) EXCEPT
{
	std::string infostr;
	cl_int error = platform.getInfo(CL_PLATFORM_VENDOR, &infostr);
	if(error){
		raiseCLException(error, file_func_line());
	}
	return QString::fromStdString(infostr);
}


QVector<cl::Device>
voxie::opencl::getDevices(const cl::Platform& platform, cl_device_type type) EXCEPT
{
	std::vector<cl::Device> devices;
	cl_int error = platform.getDevices(type, &devices);
	if(error && error != CL_DEVICE_NOT_FOUND){
		raiseCLException(error, file_func_line());
	}
	return QVector<cl::Device>::fromStdVector(devices);
}


QVector<cl::Platform>
voxie::opencl::getPlatformByName(const QString& platformName) NOEXCEPT
{
	QVector<cl::Platform> results;
	try{
		for(const cl::Platform& platform : getPlatforms()){
			if(getName(platform) == platformName)
				results.append(platform);
		}
	} catch(CLException& ex){
		qWarning() << ex;
	}

	return results;
}


QString
voxie::opencl::getName(const cl::Device& device) EXCEPT
{
    return QString::fromStdString(deviceInfo<std::string>(device, CL_DEVICE_NAME));
}


cl::Platform
voxie::opencl::getPlatform(const cl::Device& device) EXCEPT
{
    return cl::Platform(deviceInfo<cl_platform_id>(device, CL_DEVICE_PLATFORM));
}


cl_device_type
voxie::opencl::getType(const cl::Device& device) EXCEPT
{
    return deviceInfo<cl_device_type>(device, CL_DEVICE_TYPE);
}

cl::Context
voxie::opencl::createContext(const cl::Device& device) EXCEPT
{
	std::vector<cl::Device> devices;
	devices.push_back(device);
	cl_int error;
	cl::Context context(devices, nullptr, nullptr, nullptr, &error);
	if(error) {
		raiseCLException(error, file_func_line());
	}
	return context;
}


cl::Context
voxie::opencl::createContext(const cl::Platform& platform, cl_device_type deviceType) EXCEPT
{
	std::vector<cl::Device> devices = getDevices(platform, deviceType).toStdVector();
	if(devices.empty()){
		devices = getDevices(platform, CL_DEVICE_TYPE_ALL).toStdVector(); // all types
	}
	cl_int error;
	cl::Context context(devices, nullptr, nullptr, nullptr, &error);
	if(error) {
		raiseCLException(error, file_func_line());
	}
	return context;
}

cl::Context
voxie::opencl::createContext(QVector<cl::Device> devices) EXCEPT
{
    std::vector<cl::Device> devices_ = devices.toStdVector();
    cl_int error;
    cl::Context context(devices_, nullptr, nullptr, nullptr, &error);
    if(error) {
        raiseCLException(error, file_func_line());
    }
    return context;
}


size_t
voxie::opencl::getGlobalMemorySize(const cl::Device& device) EXCEPT
{
    return deviceInfo<cl_ulong>(device, CL_DEVICE_GLOBAL_MEM_SIZE);
}


size_t
voxie::opencl::getClockFrequency(const cl::Device& device) EXCEPT
{
    return deviceInfo<cl_uint>(device, CL_DEVICE_MAX_CLOCK_FREQUENCY);
}


size_t
voxie::opencl::getNumComputeUnits(const cl::Device& device) EXCEPT
{
    return deviceInfo<cl_uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS);
}


QVector<cl::Device>
voxie::opencl::getDeviceByName(const cl::Platform& platform, const QString& deviceName) NOEXCEPT
{
	QVector<cl::Device> results;
	try{
		for(const cl::Device& device: getDevices(platform)){
			if(getName(device) == deviceName)
				results.append(device);
		}
	} catch(CLException& ex) {
		qWarning() << ex;
	}
	return results;
}


QVector<cl::Device>
voxie::opencl::getDevices(const cl::Context& context, cl_device_type types) EXCEPT
{
	std::vector<cl::Device> devices;
	cl_int error = context.getInfo(CL_CONTEXT_DEVICES, &devices);
	if(error){
		raiseCLException(error, file_func_line());
	}
	QVector<cl::Device> toReturn;
	for(cl::Device device : devices){
		if( (getType(device) & types) != 0){
			toReturn.append(device);
		}
	}
	return toReturn;
}


QVector<cl::Device>
voxie::opencl::getDevices(cl_device_type type) EXCEPT
{
	QVector<cl::Device> devices;
	for(const cl::Platform& platform: voxie::opencl::getPlatforms()){
		devices += getDevices(platform, type);
	}
	return devices;
}


cl::CommandQueue
voxie::opencl::createCommandQueue(const cl::Context& context, const cl::Device& device, cl_command_queue_properties properties) EXCEPT
{
	cl_int error;
	cl::CommandQueue commandQueue(context, device, properties, &error);
	if(error){
		raiseCLException(error, file_func_line());
	}
	return commandQueue;
}


QString
voxie::opencl::clErrorToString(cl_int err) NOEXCEPT
{
	switch (err) {
		case 0:
			return "CL_SUCCESS";
		case -1:
			return "CL_DEVICE_NOT_FOUND";
		case -2:
			return "CL_DEVICE_NOT_AVAILABLE";
		case -3:
			return "CL_COMPILER_NOT_AVAILABLE";
		case -4:
			return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
		case -5:
			return "CL_OUT_OF_RESOURCES";
		case -6:
			return "CL_OUT_OF_HOST_MEMORY";
		case -7:
			return "CL_PROFILING_INFO_NOT_AVAILABLE";
		case -8:
			return "CL_MEM_COPY_OVERLAP";
		case -9:
			return "CL_IMAGE_FORMAT_MISMATCH";
		case -10:
			return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
		case -11:
			return "CL_BUILD_PROGRAM_FAILURE";
		case -12:
			return "CL_MAP_FAILURE";
		case -13:
			return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
		case -14:
			return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
		case -15:
			return "CL_COMPILE_PROGRAM_FAILURE";
		case -16:
			return "CL_LINKER_NOT_AVAILABLE";
		case -17:
			return "CL_LINK_PROGRAM_FAILURE";
		case -18:
			return "CL_DEVICE_PARTITION_FAILED";
		case -19:
			return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
			//
		case -30:
			return "CL_INVALID_VALUE";
		case -31:
			return "CL_INVALID_DEVICE_TYPE";
		case -32:
			return "CL_INVALID_PLATFORM";
		case -33:
			return "CL_INVALID_DEVICE";
		case -34:
			return "CL_INVALID_CONTEXT";
		case -35:
			return "CL_INVALID_QUEUE_PROPERTIES";
		case -36:
			return "CL_INVALID_COMMAND_QUEUE";
		case -37:
			return "CL_INVALID_HOST_PTR";
		case -38:
			return "CL_INVALID_MEM_OBJECT";
		case -39:
			return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
		case -40:
			return "CL_INVALID_IMAGE_SIZE";
		case -41:
			return "CL_INVALID_SAMPLER";
		case -42:
			return "CL_INVALID_BINARY";
		case -43:
			return "CL_INVALID_BUILD_OPTIONS";
		case -44:
			return "CL_INVALID_PROGRAM";
		case -45:
			return "CL_INVALID_PROGRAM_EXECUTABLE";
		case -46:
			return "CL_INVALID_KERNEL_NAME";
		case -47:
			return "CL_INVALID_KERNEL_DEFINITION";
		case -48:
			return "CL_INVALID_KERNEL";
		case -49:
			return "CL_INVALID_ARG_INDEX";
		case -50:
			return "CL_INVALID_ARG_VALUE";
		case -51:
			return "CL_INVALID_ARG_SIZE";
		case -52:
			return "CL_INVALID_KERNEL_ARGS";
		case -53:
			return "CL_INVALID_WORK_DIMENSION";
		case -54:
			return "CL_INVALID_WORK_GROUP_SIZE";
		case -55:
			return "CL_INVALID_WORK_ITEM_SIZE";
		case -56:
			return "CL_INVALID_GLOBAL_OFFSET";
		case -57:
			return "CL_INVALID_EVENT_WAIT_LIST";
		case -58:
			return "CL_INVALID_EVENT";
		case -59:
			return "CL_INVALID_OPERATION";
		case -60:
			return "CL_INVALID_GL_OBJECT";
		case -61:
			return "CL_INVALID_BUFFER_SIZE";
		case -62:
			return "CL_INVALID_MIP_LEVEL";
		case -63:
			return "CL_INVALID_GLOBAL_WORK_SIZE";
		case -64:
			return "CL_INVALID_PROPERTY";
		case -65:
			return "CL_INVALID_IMAGE_DESCRIPTOR";
		case -66:
			return "CL_INVALID_COMPILER_OPTIONS";
		case -67:
			return "CL_INVALID_LINKER_OPTIONS";
		case -68:
			return "CL_INVALID_DEVICE_PARTITION_COUNT";
		default:
			return "errcode:" + QString::number(err);
		}
}


const char*
voxie::opencl::CLException::what() const NOEXCEPT
{
    return (clErrorToString(getErrorCode()) + " - " + getMessage()).toStdString().c_str();
}


QDebug
voxie::opencl::operator<<(QDebug dbg, const CLException& ex)
{
	dbg.nospace() << "CLException: " << clErrorToString(ex.getErrorCode()) << " [" << ex.getMessage() << "]";
	return dbg;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
