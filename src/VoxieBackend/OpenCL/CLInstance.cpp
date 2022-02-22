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

#include "CLInstance.hpp"

#include <QtCore/QDebug>

#include <VoxieClient/QtUtil.hpp>

using namespace vx::opencl;
using namespace vx;

// static member initialization
bool CLInstance::initialized = false;
CLInstance* CLInstance::defaultInstance = nullptr;

CLInstance* CLInstance::createInstance(const cl::Context& context) NOEXCEPT {
  CLInstance* instance = nullptr;
  try {
    // test context
    QVector<cl::Device> devices = vx::opencl::getDevices(context);
    if (devices.isEmpty()) {
      qWarning()
          << "CLInstance could not be created properly, no devices in context";
    }
  } catch (CLException& ex) {
    qWarning() << "CLInstance could not be created properly" << ex;
  }
  instance = new CLInstance(context);
  return instance;
}

CLInstance* CLInstance::createInstance(const cl::Device& device) NOEXCEPT {
  CLInstance* instance = nullptr;
  cl::Context ctx;
  try {
    ctx = createContext(device);
  } catch (CLException& ex) {
    qWarning() << "CLInstance could not be created properly" << ex;
  }
  instance = new CLInstance(ctx);
  return instance;
}

CLInstance* CLInstance::createInstance(const cl::Platform& platform) NOEXCEPT {
  CLInstance* instance = nullptr;
  cl::Context ctx;
  try {
    ctx = createContext(platform, CL_DEVICE_TYPE_CPU | CL_DEVICE_TYPE_GPU);
  } catch (CLException& ex) {
    qWarning() << "CLInstance could net be created properly" << ex;
  }
  instance = new CLInstance(ctx);
  return instance;
}

void CLInstance::initialize(CLInstance* defaultInst) EXCEPT {
  if (initialized) {
    qWarning() << "attempt to initialize CLInstances again failed. Can only "
                  "initialize once";
    return;
  }

  // initialize with invalid ctx in case of later failure
  CLInstance* invalidInstance = new CLInstance();  // invalid ctx
  CLInstance::defaultInstance = invalidInstance;

  if (defaultInst == nullptr) {
    // automatic initialization, find platform with most gpus
    QVector<cl::Platform> platforms = getPlatforms();
    if (platforms.isEmpty()) {
      qInfo() << "No OpenCL platform found.";
      return;
    }
    int winner = 0;
    int maxNumGPU = 0;
    int maxNumCPU = 0;
    for (int i = 0; i < platforms.length(); i++) {
      cl::Platform platform = platforms[i];
      int numGPU =
          vx::opencl::getDevices(platform, CL_DEVICE_TYPE_GPU).length();
      int numCPU =
          vx::opencl::getDevices(platform, CL_DEVICE_TYPE_CPU).length();
      if (numGPU > maxNumGPU || (numGPU == maxNumGPU && numCPU > maxNumCPU)) {
        winner = i;
        maxNumGPU = numGPU;
        maxNumCPU = numCPU;
      }
    }
    CLInstance::defaultInstance = CLInstance::createInstance(platforms[winner]);
  } else {
    CLInstance::defaultInstance = defaultInst;
  }

  initialized = true;
  delete invalidInstance;  // delete placeholder instance
}

CLInstance::CLInstance(cl::Context context) : context(context) {
  QMutexLocker locker(&this->mutex);

  // init commandQueues
  try {
    for (cl::Device device : this->getDevices()) {
      cl::CommandQueue queue = this->createCommandQueue(device);
      this->commandQueues.insert(device(), queue);
    }
  } catch (CLException& ex) {
    qWarning() << ex;
  }
}

cl::Platform CLInstance::getPlatform() const EXCEPT {
  auto devices = this->getDevices();
  return opencl::getPlatform(devices[0]);
}

int CLInstance::getNumDevices(cl_device_type type) const EXCEPT {
  return getDevices(type).length();
}

QVector<cl::Device> CLInstance::getDevices(cl_device_type type) const EXCEPT {
  return vx::opencl::getDevices(this->context, type);
}

