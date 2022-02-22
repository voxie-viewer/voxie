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

#include <VoxieBackend/Data/FloatBuffer.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>

#include <QtCore/QObject>
#include <QtCore/QSize>

#include <QtGui/QImage>

namespace vx {
/**
 * Image with float value pixels.
 * The images data is shared across object copies of it. The data is
 * deleted when the images reference count decreases to 0. To actually
 * obtain a copy of the image (deep copy) use clone().
 *
 * The Floatimage's pixel data can be stored in one of 2 different locations
 * according to its MemoryMode. When in STDMEMORY_MODE the pixel data
 * is stored in a FloatBuffer as array. When in CLMEMORY_MODE the data
 * is stored in an openCL cl::Buffer for use with the openCL API for
 * fast processing on a graphics card. The mode can be switched using
 * one of the switchMode methods.
 *
 * Image's data may contain NaN values representing invalid or transparent
 * pixels
 *
 * @brief Image with float value pixels.
 */
class VOXIEBACKEND_EXPORT FloatImage {
 public:
  /**
   * Memory Mode determines where the current data of the FloatImage is stored.
   * STDMEMORY_MODE means current data is in a normal cpp accessible float[]
   * CLMEMORY_MODE means current data is in a cl::Buffer accessible via openCL
   */
  enum MemoryMode { STDMEMORY_MODE, CLMEMORY_MODE };

  friend class Colorizer;

 private:
  /** stores all data of the image */
  class FloatImageData {
   public:
    // throws Exception if enableSharedMemory is true
    FloatImageData(size_t w, size_t h, bool enableSharedMemory,
                   bool enableSharedMemoryWrite = false)
        : width(w),
          height(h),
          pixels(w * h, enableSharedMemory, enableSharedMemoryWrite),
          clPixels(),
          mode(STDMEMORY_MODE),
          clInstance(nullptr) {}
    size_t width, height;
    FloatBuffer pixels;
    cl::Buffer clPixels;
    MemoryMode mode;
    opencl::CLInstance* clInstance;
  };

 public:
  /**
   * @brief returns pixel at (x,y). Switches mode to STDMEMORY_MODE.
   * @param x coordinate of pixel
   * @param y coordinate of pixel
   * @return a pixel of the image.
   */
  inline float getPixel(size_t x, size_t y) {
    Q_ASSERT(x < this->getWidth() && y < this->getHeight());
    if (this->getMode() != STDMEMORY_MODE) switchMode();
    return this->imageData->pixels[this->getWidth() * y + x];
  }

  /**
   * @brief sets pixel at (x,y). Switches mode to STDMEMORY_MODE.
   * @param x coordinate of pixel
   * @param y coordinate of pixel
   * @param val new pixel value.
   */
  inline void setPixel(size_t x, size_t y, float val) {
    Q_ASSERT(x < this->getWidth() && y < this->getHeight());
    if (this->getMode() != STDMEMORY_MODE) switchMode();
    this->imageData->pixels[this->getWidth() * y + x] = val;
  }

  /**
   * @return Returns the width of the image.
   */
  size_t getWidth() const;

  /**
   * @return Returns the height of the image.
   */
  size_t getHeight() const;

  /**
   * @brief floatimage data can be located either in normal memory
   * or in OpenCL memory dependig on the floatimage's mode
   * @return current mode
   */
  MemoryMode getMode() const { return this->imageData->mode; }

  /**
   * @brief switches mode (CLMEMORY_MODE or STDMEMORY_MODE)
   * @param syncMemory whether memory should be synchronized on switch.
   * If true and switching from CLMEMORY_MODE to STDMEMORY_MODE CL memory's data
   * is read to standard memory. If true and switching from STDMEMORY_MODE to
   * CLMEMORY_MODE CL memory is allocated and filled with standard memory's
   * data.
   * @param clInstance to be used when switching to CLMEMORY_MODE, parameter is
   * ignored when switching to STDMEMORY_MODE. When passing NULL the default
   * instance is used.
   * @return MemoryMode of the image after call. It is not guarranteed that
   * switching to CLMEMORY_MODE will succeed, image will then remain in
   * STDMEMORY_MODE.
   * @throws opencl::CLException when switching to CLMEMORY_MODE fails. Switchng
   * to STDMEMORY_MODE will never throw.
   */
  MemoryMode switchMode(bool syncMemory = true,
                        opencl::CLInstance* clInstance = nullptr) EXCEPT;

  /**
   * switches MemoryMode and synchronizes memory
   * @see switchMode(bool, opencl::CLInstance*)
   * @throws opencl::CLException when switching to CLMEMORY_MODE fails.
   */
  MemoryMode switchMode(opencl::CLInstance* clInstance) EXCEPT {
    return switchMode(true, clInstance);
  }

  /**
   * switches MemoryMode when newMode is unequal to current mode
   * @see switchMode(bool, opencl::CLInstance*)
   * @throws opencl::CLException when switching to CLMEMORY_MODE fails.
   */
  MemoryMode switchMode(MemoryMode newMode, bool syncMemory = true,
                        opencl::CLInstance* clInstance = nullptr) EXCEPT {
    if (newMode != this->imageData->mode) {
      switchMode(syncMemory, clInstance);
    }
    return this->imageData->mode;
  }

