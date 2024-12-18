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

#include <VoxieClient/CastFloatToIntSafe.hpp>
#include <VoxieClient/Optional.hpp>
#include <VoxieClient/Vector.hpp>

// https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// https://www.codingwiththomas.com/blog/c-static-dynamic-polymorphism-crtp-and-c20s-concepts
// https://www.delftstack.com/howto/cpp/static-polymorphism-cpp/

#define VX_STATIC_POLYMORPHISM_CAST static_cast
// #define VX_STATIC_POLYMORPHISM_CAST reinterpret_cast

// Methods marked with this attribute should be overwritten in a subclass and
// because they are called using self() (which returns a pointer to the concrete
// type), they should never be called directly.
#define VX_STATIC_POLYMORPHISM_ABSTRACT \
  [[gnu::error("Call to abstract method")]]

#define VX_STATIC_POLYMORPHISM_CLASS                               \
 private:                                                          \
  ConcreteType* self() {                                           \
    return VX_STATIC_POLYMORPHISM_CAST<ConcreteType*>(this);       \
  }                                                                \
  const ConcreteType* self() const {                               \
    return VX_STATIC_POLYMORPHISM_CAST<const ConcreteType*>(this); \
  }

#define VX_STATIC_POLYMORPHISM_CLASS_CONCRETE(Ty) \
 private:                                         \
  Ty* self() { return this; }                     \
  const Ty* self() const { return this; }

namespace vx {
// Note: This classes are abstract classes, but they do not use virtual methods
// for performance reasons. Instead, method resolution will be done statically.
// This class provides read access to a voxel volume.
template <typename ConcreteType, typename T>
class VoxelAccessor {
  VX_STATIC_POLYMORPHISM_CLASS

 public:
  using DataType = T;

  // Return the voxel if pos is in the volume and nullopt otherwise.
  Optional<DataType> getVoxelChecked(const vx::Vector<size_t, 3>& pos) const {
    for (size_t i = 0; i < 3; i++) {
      if (pos[i] >= self()->arrayShape()[i]) return vx::nullopt;
    }
    return self()->getVoxelUnchecked(pos);
  }

  // Same, but accept signed argument (negative values are always outside the
  // volume)
  // TODO: Would it be faster and also correct to just cast to size_t and call
  // th version above?
  Optional<DataType> getVoxelChecked(
      const vx::Vector<ptrdiff_t, 3>& pos) const {
    for (size_t i = 0; i < 3; i++) {
      if (pos[i] < 0 || (size_t)pos[i] >= self()->arrayShape()[i])
        return vx::nullopt;
    }
    return self()->getVoxelUnchecked(vectorCastNarrow<size_t>(pos));
  }

  // Return the voxel if pos is in the volume and has undefined behavior
  // otherwise.
  VX_STATIC_POLYMORPHISM_ABSTRACT
  T getVoxelUnchecked(const vx::Vector<size_t, 3>& pos) const;
};

template <typename ConcreteType, typename T>
class InterpolatedVoxelAccessor {
  VX_STATIC_POLYMORPHISM_CLASS

 public:
  using DataType = T;

  // Return the voxel if pos is in the volume and nullopt otherwise.
  // For linear interpolation it is enough if part pos touches part of a voxel.
  VX_STATIC_POLYMORPHISM_ABSTRACT
  Optional<T> getVoxelInterpolatedVoxel(const vx::Vector<double, 3>& pos) const;

  // Same as getVoxelInterpolatedVoxel, but coordinate are in m.
  Optional<DataType> getVoxelInterpolatedObject(
      const vx::Vector<double, 3>& pos) const {
    // Transform coordinates from object to voxel coordinate system
    return self()->getVoxelInterpolatedVoxel(elementwiseDivision(
        pos - self()->volumeOrigin(), self()->gridSpacing()));
  }
};

template <typename ConcreteType, typename Src,
          typename T = typename Src::DataType>
class VoxelAccessorForward : public VoxelAccessor<ConcreteType, T> {
  VX_STATIC_POLYMORPHISM_CLASS

 protected:
  Src src;

 public:
  VoxelAccessorForward(Src src) : src(std::move(src)) {}

