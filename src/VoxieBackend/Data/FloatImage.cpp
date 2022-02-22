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

#include "FloatImage.hpp"

#include <VoxieClient/Exception.hpp>

#include <string.h>
#include <cmath>

#include <QtGui/QColor>
#include <QtGui/QRgb>

// TODO: This class is used from multiple threads, but is not really safe for
// multithreading

using namespace vx;

FloatImage::FloatImage(size_t width, size_t height, bool enableSharedMemory,
                       bool enableSharedMemoryWrite)
    : imageData(new FloatImageData(width, height, enableSharedMemory,
                                   enableSharedMemoryWrite)) {}

FloatImage::FloatImage() : imageData(new FloatImageData(0, 0, false)) {}

size_t FloatImage::getWidth() const { return this->imageData->width; }

size_t FloatImage::getHeight() const { return this->imageData->height; }

QSize FloatImage::getDimension() const {
  return QSize((int)this->getWidth(), (int)this->getHeight());
}

FloatBuffer FloatImage::getBufferCopy(bool* insync) const {
  FloatBuffer buffer = this->imageData->pixels.copy(false);
  if (this->getMode() == CLMEMORY_MODE) {
    // fetch from clBuffer
    try {
      this->imageData->clInstance->readBuffer(this->imageData->clPixels,
                                              buffer.data());
      if (insync != nullptr) *insync = true;
    } catch (opencl::CLException& ex) {
      if (insync != nullptr) *insync = false;
      qWarning() << ex;
    }
  } else {
    if (insync != nullptr) *insync = true;
  }
  return buffer;
}

FloatBuffer& FloatImage::getBuffer() {
  if (this->getMode() == CLMEMORY_MODE) {
    qWarning() << "Image is in CLMEMORY_MODE - Buffer with write access may be "
                  "overwritten on mode switch";
  }
  return this->imageData->pixels;
}
const FloatBuffer& FloatImage::getBuffer() const {
  if (this->getMode() == CLMEMORY_MODE) {
    qWarning() << "Image is in CLMEMORY_MODE - Buffer with write access may be "
                  "overwritten on mode switch";
  }
  return this->imageData->pixels;
}

FloatImage::MemoryMode FloatImage::switchMode(
    bool syncMemory, opencl::CLInstance* clInstance) EXCEPT {
  if (this->getMode() == STDMEMORY_MODE) {
    // switch to CL
    this->imageData->clInstance =
        (clInstance == nullptr ? opencl::CLInstance::getDefaultInstance()
                               : clInstance);
    try {
      this->imageData->clPixels = this->imageData->clInstance->createBuffer(
          this->imageData->pixels.byteSize(),
          (syncMemory ? this->imageData->pixels.data() : nullptr));
      this->imageData->mode = CLMEMORY_MODE;
    } catch (opencl::CLException& ex) {
      qWarning() << "Cannot switch mode" << ex;
      this->imageData->clInstance = nullptr;
      ex.raise();  // raise exception to next lvl
    }

  } else {
    // switch to STD
    if (syncMemory) {
      try {
        this->getCLInstance()->readBuffer(this->imageData->clPixels,
                                          this->imageData->pixels.data());
      } catch (opencl::CLException& ex) {
        qWarning() << "Cannot sync buffers" << ex;
        // dont throw, this must always succeed//ex.raise();
      }
    }
    this->imageData->clPixels =
        cl::Buffer();  // reset clbuffer to free memory on device
    this->imageData->clInstance = nullptr;
    this->imageData->mode = STDMEMORY_MODE;
  }

  return this->getMode();
}

cl::Buffer FloatImage::getCLBufferCopy() const EXCEPT {
  if (this->imageData->mode == STDMEMORY_MODE) {
    return cl::Buffer();
  } else {
    Q_ASSERT(this->imageData->clInstance != nullptr);
    return this->imageData->clInstance->copyBuffer(this->imageData->clPixels);
  }
}

cl::Buffer& FloatImage::getCLBuffer() { return this->imageData->clPixels; }
const cl::Buffer& FloatImage::getCLBuffer() const {
  return this->imageData->clPixels;
}

FloatImage FloatImage::clone(bool enableSharedMemory) const {
  FloatImage _clone;
  _clone.imageData->width = this->getWidth();
  _clone.imageData->height = this->getHeight();

  bool clFailed = false;
  if (this->getMode() == CLMEMORY_MODE) {
    try {
      _clone.imageData->clPixels = this->getCLBufferCopy();  // might throw
      _clone.imageData->pixels = this->imageData->pixels.copy(
          enableSharedMemory);  // not the same as getBufferCopy
      _clone.imageData->clInstance = this->imageData->clInstance;
      _clone.imageData->mode = CLMEMORY_MODE;
    } catch (opencl::CLException&) {
      clFailed = true;
    }
  }
  if (this->getMode() == STDMEMORY_MODE || clFailed) {
    _clone.imageData->pixels = this->getBufferCopy();
  }
  return _clone;
}