cl::Device CLInstance::getDevice(cl_device_type preferredType) const EXCEPT {
  QVector<cl::Device> devices = this->getDevices(preferredType);
  if (devices.isEmpty()) {
    return this->getDevices()[0];  // just return something
  } else {
    return devices[0];
  }
}

cl::CommandQueue CLInstance::getCommandQueue(
    cl_device_type preferredDevice) const EXCEPT {
  return this->getCommandQueue(getDevice(preferredDevice));
}

cl::CommandQueue CLInstance::getCommandQueue(
    const cl::Device& device) const NOEXCEPT {
  QMutexLocker locker(&this->mutex);

  return this->commandQueues.value(device());
}

cl::CommandQueue CLInstance::createCommandQueue(
    cl_device_type preferredDevice) const EXCEPT {
  return this->createCommandQueue(getDevice(preferredDevice));
}

cl::CommandQueue CLInstance::createCommandQueue(
    const cl::Device& device) const EXCEPT {
  return vx::opencl::createCommandQueue(this->context, device);
}

cl::Program CLInstance::createProgram(const QString& source,
                                      const QString& buildOptions,
                                      const QString& id) EXCEPT {
  cl::Program::Sources sources;
  std::string src = source.toStdString();
  sources.push_back(
      std::pair<const char*, ::size_t>(src.c_str(), src.length()));

  cl_int error;
  cl::Program program(this->context, sources, &error);
  if (error) {
    raiseCLException(error, file_func_line() + ": " + id);
  }

  error = program.build(toStdVector(this->getDevices()),
                        buildOptions.toStdString().c_str());
  if (error) {
    if (error == CL_BUILD_PROGRAM_FAILURE) {
      qWarning() << "--- OpenCL Build Log ---"
                 << "\nSource:\n"
                 << source << "\n";
      for (const cl::Device& device : this->getDevices()) {
        std::string log;
        program.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &log);
        qWarning() << "Log for Device " << getName(device) << " :\n"
                   << log.c_str() << "\n";
      }
    }
    raiseCLException(error, file_func_line());
  }
  if (!id.isEmpty()) {
    QMutexLocker locker(&this->mutex);

    if (this->programs.contains(id)) {
      qWarning() << "overwrite existing program with id:" << id;
    }
    this->programs.insert(id, program);
  }

  return program;
}

cl::Program CLInstance::createProgramFromFile(const QString& filename,
                                              const QString& buildOptions,
                                              const QString& id) EXCEPT {
  QFile sourceFile(filename);
  if (!sourceFile.exists()) {
    qWarning() << "file does not exist:" << filename;
    IOException(QString("file does not exist: ") + filename).raise();
  }
  if (!sourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "could not open file:" << filename;
    IOException(QString("could not open file: ") + filename).raise();
  }
  QTextStream stream(&sourceFile);
  QString src(stream.readAll());
  sourceFile.close();

  cl::Program prog = this->createProgram(src, buildOptions, id);
  return prog;
}

bool CLInstance::hasProgramID(const QString& programID) const NOEXCEPT {
  QMutexLocker locker(&this->mutex);

  return this->programs.contains(programID);
}

cl::Program CLInstance::getProgram(const QString& programId) const NOEXCEPT {
  QMutexLocker locker(&this->mutex);

  return programs.value(programId);
}

cl::Kernel CLInstance::getKernel(const QString& programId,
                                 const QString& kernelName) const EXCEPT {
  QMutexLocker locker(&this->mutex);

  if (!this->programs.contains(programId)) {
    qWarning() << "NO PROGRAM PRESENT WITH ID:" << programId;
    return cl::Kernel();
  }
  if (kernelName.isEmpty()) {
    qWarning() << "kernel name is not valid, kernal name is empty!";
    return cl::Kernel();
  }
  cl_int error;
  cl::Kernel kernel(this->programs.value(programId),
                    kernelName.toStdString().c_str(), &error);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  return kernel;
}

