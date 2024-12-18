/*
 * Copyright (c) 2014-2024 The Voxie Authors
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

#include "Common.hpp"

#include <VoxieClient/Exception.hpp>

#include <QtCore/QDebug>

namespace vx {
namespace libjpeg {
namespace IMPL_NAMESPACE {
QString getLibjpegErrorMessage(j_common_ptr cinfo) {
  char buffer[JMSG_LENGTH_MAX];
  (*cinfo->err->format_message)(cinfo, buffer);
  return buffer;
}

void handlerErrorExit(j_common_ptr cinfo) {
  jpeg_abort(cinfo);
  throw vx::Exception(
      "de.uni_stuttgart.Voxie.LibjpegError",
      "libjpeg returned an error: " + getLibjpegErrorMessage(cinfo));
}

// TODO: Allow changing this?
static const bool abort_on_warning = true;

// TODO: Allow changing this
static const int max_msg_level = -1;

void handlerEmitMessage(j_common_ptr cinfo, int msg_level) {
  if (msg_level >= 0) {
    if (msg_level <= max_msg_level)
      qDebug() << "libjpeg trace" << msg_level << getLibjpegErrorMessage(cinfo);
  } else {
    if (abort_on_warning) {
      jpeg_abort(cinfo);
      if (msg_level == -1) {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.LibjpegError",
            "libjpeg returned a warning: " + getLibjpegErrorMessage(cinfo));
      } else {
        throw vx::Exception("de.uni_stuttgart.Voxie.LibjpegError",
                            "libjpeg returned a message with level " +
                                QString::number(msg_level) + ": " +
                                getLibjpegErrorMessage(cinfo));
      }
    }
    if (msg_level <= max_msg_level) {
      if (msg_level == -1) {
        qWarning() << getLibjpegErrorMessage(cinfo);
      } else {
        qWarning() << "libjpeg" << msg_level << getLibjpegErrorMessage(cinfo);
      }
    }
  }
}

}  // namespace IMPL_NAMESPACE
}  // namespace libjpeg
}  // namespace vx
