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

#include "WindowMode.hpp"

#include <VoxieClient/Exception.hpp>

#include <QtWidgets/QWidget>

#include <QtCore/QString>

QString vx::windowModeToString(WindowMode mode) {
  switch (mode) {
    case WindowMode::Normal:
      return "de.uni_stuttgart.Voxie.WindowMode.Normal";
    case WindowMode::Minimized:
      return "de.uni_stuttgart.Voxie.WindowMode.Minimized";
    case WindowMode::Maximized:
      return "de.uni_stuttgart.Voxie.WindowMode.Maximized";
    case WindowMode::FullScreen:
      return "de.uni_stuttgart.Voxie.WindowMode.FullScreen";
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Got invalid windows mode");
  }
}
vx::WindowMode vx::parseWindowMode(const QString& str) {
  if (str == "de.uni_stuttgart.Voxie.WindowMode.Normal")
    return WindowMode::Normal;
  if (str == "de.uni_stuttgart.Voxie.WindowMode.Minimized")
    return WindowMode::Minimized;
  if (str == "de.uni_stuttgart.Voxie.WindowMode.Maximized")
    return WindowMode::Maximized;
  if (str == "de.uni_stuttgart.Voxie.WindowMode.FullScreen")
    return WindowMode::FullScreen;
  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "Got invalid WindowsMode value: '" + str + "'");
}

vx::WindowMode vx::getWindowMode(QWidget* widget) {
  if (widget->isMinimized()) return vx::WindowMode::Minimized;
  if (widget->isFullScreen()) return vx::WindowMode::FullScreen;
  if (widget->isMaximized()) return vx::WindowMode::Maximized;
  return vx::WindowMode::Normal;
}
void vx::setWindowMode(QWidget* widget, WindowMode mode) {
  switch (mode) {
    case vx::WindowMode::Normal:
      widget->showNormal();
      break;
    case vx::WindowMode::Minimized:
      widget->showMinimized();
      break;
    case vx::WindowMode::Maximized:
      widget->showMaximized();
      break;
    case vx::WindowMode::FullScreen:
      widget->showFullScreen();
      break;
    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "vx::setWindowMode: Got invalid WindowsMode value");
  }
}