void CLInstance::executeKernel(const cl::Kernel& kernel,
                               const cl::NDRange& globalWorkSize,
                               const cl::CommandQueue& commandQueue,
                               bool blocking) EXCEPT {
  cl_int error = commandQueue.enqueueNDRangeKernel(
      kernel, cl::NullRange, globalWorkSize, cl::NullRange);
  if (error) {
    raiseCLException(error,
                     file_func_line() + ": " +
                         kernel.getInfo<CL_KERNEL_FUNCTION_NAME>().c_str());
  }
  if (blocking) {
    error = commandQueue.finish();
    if (error) {
      raiseCLException(error, file_func_line());
    }
  }
}

void CLInstance::executeKernel(const cl::Kernel& kernel,
                               const cl::NDRange& globalWorkSize,
                               cl_device_type preferredDeviceType,
                               bool blocking) const EXCEPT {
  CLInstance::executeKernel(kernel, globalWorkSize,
                            getCommandQueue(preferredDeviceType), blocking);
}

size_t CLInstance::getMemorySize(const cl::Memory* memory) EXCEPT {
  if (memory == nullptr) {
    return 0;
  }
  size_t imageSize;
  cl_int error = memory->getInfo(CL_MEM_SIZE, &imageSize);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  return imageSize;
}

cl::Context CLInstance::getMemoryContext(const cl::Memory* memory) EXCEPT {
  cl::Context ctx;
  cl_int error = memory->getInfo(CL_MEM_CONTEXT, &ctx);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  return ctx;
}

cl::Buffer CLInstance::createBuffer(size_t byteSize,
                                    void* hostMemory) const EXCEPT {
  cl_int error;
  cl_mem_flags memflags = hostMemory == nullptr
                              ? CL_MEM_READ_WRITE
                              : CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
  cl::Buffer buffer(this->context, memflags, byteSize, hostMemory, &error);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  return buffer;
}

void CLInstance::fillBuffer(const cl::Buffer& buffer,
                            void* hostMemory) const EXCEPT {
  if (hostMemory == nullptr) {
    return;
  }

  cl_int error = this->getCommandQueue().enqueueWriteBuffer(
      buffer, true, 0, getMemorySize(&buffer), hostMemory);
  if (error) {
    raiseCLException(error, file_func_line());
  }
}

void CLInstance::readBuffer(const cl::Buffer& buffer,
                            void* hostMemory) const EXCEPT {
  if (hostMemory == nullptr) {
    return;
  }

  cl_int error = this->getCommandQueue().enqueueReadBuffer(
      buffer, true, 0, getMemorySize(&buffer), hostMemory);
  if (error) {
    raiseCLException(error, file_func_line());
  }
}

cl::Buffer CLInstance::copyBuffer(cl::Buffer buffer) const EXCEPT {
  size_t memsize = getMemorySize(&buffer);
  cl::Buffer bufferCopy = this->createBuffer(memsize);
  cl_int error = this->getCommandQueue().enqueueCopyBuffer(buffer, bufferCopy,
                                                           0, 0, memsize);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  return bufferCopy;
}

