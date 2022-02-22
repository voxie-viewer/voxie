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

#include <qopengl.h>
#include <QObject>
#include <cstring>

/**
 * @brief The Texture class is a data class that holds the raw pixel data of a
 * texture ready to use in OpenGL.
 * @author Robin Höchster
 */
class Texture : public QObject {
  Q_OBJECT
 public:
  /**
   * @brief Texture creates a new instance and saves the given pixel data as
   * pointer or makes a deep copy.
   * @param textureID The OpenGL texture ID obtained from glGenTextures()
   * @param pixels
   * @param width The pixel width of the texture.
   * @param height The pixel height of the texture.
   * @param bufferSize The byte size of the pixel data.
   * @param makeCopy If true, the pixels are copied completely. If done so, the
   * buffer holding the pixels will be deleted on destruction of the texture
   * object.
   */
  Texture(GLuint textureID, const void* pixels, const int width,
          const int height, const long bufferSize, const bool makeCopy = false)
      : _textureID(textureID),
        _width(width),
        _height(height),
        _bufferSize(bufferSize),
        deleteBuffer(makeCopy) {
    if (makeCopy) {
      auto p = new unsigned char[bufferSize];
      _pixels = p;
      memcpy(p, pixels, bufferSize);
    }
  }

  ~Texture() {
    if (deleteBuffer) {
      delete[] _pixels;
      _pixels = NULL;
    }
  }

  /**
   * @brief pixels returns the pixel data.
   * @return
   */
  const void* pixels() const { return _pixels; }

  /**
   * @brief width returns the pixel width of the texture.
   * @return
   */
  int width() const { return _width; }

  /**
   * @brief height returns the pixel height of the texture.
   * @return
   */
  int height() const { return _height; }

  /**
   * @brief bufferSize returns the byte size of the buffer holding the pixel
   * data.
   * @return
   */
  long bufferSize() const { return _bufferSize; }

  /**
   * @brief textureID returns the OpenGL texture ID.
   * @return
   */
  GLuint textureID() const { return _textureID; }

 private:
  const unsigned char* _pixels;
  const GLuint _textureID = 0;
  const int _width = 0;
  const int _height = 0;
  const long _bufferSize;
  const bool deleteBuffer = false;
};
