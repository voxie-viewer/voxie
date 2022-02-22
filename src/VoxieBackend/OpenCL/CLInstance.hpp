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

#include <VoxieBackend/OpenCL/CLUtil.hpp>

#include <VoxieBackend/lib/CL/cl-patched.hpp>

#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QThread>

namespace vx {
namespace opencl {

/**
 * Wrapper class for cl::Context providing utility methods for use with a
 * cl::Context. These include simplified method calls to the opencl api which
 * depend on cl::Context like cl::CommandQueue, cl::Memory object and
 * cl::Program methods.
 * Use CLInstance::getDefaultInstance() to obtain the default CLInstance used
 * by the application.
 * For thread safe use of cl::CommandQueue's CLInstance keeps a single
 * commandqueue for every device of it's context, these commandQueues can be
 * obtained with getCommandQueue.
 * CLInstance can store created cl::Program's for easier access to them from
 * different locations. see createProgram for details.
 * @brief Wrapper class for cl::Context providing utility methods for use with a
 * cl::Context.
 */
class VOXIEBACKEND_EXPORT CLInstance : public QObject {
  Q_OBJECT
 private:
  static bool initialized;
  static CLInstance* defaultInstance;

  cl::Context context;

  mutable QMutex mutex;
  QMap<QString, cl::Program> programs;
  QMap<cl_device_id, cl::CommandQueue> commandQueues;

 public:
  /**
   * Create an invalid instance
   */
  CLInstance() {}

  /**
   * @return new CLInstance of given context
   */
  static CLInstance* createInstance(const cl::Context& context) NOEXCEPT;

  /**
   * @return new CLInstance with given device
   */
  static CLInstance* createInstance(const cl::Device& device) NOEXCEPT;

  /**
   * @return new CLInstance with all gpus and cpus found on given platform
   * or all other devices when no cpus or gpus are present
   */
  static CLInstance* createInstance(const cl::Platform& platform) NOEXCEPT;

  /**
   * @return the default CLInstance (always a valid pointer when is initialized)
   */
  static CLInstance* getDefaultInstance() NOEXCEPT { return defaultInstance; }

  /**
   * @brief initializes default instance.
   * The defaultInstance will always be initialized when method returns.
   * Method can only be called once. Subsequent calls will have no effect.
   * @param defaultInst can be NULL then default instance will automaticlly
   * be initialized.
   */
  static void initialize(CLInstance* defaultInst) EXCEPT;

  /**
   * @return whether initialize() has been called yet.
   */
  static bool isInitialized() { return initialized; }

  // --

  /**
   * @return platform of this instance's devices
   * @throws CLException (CL_INVALID_DEVICE) should practically never happen,
   * (CL_INVALID_CONTEXT) if this instances context is invalid
   */
  cl::Platform getPlatform() const EXCEPT;

  /**
   * @return number of devices of given type in this instances context
   * @param type device type
   * @throws CLException (CL_INVALID_CONTEXT) if this instances context is
   * invalid
   */
  int getNumDevices(cl_device_type type = CL_DEVICE_TYPE_ALL) const EXCEPT;

  /**
   * @return devices of given type in this instances context
   * @param type device type
   * @throws CLException (CL_INVALID_CONTEXT) if this instances context is
   * invalid
   */
  QVector<cl::Device> getDevices(
      cl_device_type type = CL_DEVICE_TYPE_ALL) const EXCEPT;

  /**
   * @return device of preferred type from instances context, if instance
   * contains no device of preferred type the first device returned by
   * opencl::getDevices(context) will be returned.
   * @param preferredType preferred device type
   * @throws CLException (CL_INVALID_CONTEXT) if this instances context is
   * invalid
   */
  cl::Device getDevice(
      cl_device_type preferredType = CL_DEVICE_TYPE_GPU) const EXCEPT;

  /**
   * @return true iff this is a valid context
   */
  bool isValid() const NOEXCEPT { return context() != nullptr; }

  /**
   * @return this instance's context
   */
  cl::Context getContext() const NOEXCEPT { return this->context; }

