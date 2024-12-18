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

#include <VoxieClient/Map.hpp>
#include <VoxieClient/Quaternion.hpp>
#include <VoxieClient/Vector.hpp>
#include <VoxieClient/VoxieClient.hpp>

#include <cmath>

namespace vx {
// Note: Currently only 3D rotations are supported

template <typename T, std::size_t dim>
class Rotation;

namespace intern {
VOXIECLIENT_EXPORT void throwQuaternionLengthOutOfRange(double lenSq);
}

template <typename T>
class Rotation<T, 3> {
  static_assert(!std::numeric_limits<T>::is_integer,
                "Rotations will not work with integer types");

  Quaternion<T> quaternion_;

 public:
  explicit Rotation(const Quaternion<T>& quaternion) : quaternion_(quaternion) {
    T lenSq = squaredNorm(quaternion);
    // Allow a lot of values because normalization of small vectors currently
    // does not work very well (e.g. in fromAxisAndAngle())
    // if (lenSq < 0.98 || lenSq > 1.02)
    if (lenSq < 0.90 || lenSq > 1.10)
      vx::intern::throwQuaternionLengthOutOfRange(lenSq);
    using std::sqrt;
    quaternion_ /= sqrt(lenSq);
  }

  // Convert to rotation matrix
  // https://en.wikipedia.org/w/index.php?title=Rotation_matrix&oldid=1033792727#Quaternion
  vx::Matrix<T, 3> asRotationMatrix() const {
    auto w = asQuaternion().a();
    auto x = asQuaternion().b();
    auto y = asQuaternion().c();
    auto z = asQuaternion().d();
    return {
        {1 - 2 * y * y - 2 * z * z, 2 * x * y - 2 * z * w,
         2 * x * z + 2 * y * w},
        {2 * x * y + 2 * z * w, 1 - 2 * x * x - 2 * z * z,
         2 * y * z - 2 * x * w},
        {2 * x * z - 2 * y * w, 2 * y * z + 2 * x * w,
         1 - 2 * x * x - 2 * y * y},
    };
  }
  operator LinearMap<T, 3>() const {
    return createLinearMap(asRotationMatrix());
  }
  operator AffineMap<T, 3>() const {
    return createLinearMap(asRotationMatrix());
  }
  operator ProjectiveMap<T, 3>() const {
    return createLinearMap(asRotationMatrix());
  }

  const Quaternion<T>& asQuaternion() const { return this->quaternion_; }
  Quaternion<T>& asQuaternion() { return this->quaternion_; }

  vx::Vector<T, 3> map(const vx::Vector<T, 3>& v) const {
    Quaternion<T> res = asQuaternion() * Quaternion<T>(0, v[0], v[1], v[2]) *
                        asQuaternion().conjugate();
    return vx::Vector<T, 3>{res.b(), res.c(), res.d()};
  }
  vx::Vector<T, 3> mapVector(const vx::Vector<T, 3>& v) const {
    // Rotation is linear, so mapVector() does the same as map()
    return map(v);
  }

  friend Rotation operator*(const Rotation& rot1, const Rotation& rot2) {
    return Rotation(rot1.asQuaternion() * rot2.asQuaternion());
  }
  friend Rotation& operator*=(Rotation& rot, const Rotation& rot2) {
    return rot = rot * rot2;
  }

 private:
  LinearMap<T, 3> asMap() const { return *this; }
  AffineMap<T, 3> asAMap() const { return *this; }
  ProjectiveMap<T, 3> asPMap() const { return *this; }

 public:
  // Needed to avoid casts
  friend LinearMap<T, 3> operator*(const Rotation& v1,
                                   const LinearMap<T, 3>& v2) {
    return v1.asMap() * v2;
  }
  friend LinearMap<T, 3> operator*(const LinearMap<T, 3>& v1,
                                   const Rotation& v2) {
    return v1 * v2.asMap();
  }
  friend LinearMap<T, 3>& operator*=(LinearMap<T, 3>& v1, const Rotation& v2) {
    return v1 *= v2.asMap();
  }