  // TODO: Return references here or values?
  const vx::Vector<size_t, 3>& arrayShape() const { return src.arrayShape(); }
  const vx::Vector<double, 3>& volumeOrigin() const {
    return src.volumeOrigin();
  }
  const vx::Vector<double, 3>& gridSpacing() const { return src.gridSpacing(); }
};

template <typename ConcreteType, typename Src>
class InterpolatedVoxelAccessorForward
    : public InterpolatedVoxelAccessor<ConcreteType, typename Src::DataType> {
  VX_STATIC_POLYMORPHISM_CLASS

 protected:
  Src src;

 public:
  InterpolatedVoxelAccessorForward(Src src) : src(std::move(src)) {}

  const vx::Vector<size_t, 3>& arrayShape() const { return src.arrayShape(); }
  const vx::Vector<double, 3>& volumeOrigin() const {
    return src.volumeOrigin();
  }
  const vx::Vector<double, 3>& gridSpacing() const { return src.gridSpacing(); }
};

template <typename DstType, typename Src>
// Implements VoxelAccessor<DstType>
class ConvertedVoxelAccessor
    : public VoxelAccessorForward<ConvertedVoxelAccessor<DstType, Src>, Src,
                                  DstType> {
  VX_STATIC_POLYMORPHISM_CLASS_CONCRETE(ConvertedVoxelAccessor)

 public:
  using VoxelAccessorForward<ConvertedVoxelAccessor, Src,
                             DstType>::VoxelAccessorForward;

  DstType getVoxelUnchecked(const vx::Vector<size_t, 3>& pos) const {
    return static_cast<DstType>(self()->src.getVoxelUnchecked(pos));
  }
};
template <typename DstType, typename Src>
ConvertedVoxelAccessor<DstType, Src> convertedVoxelAccessor(Src src) {
  return ConvertedVoxelAccessor<DstType, Src>(std::move(src));
}

template <typename Src>
// Implements InterpolatedVoxelAccessor<Src::DataType>
class NearestInterpolation
    : public InterpolatedVoxelAccessorForward<NearestInterpolation<Src>, Src> {
  VX_STATIC_POLYMORPHISM_CLASS_CONCRETE(NearestInterpolation)

 public:
  using InterpolatedVoxelAccessorForward<NearestInterpolation,
                                         Src>::InterpolatedVoxelAccessorForward;
  using DataType =
      typename InterpolatedVoxelAccessorForward<NearestInterpolation,
                                                Src>::DataType;

  Optional<DataType> getVoxelInterpolatedVoxel(
      const vx::Vector<double, 3>& pos) const {
    vx::Vector<size_t, 3> posInt;
    for (std::size_t i = 0; i < 3; i++) {
      auto asInt = castFloatToIntSafe<qint64>(std::floor(pos[i]));
      if (!asInt.has_value() || asInt.value() < 0 ||
          // (quint64)asInt.value() > std::numeric_limits<std::size_t>::max()
          (quint64)asInt.value() >= self()->arrayShape()[i])
        return vx::nullopt;
      posInt[i] = static_cast<std::size_t>(asInt.value());
    }

    // return self()->src.getVoxelChecked(posInt);
    return self()->src.getVoxelUnchecked(posInt);
  }
};
template <typename Src>
NearestInterpolation<Src> nearestInterpolation(Src src) {
  return NearestInterpolation<Src>(std::move(src));
}

// Note: Src::DataType should normally be a float type.
template <typename Src>
// Implements InterpolatedVoxelAccessor<Src::DataType>
class LinearInterpolation
    : public InterpolatedVoxelAccessorForward<LinearInterpolation<Src>, Src> {
  VX_STATIC_POLYMORPHISM_CLASS_CONCRETE(LinearInterpolation)

 public:
  using InterpolatedVoxelAccessorForward<LinearInterpolation,
                                         Src>::InterpolatedVoxelAccessorForward;
  using DataType =
      typename InterpolatedVoxelAccessorForward<LinearInterpolation,
                                                Src>::DataType;

  Optional<DataType> getVoxelInterpolatedVoxel(
      const vx::Vector<double, 3>& pos0) const {
    auto pos = pos0 - vx::Vector<double, 3>{0.5, 0.5, 0.5};

    vx::Vector<ptrdiff_t, 3> posInt;
    for (std::size_t i = 0; i < 3; i++) {
      auto asInt = castFloatToIntSafe<qint64>(std::floor(pos[i]));
      // When there was an overflow during conversion or all used voxels are out
      // of range, return NaN
      if (!asInt.has_value() || asInt.value() < -1 ||
          //(quint64)asInt.value() > std::numeric_limits<std::size_t>::max()
          asInt.value() >= (qint64)self()->arrayShape()[i])
        return vx::nullopt;
      posInt[i] = static_cast<std::size_t>(asInt.value());
    }

    // Interpolation coefficients
    vx::Vector<double, 3> k = pos - vectorCastNarrow<double>(posInt);
    vx::Vector<double, 3> kN = vx::Vector<double, 3>{1, 1, 1} - k;

    // Use defVal = 0 to make sure a proper value is returned if the position is
    // at least one of the 8 used voxels is in the volume
    DataType defVal = static_cast<DataType>(0);
    return k[0] * k[1] * k[2] *
               self()
                   ->src
                   .getVoxelChecked(posInt + vx::Vector<ptrdiff_t, 3>{1, 1, 1})
                   .value_or(defVal) +
           k[0] * k[1] * kN[2] *
               self()
                   ->src
                   .getVoxelChecked(posInt + vx::Vector<ptrdiff_t, 3>{1, 1, 0})
                   .value_or(defVal) +
           k[0] * kN[1] * k[2] *
               self()
                   ->src
                   .getVoxelChecked(posInt + vx::Vector<ptrdiff_t, 3>{1, 0, 1})
                   .value_or(defVal) +
           k[0] * kN[1] * kN[2] *
               self()
                   ->src
                   .getVoxelChecked(posInt + vx::Vector<ptrdiff_t, 3>{1, 0, 0})
                   .value_or(defVal) +
           kN[0] * k[1] * k[2] *
               self()
                   ->src
                   .getVoxelChecked(posInt + vx::Vector<ptrdiff_t, 3>{0, 1, 1})
                   .value_or(defVal) +
           kN[0] * k[1] * kN[2] *
               self()
                   ->src
                   .getVoxelChecked(posInt + vx::Vector<ptrdiff_t, 3>{0, 1, 0})
                   .value_or(defVal) +
           kN[0] * kN[1] * k[2] *
               self()
                   ->src
                   .getVoxelChecked(posInt + vx::Vector<ptrdiff_t, 3>{0, 0, 1})
                   .value_or(defVal) +
           kN[0] * kN[1] * kN[2] *
               self()
                   ->src
                   .getVoxelChecked(posInt + vx::Vector<ptrdiff_t, 3>{0, 0, 0})
                   .value_or(defVal);
  }
};
template <typename Src>
LinearInterpolation<Src> linearInterpolation(Src src) {
  return LinearInterpolation<Src>(std::move(src));
}

}  // namespace vx
