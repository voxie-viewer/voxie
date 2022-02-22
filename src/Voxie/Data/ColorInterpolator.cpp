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

#include <Voxie/Data/ColorInterpolator.hpp>

#include <array>
#include <cmath>

#include <QtCore/QDebug>

namespace vx {

namespace internal {

namespace color {

// TODO possibly make these classes accessible if they turn out to be useful

static constexpr float pi = 3.14159265359f;

struct Rgb;
struct Xyz;
struct Lab;
struct Msh;

using Vec3 = std::array<float, 3>;
using M3x3 = std::array<Vec3, 3>;

struct Rgb {
  inline Rgb(const vx::Color& color)
      : r(color.red()), g(color.green()), b(color.blue()) {}
  inline Rgb(float r, float g, float b) : r(r), g(g), b(b) {}
  inline explicit Rgb(Xyz xyz);

  inline vx::Color toColor() {
    return vx::Color(std::make_tuple(static_cast<double>(r),
                                     static_cast<double>(g),
                                     static_cast<double>(b), 1.0));
  }

  float r;
  float g;
  float b;
};

struct Xyz {
  // Illuminant D65 with Y = 100
  static constexpr M3x3 mTo = {{{0.4124f, 0.3576f, 0.1805f},
                                {0.2126f, 0.7152f, 0.0722f},
                                {0.0193f, 0.1192f, 0.9505f}}};
  static constexpr M3x3 mFrom = {{{+3.2406f, -1.5372f, -0.4986f},
                                  {-0.9689f, +1.8758f, +0.0415f},
                                  {+0.0557f, -0.2040f, +1.0570f}}};

  inline Xyz(float x, float y, float z) : x(x), y(y), z(z) {}
  inline explicit Xyz(Rgb rgb);
  inline explicit Xyz(Lab lab);

  float x;
  float y;
  float z;
};

constexpr M3x3 Xyz::mTo;
constexpr M3x3 Xyz::mFrom;

struct Lab {
  // Illuminant D65 with Y = 100
  static constexpr float Xn = 95.0489f;
  static constexpr float Yn = 100.0f;
  static constexpr float Zn = 108.8840f;
  static constexpr float Delta = 6.f / 29.f;

  inline Lab(float l, float a, float b) : l(l), a(a), b(b) {}
  inline explicit Lab(Xyz xyz);
  inline explicit Lab(Msh msh);

  float l;
  float a;
  float b;
};

struct Msh {
  inline Msh(float m, float s, float h) : m(m), s(s), h(h) {}
  inline explicit Msh(Lab lab);