  // TODO: Allow LinearMap<> * AffineMap<> etc.?
  friend AffineMap<T, 3> operator*(const Rotation& v1,
                                   const AffineMap<T, 3>& v2) {
    return v1.asAMap() * v2;
  }
  friend AffineMap<T, 3> operator*(const AffineMap<T, 3>& v1,
                                   const Rotation& v2) {
    return v1 * v2.asAMap();
  }
  friend AffineMap<T, 3>& operator*=(AffineMap<T, 3>& v1, const Rotation& v2) {
    return v1 *= v2.asAMap();
  }

  friend ProjectiveMap<T, 3> operator*(const Rotation& v1,
                                       const ProjectiveMap<T, 3>& v2) {
    return v1.asPMap() * v2;
  }
  friend ProjectiveMap<T, 3> operator*(const ProjectiveMap<T, 3>& v1,
                                       const Rotation& v2) {
    return v1 * v2.asPMap();
  }
  friend ProjectiveMap<T, 3>& operator*=(ProjectiveMap<T, 3>& v1,
                                         const Rotation& v2) {
    return v1 *= v2.asPMap();
  }

  // The angle of the rotation in radians
  T angle() const {
    // Make sure rounding errors (e.g. -1.0001 or 1.0001) don't cause problems
    if (asQuaternion().a() < -1 || asQuaternion().a() > 1)
      return 0;
    else
      return 2 * std::acos(asQuaternion().a());
  }
  T angleDeg() const {
    T pi = std::acos((T)-1);
    return angle() / pi * 180;
  }

  void toZYZ(T& alpha, T& beta, T& gamma) const {
    T pi = std::acos((T)-1);
    const auto& q = asQuaternion();

    // Some rotation matrix elements
    // (m11 m21 m31)
    // (m12 m22 m32)
    // (m13 m23 m33)
    T m33 = 1 - 2 * q.b() * q.b() - 2 * q.c() * q.c();
    // TODO: Should this depend on T?
    T eps = static_cast<T>(1e-6l);  // = 89.918435 deg
    if (m33 > 1 - eps) {            // m33 = cos b ~ 1
      T m11 = 1 - 2 * q.c() * q.c() -
              2 * q.d() * q.d();  // = cos a cos b cos g - sin a sin g = cos
                                  // a cos g - sin a sin g = cos (a + g)
      T m12 = 2 * q.b() * q.c() +
              2 * q.a() * q.d();     // = cos a sin g + sin a cos b cos g = cos
                                     // a sin g + sin a cos g = sin (a + g)
      alpha = std::atan2(m12, m11);  // a + g
      beta = 0;
      gamma = 0;
    } else if (m33 < -1 + eps) {  // m33 = cos b ~ -1
      T m11 = 1 - 2 * q.c() * q.c() -
              2 * q.d() * q.d();  // = cos a cos b cos g - sin a sin g = - cos a
                                  // cos g - sin a sin g = - cos (a - g)
      T m12 = 2 * q.b() * q.c() +
              2 * q.a() * q.d();  // = cos a sin g + sin a cos b cos g = cos
                                  // a sin g - sin a cos g = - sin (a - g)
      alpha = std::atan2(-m12, -m11);  // a - g
      beta = pi;
      gamma = 0;
    } else {
      T m13 = 2 * q.b() * q.d() - 2 * q.a() * q.c();
      T m23 = 2 * q.c() * q.d() + 2 * q.a() * q.b();
      T m31 = 2 * q.b() * q.d() + 2 * q.a() * q.c();
      T m32 = 2 * q.c() * q.d() - 2 * q.a() * q.b();
      alpha = std::atan2(m32, m31);
      beta = std::acos(m33);
      gamma = std::atan2(m23, -m13);
    }
  }
  void toZYZDeg(T& alpha, T& beta, T& gamma) const {
    T pi = std::acos((T)-1);
    toZYZ(alpha, beta, gamma);
    alpha *= 180 / pi;
    beta *= 180 / pi;
    gamma *= 180 / pi;
  }

