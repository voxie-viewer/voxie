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

#include <Voxie/Data/Color.hpp>

namespace vx {
enum class AlphaInterpolationType {
  Linear,
  Left,
  Right,
  AlphaInterpolationTypeCount,
};
class VOXIECORESHARED_EXPORT ColorInterpolator {
 public:
  enum InterpolationType {
    RGB,
    LAB,
    MSH,
    MSHDiverging,
    Left,
    Right,
    InterpolationTypeCount,
  };

  ColorInterpolator(
      InterpolationType type,
      AlphaInterpolationType alphaType = vx::AlphaInterpolationType::Linear);

  Color interpolate(Color left, Color right, float ratio) const;

  inline InterpolationType getType() const { return type; }
  void setAlphaType(AlphaInterpolationType type) { alphaType = type; }
  AlphaInterpolationType getAlphaType() { return alphaType; }

 private:
  static Color interpolateRGB(Color left, Color right, float ratio);
  static Color interpolateLAB(Color left, Color right, float ratio);
  static Color interpolateMSH(Color left, Color right, float ratio);
  static Color interpolateMSHDiverging(Color left, Color right, float ratio);
  static Color interpolateRight(Color left, Color right, float ratio);
  static Color interpolateLeft(Color left, Color right, float ratio);

  InterpolationType type = RGB;
  AlphaInterpolationType alphaType = vx::AlphaInterpolationType::Linear;
};

}  // namespace vx