  float m;
  float s;
  float h;
};

inline float lerp(float v1, float v2, float ratio) {
  return v1 * (1.f - ratio) + v2 * ratio;
}

inline Rgb lerp(const Rgb& rgb1, const Rgb& rgb2, float ratio) {
  return Rgb(lerp(rgb1.r, rgb2.r, ratio), lerp(rgb1.g, rgb2.g, ratio),
             lerp(rgb1.b, rgb2.b, ratio));
}

inline Xyz lerp(const Xyz& xyz1, const Xyz& xyz2, float ratio) {
  return Xyz(lerp(xyz1.x, xyz2.x, ratio), lerp(xyz1.y, xyz2.y, ratio),
             lerp(xyz1.z, xyz2.z, ratio));
}

inline Lab lerp(const Lab& lab1, const Lab& lab2, float ratio) {
  return Lab(lerp(lab1.l, lab2.l, ratio), lerp(lab1.a, lab2.a, ratio),
             lerp(lab1.b, lab2.b, ratio));
}

inline Msh lerp(const Msh& msh1, const Msh& msh2, float ratio) {
  return Msh(lerp(msh1.m, msh2.m, ratio), lerp(msh1.s, msh2.s, ratio),
             lerp(msh1.h, msh2.h, ratio));
}

inline Vec3 matrixMultiply(const M3x3& m, const Vec3& vec) {
  Vec3 result;
  for (int i = 0; i < 3; ++i) {
    result[i] = vec[0] * m[i][0] + vec[1] * m[i][1] + vec[2] * m[i][2];
  }
  return result;
}

Rgb::Rgb(Xyz xyz) {
  // https://en.wikipedia.org/wiki/SRGB
  auto gamma = [](float u) -> float {
    u /= 100.f;
    return u <= 0.0031308 ? 12.92 * u
                          : (211. * std::pow(u, 5. / 12.) - 11.) / 200.;
  };

  Vec3 linear = matrixMultiply(Xyz::mFrom, {xyz.x, xyz.y, xyz.z});
  r = gamma(linear[0]);
  g = gamma(linear[1]);
  b = gamma(linear[2]);
}

Xyz::Xyz(Rgb rgb) {
  // https://en.wikipedia.org/wiki/SRGB
  auto invGamma = [](float u) -> float {
    return 100.f * (u <= 0.04045 ? u / 12.92
                                 : std::pow((200. * u + 11.) / 211., 12. / 5.));
  };

  Vec3 linear = {invGamma(rgb.r), invGamma(rgb.g), invGamma(rgb.b)};
  Vec3 xyz = matrixMultiply(Xyz::mTo, linear);
  x = xyz[0];
  y = xyz[1];
  z = xyz[2];
}

Xyz::Xyz(Lab lab) {
  // https://en.wikipedia.org/wiki/CIELAB_color_space
  float l = (lab.l + 16) / 116;
  float a = lab.a / 500;
  float b = lab.b / 200;

  auto f = [](float t) -> float {
    static constexpr float deltaSquared = Lab::Delta * Lab::Delta;
    return t > Lab::Delta ? t * t * t : (3. * deltaSquared) * (t - 4. / 29.);
  };

  x = Lab::Xn * f(l + a);
  y = Lab::Yn * f(l);
  z = Lab::Zn * f(l - b);
}

Lab::Lab(Xyz xyz) {
  // https://en.wikipedia.org/wiki/CIELAB_color_space
  float x = xyz.x / Lab::Xn;
  float y = xyz.y / Lab::Yn;
  float z = xyz.z / Lab::Zn;

  auto fi = [](float t) -> float {
    static constexpr float deltaSquared = Lab::Delta * Lab::Delta;
    static constexpr float deltaCubed = Lab::Delta * Lab::Delta * Lab::Delta;
    return t > deltaCubed ? std::cbrt(t) : t / (3. * deltaSquared) + 4. / 29.;
  };

  l = 116 * fi(y) - 16;
  a = 500 * (fi(x) - fi(y));
  b = 200 * (fi(y) - fi(z));
}

Lab::Lab(Msh msh) {
  // "Diverging Color Maps for Scientific Visualization." Kenneth Moreland.
  // https://www.kennethmoreland.com/color-maps/ColorMapsExpanded.pdf
  l = msh.m * std::cos(msh.s);
  a = msh.m * std::sin(msh.s) * std::cos(msh.h);
  b = msh.m * std::sin(msh.s) * std::sin(msh.h);
}

Msh::Msh(Lab lab) {
  // "Diverging Color Maps for Scientific Visualization." Kenneth Moreland.
  // https://www.kennethmoreland.com/color-maps/ColorMapsExpanded.pdf
  m = std::sqrt(lab.l * lab.l + lab.a * lab.a + lab.b * lab.b);
  s = std::acos(lab.l / m);
  h = std::atan2(lab.b, lab.a);
}

static float mshAdjustHue(Msh msh, float mUnsat) {
  // "Diverging Color Maps for Scientific Visualization." Kenneth Moreland.
  // https://www.kennethmoreland.com/color-maps/ColorMapsExpanded.pdf
  if (msh.m >= mUnsat) {
    return msh.h;
  } else {
    // Try to spin the hue away from purple
    float hSpin = (msh.s * std::sqrt(mUnsat * mUnsat - msh.m * msh.m)) /
                  (msh.m * std::sin(msh.s));
    return msh.h > -(pi / 3.) ? msh.h + hSpin : msh.h - hSpin;
  }
}

}  // namespace color

}  // namespace internal

using namespace internal::color;

ColorInterpolator::ColorInterpolator(InterpolationType type,
                                     AlphaInterpolationType alphaType)
    : type(type), alphaType(alphaType) {}

Color ColorInterpolator::interpolate(Color left, Color right,
                                     float ratio) const {
  auto apply = [&](Color (*interpolator)(Color, Color, float)) -> Color {
    auto clamp = [](float value) -> float {
      return std::max(0.f, std::min(value, 1.f));
    };

    Color result = interpolator(left, right, ratio);

    // Clamp result to range [0; 1]
    result.setRed(clamp(result.red()));
    result.setGreen(clamp(result.green()));
    result.setBlue(clamp(result.blue()));

    // Use simple linear interpolation for alpha
    switch (alphaType) {
      case AlphaInterpolationType::Linear:
      default:
        result.setAlpha(clamp(lerp(left.alpha(), right.alpha(), ratio)));
        break;
      case AlphaInterpolationType::Left:
        result.setAlpha(clamp(left.alpha()));
        break;
      case AlphaInterpolationType::Right:
        result.setAlpha(clamp(right.alpha()));
        break;
    }
    return result;
  };

  switch (type) {
    case RGB:
    default:
      return apply(interpolateRGB);
    case LAB:
      return apply(interpolateLAB);
    case MSH:
      return apply(interpolateMSH);
    case MSHDiverging:
      return apply(interpolateMSHDiverging);
    case Right:
      return apply(interpolateRight);
    case Left:
      return apply(interpolateLeft);
  }
}

Color ColorInterpolator::interpolateRGB(Color left, Color right, float ratio) {
  return lerp(Rgb(left), Rgb(right), ratio).toColor();
}

Color ColorInterpolator::interpolateLAB(Color left, Color right, float ratio) {
  return Rgb(Xyz(lerp(Lab(Xyz(Rgb(left))), Lab(Xyz(Rgb(right))), ratio)))
      .toColor();
}

Color ColorInterpolator::interpolateRight(Color left, Color right,
                                          float ratio) {
  Q_UNUSED(left);
  Q_UNUSED(ratio);
  return right;
}

Color ColorInterpolator::interpolateLeft(Color left, Color right, float ratio) {
  Q_UNUSED(right);
  Q_UNUSED(ratio);
  return left;
}

Color ColorInterpolator::interpolateMSH(Color left, Color right, float ratio) {
  return Rgb(Xyz(Lab(lerp(Msh(Lab(Xyz(Rgb(left)))), Msh(Lab(Xyz(Rgb(right)))),
                          ratio))))
      .toColor();
}

Color ColorInterpolator::interpolateMSHDiverging(Color left, Color right,
                                                 float ratio) {
  // "Diverging Color Maps for Scientific Visualization." Kenneth Moreland.
  // https://www.kennethmoreland.com/color-maps/ColorMapsExpanded.pdf

  // Convert gradient colors to MSH
  Msh msh1 = Msh(Lab(Xyz(Rgb(left))));
  Msh msh2 = Msh(Lab(Xyz(Rgb(right))));

  // Threshold for a color to be considered "saturated"
  static constexpr float minSat = 0.05f;

  // Add white center point for saturated gradient colors
  if (std::min(msh1.s, msh2.s) > minSat &&
      std::abs(msh1.h - msh2.h) > pi / 3.f) {
    float mMid = std::max(std::max(msh1.m, msh2.m), 88.f);
    if (ratio < 0.5) {
      msh2 = Msh(mMid, 0.0, 0.0);
      ratio = 2.0 * ratio;
    } else {
      msh1 = Msh(mMid, 0.0, 0.0);
      ratio = 2.0 * ratio - 1.0;
    }
  }

  // Adjust unsaturated hue
  if (msh1.s < minSat && msh2.s > minSat) {
    msh1.h = mshAdjustHue(msh2, msh1.m);
  } else if (msh2.s < minSat && msh1.s > minSat) {
    msh2.h = mshAdjustHue(msh1, msh2.m);
  }

  // Interpolate linearly in MSH space
  Msh mshInterpolated = lerp(msh1, msh2, ratio);

  // Convert color back to RGB
  return Rgb(Xyz(Lab(mshInterpolated))).toColor();
}

}  // namespace vx