  friend std::ostream& operator<<(std::ostream& o, const Rotation& m) {
    return o << "Rotation" << m.asQuaternion();
  }

#if VX_VECTOR_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const Rotation& m) {
    QDebugStateSaver saver(debug);
    return debug.nospace() << "Rotation" << m.asQuaternion();
  }
#endif
};

template <typename T>
inline Rotation<T, 3> identityRotation() {
  return Rotation<T, 3>(Quaternion<T>::one());
}

template <typename To, typename From>
inline std::enable_if_t<IsConvertibleWithoutNarrowing<From, To>::value,
                        Rotation<To, 3>>
rotationCast(const Rotation<From, 3>& rot) {
  return Rotation<To, 3>(quaternionCast<To>(rot.asQuaternion()));
}

template <typename To, typename From>
inline std::enable_if_t<std::is_convertible<From, To>::value, Rotation<To, 3>>
rotationCastNarrow(const Rotation<From, 3>& rot) {
  return Rotation<To, 3>(quaternionCastNarrow<To>(rot.asQuaternion()));
}

template <typename T>
inline Rotation<T, 3> inverse(const Rotation<T, 3>& rot) {
  return Rotation<T, 3>(rot.asQuaternion().conjugate());
}

template <typename T>
inline Rotation<T, 3> rotationFromAxisAngle(const vx::Vector<T, 3>& axis,
                                            T rad) {
  using std::cos;
  using std::sin;
  using std::sqrt;

  T axisNorm = sqrt(squaredNorm(axis));
  if (axisNorm == 0) {
    // Zero-length axis. (Or axis length close enought to zero that the squared
    // values is rounded to zero). This should only happen for rad == 0.
    // Return an identity rotation even if rad != 0, as there is no proper value
    // to return in this case.
    return identityRotation<T>();
  }
  // TODO: Check what happens if axisNorm is very close to 0 (might return a
  // non-normalized vector in this case and the Rotation constructor below might
  // throw)
  auto axisNormalized = axis / axisNorm;

  T sinVal = sin(rad / 2);
  T cosVal = cos(rad / 2);
  auto axis2 = sinVal * axisNormalized;
  // qDebug() << "Normalized" << squaredNorm(axisNormalized);
  return Rotation<T, 3>(Quaternion<T>(cosVal, axis2[0], axis2[1], axis2[2]));
}
template <typename T>
inline Rotation<T, 3> rotationFromAxisAngleDeg(const vx::Vector<T, 3>& axis,
                                               T deg) {
  T pi = std::acos((T)-1);
  return rotationFromAxisAngle(axis, deg / 180 * pi);
}
template <typename T>
inline Rotation<T, 3> rotationFromZYZ(T alpha, T beta, T gamma) {
  vx::Vector<T, 3> yAxis = {0, 1, 0};
  vx::Vector<T, 3> zAxis = {0, 0, 1};
  return rotationFromAxisAngle(zAxis, alpha) *
         rotationFromAxisAngle(yAxis, beta) *
         rotationFromAxisAngle(zAxis, gamma);
}
template <typename T>
inline Rotation<T, 3> rotationFromZYZDeg(T alpha, T beta, T gamma) {
  T pi = std::acos((T)-1);
  return rotationFromZYZ(alpha / 180 * pi, beta / 180 * pi, gamma / 180 * pi);
}

// The angle between rotations rot1 and rot2 in radians
template <typename T>
inline T angularDifference(const Rotation<T, 3>& rot1,
                           const Rotation<T, 3>& rot2) {
  return (inverse(rot2) * rot1).angle();
}
}  // namespace vx