  // --
  /**
   * @return commandQueue for device of preffered device type, if instance
   * contains no device of preferred type the commandQueue for the first device
   * of this instance is returned.
   * @param preferredDevice type of preferred device
   * @throws CLException (CL_INVALID_CONTEXT) if this instances context is
   * invalid
   */
  cl::CommandQueue getCommandQueue(
      cl_device_type preferredDevice = CL_DEVICE_TYPE_GPU) const EXCEPT;

  /**
   * @return commandQueue for given device, if device is not part of this
   * instance an invalid command queue will be returned.
   * @param device for commandQueue
   */
  cl::CommandQueue getCommandQueue(const cl::Device& device) const NOEXCEPT;

  /**
   * @return newly created commandQueue for preferred device type, if instance
   * contains no device of preferred type a commandQueue for the instances first
   * device will be created and returned
   * @param preferredDevice preferred device type
   * @throws CLException. See opencl::createCommandQueue() for details about
   * throw causes.
   */
  cl::CommandQueue createCommandQueue(
      cl_device_type preferredDevice = CL_DEVICE_TYPE_GPU) const EXCEPT;

  /**
   * @return newly created commandQueue for given device. If device is not
   * part of this instances context CLException is thrown.
   * @param device device for commandQueue
   * @throws CLException. See opencl::createCommandQueue() for details about
   * throw causes.
   */
  cl::CommandQueue createCommandQueue(const cl::Device& device) const EXCEPT;

  // --

  /**
   * @returns true when instance contains a program associated with given id.
   * @param programID
   * @see createProgram
   */
  bool hasProgramID(const QString& programID) const NOEXCEPT;

  /**
   * Creates program from source string and builds it for all devices of this
   * instance. When id is provided the Program will be stored within this
   * instance and can be obtained with getProgram(id). If build fails build log
   * is put to qDebug and CLException is thrown.
   * @param source sourcecode of the program
   * @param buildOptions options for building the program, can be emtpy
   * @param id used to obtain Program from instance later, if empty the program
   * will not be stored in instance.
   * @return program
   * @throws CLException
   */
  cl::Program createProgram(const QString& source, const QString& buildOptions,
                            const QString& id = QString()) EXCEPT;

  /**
   * Creates program file and builds it for all devices of this instance.
   * When id is provided the Program will be stored within this instance and can
   * be obtained with getProgram(id). If build fails build log is put to qDebug
   * and CLException is thrown.
   * @param source sourcecode of the program
   * @param buildOptions options for building the program, can be emtpy
   * @param id used to obtain Program from instance later, if empty the program
   * will not be stored in instance.
   * @return program
   * @throws IOException CLException
   */
  cl::Program createProgramFromFile(const QString& filename,
                                    const QString& buildOptions,
                                    const QString& id = QString()) EXCEPT;

  /**
   * @return Program for given id, if no program is stored with this id an
   * invalid cl::Program is returned.
   * @param programId
   */
  cl::Program getProgram(const QString& programId) const NOEXCEPT;

  /**
   * @return kernel from program with id programId with given kernelName, if
   * no program is stored with given id or kernelName is empty an invalid
   * cl::Kernel is returned.
   * @param programId of kernels program
   * @param kernelName name of the kernel
   * @throws CLException
   * if program was not sucessfully built (CL_INVALID_PROGRAM_EXECUTABLE),
   * if kernelName is not found in Program (CL_INVALID_KERNEL_NAME),
   * if there is a failure to allocate resources required by the OpenCL
   * implementation on the host (CL_OUT_OF_HOST_MEMORY)
   */
  cl::Kernel getKernel(const QString& programId,
                       const QString& kernelName) const EXCEPT;

  /**
   * Enqueues NDRange Kernel on given commandQeueue with given global works
   * size, without offset and local work size determinded by by the openCl
   * implementation.
   * @param kernel the kernel to be executed
   * @param gloablWorksize the global workgroup size
   * @param commandQueue the commandQueue to enqueue kernel execution to
   * @param blocking if true, method will return when kernel finished executing
   * otherwise method will return emmediately
   * @throws CLException (CL_INVALID_PROGRAM_EXECUTABLE,
   * CL_INVALID_COMMAND_QUEUE, CL_INVALID_KERNEL, CL_INVALID_CONTEXT,
   * CL_INVALID_KERNEL_ARGS, CL_INVALID_WORK_DIMENSION,
   * CL_INVALID_GLOBAL_WORK_SIZE, CL_INVALID_WORK_GROUP_SIZE if
   * __attribute__((reqd_work_group_size(X, Y, Z))) qualifier is used in program
   * for kernel, CL_INVALID_IMAGE_SIZE CL_OUT_OF_RESOURCES,
   * CL_MEM_OBJECT_ALLOCATION_FAILURE, CL_OUT_OF_HOST_MEMORY)
   * @see clEnqueueNDRangeKernel()
   */
  static void executeKernel(const cl::Kernel& kernel,
                            const cl::NDRange& globalWorkSize,
                            const cl::CommandQueue& commandQueue,
                            bool blocking = true) EXCEPT;