  /**
   * switches MemoryMode when newMode is unequal to current mode
   * @see switchMode(opencl::CLInstance*)
   */
  MemoryMode switchMode(MemoryMode newMode,
                        opencl::CLInstance* clInstance) EXCEPT {
    if (newMode != this->imageData->mode) {
      switchMode(clInstance);
    }
    return this->imageData->mode;
  }

  /**
   * @brief returns a copy of this image's buffer. When MemoryMode is
   * CLMEMORY_MODE the returned buffer is filled with cl buffer's current data.
   * This synchronisation may fail, and will then set provided insync argument
   * to false
   * @param insync set to false when CLMEMORY_MODE synchronization failed and
   * returned buffer may not represent the current state of the images pixels
   * @return a copy of image's pixel buffer
   */
  FloatBuffer getBufferCopy(bool* insync = nullptr) const;

  /**
   * @brief returns this images buffer. When MemoryMode is CLMEMORY_MODE the
   * returned buffer is not in sync with cl buffer's current data. Changes to
   * the Buffer may then be overwritten when switching to STDMEMORY_MODE. Use
   * getBufferCopy
   * @return image's pixel buffer
   */
  FloatBuffer& getBuffer();
  const FloatBuffer& getBuffer() const;

  /**
   * @brief returns a copy of this images cl buffer. When MemoryMode is
   * STDMEMORY_MODE an invalid buffer will be returned.
   * @return copy of image's cl buffer.
   * @throws opencl::CLException when copying buffer fails
   */
  cl::Buffer getCLBufferCopy() const EXCEPT;

  /**
   * @brief returns this images cl buffer. When MemoryMode is STDMEMORY_MODE
   * an invalid buffer will be returned.
   * @return image's cl buffer
   */
  cl::Buffer& getCLBuffer();
  const cl::Buffer& getCLBuffer() const;

  /**
   * @brief returns clinstance corresponding to images opencl pixel buffer.
   * This will return NULL when in STDMEMORY_MODE
   * @return clinstance for use with image opencl pixel buffer
   */
  const opencl::CLInstance* getCLInstance() const {
    return this->imageData->clInstance;
  }

  /**
   * @brief returns clinstance corresponding to images opencl pixel buffer.
   * This will return NULL when in STDMEMORY_MODE
   * @return clinstance for use with image opencl pixel buffer
   */
  opencl::CLInstance* getCLInstance() { return this->imageData->clInstance; }

  /**
   * @return width and height of the image
   */
  QSize getDimension() const;

  /**
   * @return minimum value in this image
   */
  inline float getMinValue() const {
    float min = std::numeric_limits<float>::max();
    FloatBuffer buffer = this->getMode() == STDMEMORY_MODE
                             ? this->getBuffer()
                             : this->getBufferCopy();
    for (size_t i = 0; i < buffer.length(); i++) {
      if (buffer[i] < min) {
        min = buffer[i];
      }
    }
    return min;
  }

  /**
   * @return maximum value in this image
   */
  inline float getMaxValue() const {
    float max = std::numeric_limits<float>::lowest();
    FloatBuffer buffer = this->getMode() == STDMEMORY_MODE
                             ? this->getBuffer()
                             : this->getBufferCopy();
    for (size_t i = 0; i < buffer.length(); i++) {
      if (buffer[i] > max) {
        max = buffer[i];
      }
    }
    return max;
  }

  /**
   * @return minimum|maximum values in this image. Minimum is first maximum
   * second.
   */
  inline QPair<float, float> getMinMaxValue() const {
    float min = std::numeric_limits<float>::max();
    float max = std::numeric_limits<float>::lowest();
    FloatBuffer buffer = this->getMode() == STDMEMORY_MODE
                             ? this->getBuffer()
                             : this->getBufferCopy();
    for (size_t i = 0; i < buffer.length(); i++) {
      if (min > buffer[i]) min = buffer[i];
      if (max < buffer[i]) max = buffer[i];
    }
    return QPair<float, float>(min, max);
  }

  /**
   * creates a clone (deep copy) of this image. When in CLMEMORY_MODE will
   * attemp to copy cl pixel buffer but may fail, then clone will be in
   * STDMEMORY_MODE
   * @return clone of this image.
   */
  // throws Exception if enableSharedMemory is true
  FloatImage clone(bool enableSharedMemory = false) const;

 public:
  /**
   * @brief FloatImage constructor. FloatImage will be initialized in
   * STDMEMORY_MODE
   * @param width of image
   * @param height of image
   */
  // throws Exception if enableSharedMemory is true
  FloatImage(size_t width, size_t height, bool enableSharedMemory,
             bool enableSharedMemoryWrite = false);
  /**
   * @brief FloatImage creates an empty FloatImage of dimension (0,0)
   */
  FloatImage();

  QSharedPointer<vx::SharedMemory> getSharedMemory() {
    return imageData->pixels.getSharedMemory();
  }

 protected:
  /** holds image's data */
  QSharedPointer<FloatImageData> imageData;
};

}  // namespace vx

Q_DECLARE_METATYPE(vx::FloatImage)