cl::Image3D CLInstance::createImage3D(const cl::ImageFormat& format,
                                      size_t width, size_t height, size_t depth,
                                      void* hostMemory) const EXCEPT {
  cl_int error;
  cl_mem_flags flags = hostMemory == nullptr
                           ? (CL_MEM_READ_WRITE)
                           : (CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
  cl::Image3D buffer(this->context, flags, format, width, height, depth, 0, 0,
                     hostMemory, &error);
  if (error) {
    raiseCLException(error, file_func_line());
  }

  // Make sure image was actually allocated on the device
  cl::size_t<3> origin;
  origin[0] = origin[1] = origin[2] = 0;
  cl::size_t<3> size;
  size[0] = size[1] = size[2] = 1;
  char temp[256];
  error = this->getCommandQueue().enqueueReadImage(buffer, true, origin, size,
                                                   0, 0, temp);
  if (error) {
    raiseCLException(error, file_func_line());
  }

  return buffer;
}

cl::Image2D CLInstance::createImage2D(const cl::ImageFormat& format,
                                      size_t width, size_t height,
                                      void* hostMemory) const EXCEPT {
  cl_int error;
  cl_mem_flags flags = hostMemory == nullptr
                           ? (CL_MEM_READ_WRITE)
                           : (CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
  cl::Image2D buffer(this->context, flags, format, width, height, 0, hostMemory,
                     &error);
  if (error) {
    raiseCLException(error, file_func_line());
  }

  // Make sure image was actually allocated on the device
  cl::size_t<3> origin;
  origin[0] = origin[1] = origin[2] = 0;
  cl::size_t<3> size;
  size[0] = size[1] = size[2] = 1;
  char temp[256];
  error = this->getCommandQueue().enqueueReadImage(buffer, true, origin, size,
                                                   0, 0, temp);
  if (error) {
    raiseCLException(error, file_func_line());
  }

  return buffer;
}

void CLInstance::fillImage(cl::Image* image, const cl::Buffer& buffer) const {
  if (image == nullptr) {
    return;
  }
  cl::size_t<3> origin;
  origin[0] = origin[1] = origin[2] = 0;
  cl_int error;
  error = getCommandQueue().enqueueCopyBufferToImage(buffer, *image, 0, origin,
                                                     getImageDimensions(image));
  if (error) {
    raiseCLException(error, file_func_line());
  }
}

void CLInstance::fillImage(cl::Image* image, void* hostMemory) const EXCEPT {
  if (hostMemory == nullptr || image == nullptr) {
    return;
  }
  cl::size_t<3> origin;
  origin[0] = origin[1] = origin[2] = 0;
  cl_int error = this->getCommandQueue().enqueueWriteImage(
      *image, true, origin, getImageDimensions(image), 0, 0, hostMemory);
  if (error) {
    raiseCLException(error, file_func_line());
  }
}

void CLInstance::readImage(const cl::Image* image,
                           void* hostMemory) const EXCEPT {
  if (hostMemory == nullptr || image == nullptr) {
    return;
  }
  cl::size_t<3> origin;
  origin[0] = origin[1] = origin[2] = 0;
  cl_int error = this->getCommandQueue().enqueueReadImage(
      *image, true, origin, getImageDimensions(image), 0, 0, hostMemory);
  if (error) {
    raiseCLException(error, file_func_line());
  }
}

void CLInstance::readImage(const cl::Image* image,
                           cl::Buffer& buffer) const EXCEPT {
  if (image == nullptr) {
    return;
  }
  cl::size_t<3> origin;
  origin[0] = origin[1] = origin[2] = 0;
  cl_int error = this->getCommandQueue().enqueueCopyImageToBuffer(
      *image, buffer, origin, getImageDimensions(image), 0);
  if (error) {
    raiseCLException(error, file_func_line());
  }
}

cl::size_t<3> CLInstance::getImageDimensions(const cl::Image* image) EXCEPT {
  cl::size_t<3> dims;
  cl_int error = image->getImageInfo(CL_IMAGE_WIDTH, &dims[0]);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  error = image->getImageInfo(CL_IMAGE_HEIGHT, &dims[1]);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  error = image->getImageInfo(CL_IMAGE_DEPTH, &dims[2]);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  dims[1] = (dims[1] == 0 ? 1 : dims[1]);
  dims[2] = (dims[2] == 0 ? 1 : dims[2]);

  return dims;
}

size_t CLInstance::getImageElementSize(const cl::Image* image) EXCEPT {
  size_t elementSize;
  cl_int error = image->getImageInfo(CL_IMAGE_ELEMENT_SIZE, &elementSize);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  return elementSize;
}

cl::Sampler CLInstance::createSampler(bool normalized,
                                      cl_addressing_mode addressingMode,
                                      cl_filter_mode filterMode) const EXCEPT {
  cl_int error;
  cl::Sampler sampler(this->context, normalized, addressingMode, filterMode,
                      &error);
  if (error) {
    raiseCLException(error, file_func_line());
  }
  return sampler;
}

QDebug vx::opencl::operator<<(QDebug dbg, const IOException& ex) {
  dbg.nospace() << "IOException: "
                << "[" << ex.getMessage() << "]";
  return dbg;
}