  /**
   * Enqueues NDRange Kernel on commandQueue for device of preferred type with
   * given global works size, without offset and local work size determinded
   * by by the openCl implementation. If no device of preferred type is found
   * in this instancem te command queue of this instances first device is used
   * @param kernel kernel to be executed
   * @param gloablWorkSize the global workgroup size
   * @param preferredDeviceType
   * @param blocking blocking if true, method will return when kernel finished
   * executing otherwise method will return emmediately
   * @throws CLException see CLInstance::executeKernel()
   */
  void executeKernel(const cl::Kernel& kernel,
                     const cl::NDRange& globalWorkSize,
                     cl_device_type preferredDeviceType = CL_DEVICE_TYPE_GPU,
                     bool blocking = true) const EXCEPT;

  // --

  /**
   * @return byte size of memory object
   * @param memory pointer to memory object, e.g. cl::Buffer cl::Image
   * @throws CLException when memory object is invalid (CL_INVALID_MEM_OBJECT)
   */
  static size_t getMemorySize(const cl::Memory* memory) EXCEPT;

  /**
   * @return context associated with given memory object
   * @param memory pointer to memory object, e.g. cl::Buffer cl::Image
   * @throws CLException when memory object is invalid (CL_INVALID_MEM_OBJECT)
   */
  static cl::Context getMemoryContext(const cl::Memory* memory) EXCEPT;

  /**
   * creates cl::Buffer of given byteSize and fills it with hostMemory if
   * provided
   * @return buffer
   * @param byteSize size of buffer in bytes
   * @param hostMemory pointer to hostMemory returned buffer shall be filled
   * with
   * @throws CLException
   */
  cl::Buffer createBuffer(size_t byteSize,
                          void* hostMemory = nullptr) const EXCEPT;
  /**
   * fills given buffer with hostMemory, make sure hostMemory has at least
   * buffer size.
   * @param buffer to be filled
   * @param hostMemory to fill buffer with
   * @throws CLException
   */
  void fillBuffer(const cl::Buffer& buffer, void* hostMemory) const EXCEPT;
  /**
   * fills hostMemory with given buffer, make sure hostMemory has at least
   * buffer size.
   * @param buffer to be read
   * @param hostMemory to be filled
   * @throws CLException
   */
  void readBuffer(const cl::Buffer& buffer, void* hostMemory) const EXCEPT;

  /**
   * @return QVector of given type T filled with buffer. QVector has so many
   * elements to fit buffer size inside
   * @param buffer buffer to be read
   * @throws CLException
   */
  template <typename T>
  QVector<T> readBuffer(const cl::Buffer& buffer) const EXCEPT {
    size_t bufferSize = getMemorySize(&buffer);
    size_t elementSize = sizeof(T);
    size_t numElements = (bufferSize % elementSize == 0)
                             ? bufferSize / elementSize
                             : (bufferSize / elementSize) + 1;
    QVector<T> hostBuffer(numElements);
    this->readBuffer(buffer, hostBuffer.data());
    return hostBuffer;
  }

  /**
   * @return copy of given buffer
   * @param buffer to copy
   * @throws CLException
   */
  cl::Buffer copyBuffer(cl::Buffer buffer) const EXCEPT;

  // --

  /**
   * creates image3D of given format and dimensions filled with hostMemory if
   * provided
   * @return image
   * @param format the format used in this image
   * @param width of image
   * @param height of image
   * @param depth of image
   * @param hostMemory pointer to host memory this image is filled with
   * @throws CLException
   */
  cl::Image3D createImage3D(const cl::ImageFormat& format, size_t width,
                            size_t height, size_t depth,
                            void* hostMemory = nullptr) const EXCEPT;

  /**
   * creates image2D of given format and dimensions filled with hostMemory if
   * provided
   * @return image
   * @param format the format used in this image
   * @param width of image
   * @param height of image
   * @param hostMemory pointer to host memory this image is filled with
   * @throws CLException
   */
  cl::Image2D createImage2D(const cl::ImageFormat& format, size_t width,
                            size_t height,
                            void* hostMemory = nullptr) const EXCEPT;

  /**
   * fills image with given clBuffer, make sure buffer size is equal or greater
   * than image size
   * @param image pointer to image object
   * @param buffer to fill image with
   * @throws CLException
   */
  void fillImage(cl::Image* image, const cl::Buffer& buffer) const EXCEPT;

  /**
   * fills image with given hostMemory, make sure hostMemory size is equal or
   * greater than image size
   * @param image pointer to image object
   * @param pointer to hostMemory to fill image with
   * @throws CLException
   */
  void fillImage(cl::Image* image, void* hostMemory) const EXCEPT;

  /**
   * fills hostMemory with given image, make sure hostMemory size is equal or
   * greater than image size
   * @param image pointer to image object to fill hostMemory with
   * @param pointer to hostMemory to be filled
   * @throws CLException
   */
  void readImage(const cl::Image* image, void* hostMemory) const EXCEPT;

  /**
   * fills buffer with given image, make sure buffer size is equal or greater
   * than image size
   * @param image pointer to image object to fill hostMemory with
   * @param buffer to be filled
   * @throw CLException
   */
  void readImage(const cl::Image* image, cl::Buffer& buffer) const EXCEPT;

  /**
   * @return QVector of type T filled with image data. QVector has so many
   * elements to fit image size inside.
   * @param image pointer to imageObject to read from
   * @throws CLException
   */
  template <typename T>
  QVector<T> readImage(const cl::Image* image) const EXCEPT {
    size_t elementSize = sizeof(T);
    size_t imageSize = getMemorySize(image);
    size_t numElements = (imageSize % elementSize == 0)
                             ? imageSize / elementSize
                             : (imageSize / elementSize) + 1;
    QVector<T> hostBuffer(numElements);
    this->readImage(image, hostBuffer.data());
    return hostBuffer;
  }

  /**
   * @return images dimensions as size_t3, if given image has only 2 (or 1)
   * dimensions the 3rd (and 2nd) component is 1.
   * @param image
   * @throws CLException
   */
  static cl::size_t<3> getImageDimensions(const cl::Image* image) EXCEPT;

  /**
   * @return bytesize of a single element of given image
   * @param image
   * @throws CLException
   */
  static size_t getImageElementSize(const cl::Image* image) EXCEPT;

  /**
   * @return sampler with given filter and adressing mode normalized or not
   * normalized
   * @param normalized whether sampler uses normalized coordinates
   * @param adressingMode CL_ADDRESS_NONE CL_ADDRESS_CLAMP_TO_EDGE
   * CL_ADDRESS_CLAMP CL_ADDRESS_REPEAT
   * @param filterMode CL_FILTER_NEAREST CL_FILTER_LINEAR
   * @throws CLException
   */
  cl::Sampler createSampler(bool normalized, cl_addressing_mode adressingMode,
                            cl_filter_mode filterMode) const EXCEPT;

 private:
  CLInstance(cl::Context context);
  Q_DISABLE_COPY(CLInstance)
};

/**
 * An Instance of this class is thrown from CLInstance::createProgramFromFile()
 * when an IO error occurs. An IOException stores an error message.
 * @brief The IOException class
 */
class VOXIEBACKEND_EXPORT IOException : public QException {
 private:
  QString message;
  std::string messageStd;

 public:
  IOException(const QString& msg = "")
      : message(msg), messageStd(this->message.toStdString()) {}

  const QString& getMessage() const { return this->message; }

  IOException* clone() const override { return new IOException(*this); }

  void raise() const override { throw *this; }

  const char* what() const NOEXCEPT override { return messageStd.c_str(); }
};

/** qDebug streaming operator for IOException */
VOXIEBACKEND_EXPORT QDebug operator<<(QDebug dbg, const IOException& ex);

}  // namespace opencl
}  // namespace vx
