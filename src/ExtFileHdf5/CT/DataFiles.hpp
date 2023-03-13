/*
 * Copyright (c) 2013 Steffen Kieß
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

#ifndef CT_DATAFILES_HPP
#define CT_DATAFILES_HPP

#include <Math/Vector3.hpp>
#include <Math/DiagMatrix3.hpp>
#include <Math/Float.hpp>

#include <HDF5/Matlab.hpp>
#include <HDF5/MatlabVector2.hpp>
#include <HDF5/MatlabVector3.hpp>
#include <HDF5/MatlabDiagMatrix3.hpp>
#include <HDF5/DelayedArray.hpp>
#include <HDF5/Array.hpp>

#include <boost/scoped_ptr.hpp>

#include <half.hpp>

template <typename T, bool delayLoading = false>
struct VolumeGen;

template <typename T, bool delayLoading>
struct VolumeGen {
  // TODO: make data type dynamic?
  typedef typename boost::mpl::if_c<delayLoading, HDF5::DelayedArray<T, 3>, Math::Array<T, 3> >::type ArrayType;

  boost::optional<std::string> Type; // = "Volume"

  // Distance between two points in X/Y/Z-direction, in m
  boost::optional<Math::DiagMatrix3<ldouble> > GridSpacing;

  // Position of the first grid cell in m
  boost::optional<Math::Vector3<ldouble> > GridOrigin;

  // On loading, all values are multiplied by this value (defaults to 1)
  boost::optional<ldouble> VolumeScalingFactor;

  // VolumeStorageOrder shows how the dimensions of the data are stored.
  // The first value shows where the X dimension is stored:
  //   1: Stored in the first HDF5 dimension (i.e. the one with the largest stride)
  //  -1: Stored in the first HDF5 dimension, but mirrored
  //   2: Stored in the second HDF5 dimension
  //  -2: Stored in the second HDF5 dimension, but mirrored
  //   3: Stored in the third HDF5 dimension (i.e. the one with the smallest stride)
  //  -3: Stored in the third HDF5 dimension, but mirrored
  // The second value contains the same information for the Y dimension, the third value for the Z dimension
  // The default is "3 2 1"
  boost::optional<std::vector<int32_t> > VolumeStorageOrder;

  ArrayType Volume;

  Math::ArrayView<const T, 3> transposeVolume (const Math::ArrayView<const T, 3>& view) const {
    if (VolumeStorageOrder) {
      std::vector<int32_t> swapDim13;
      swapDim13.push_back (3);
      swapDim13.push_back (2);
      swapDim13.push_back (1);
      return Math::reorderDimensions (Math::reorderDimensions (view, swapDim13), *VolumeStorageOrder);
    } else {
      return view;
    }
  }

  void scaleValues (const Math::ArrayView<T, 3>& values) const {
    if (!VolumeScalingFactor)
      return;
    T factor = (T) *VolumeScalingFactor;
    for (size_t z = 0; z < values.template size<2> (); z++)
      for (size_t y = 0; y < values.template size<1> (); y++)
        for (size_t x = 0; x < values.template size<0> (); x++)
          values (x, y, z) *= factor;
  }

#define MEMBERS(m)                              \
  m (Type)                                      \
  m (GridSpacing)                               \
  m (GridOrigin)                                \
  m (VolumeScalingFactor)                       \
  m (VolumeStorageOrder)                        \
  m (Volume)
  HDF5_MATLAB_DECLARE_TYPE (VolumeGen, MEMBERS)
#undef MEMBERS
};

template <typename T>
Math::ArrayView<const T, 3> transposedUnscaledVolume (const VolumeGen<T>& self) {
  return self.transposeVolume (self.Volume.view ());
}
namespace DataFilesIntern {
  struct NoopDeallocator {
    template <typename U>
    void operator() (UNUSED const U& val) {
    }
  };
}
// Rather slow (will transpose volume in memory)
template <typename T>
std::shared_ptr<const Math::Array<T, 3> > transformedTransposedVolume (const VolumeGen<T>& self, bool forceCopy = false) {
  if (forceCopy || self.VolumeScalingFactor || self.VolumeStorageOrder) {
    std::shared_ptr<Math::Array<T, 3> > copyPtr = std::make_shared<Math::Array<T, 3> > (transposedUnscaledVolume (self));
    scaleValues (copyPtr->view ());
    return copyPtr;
  } else {
    return std::shared_ptr<const Math::Array<T, 3> > (&self.Volume, DataFilesIntern::NoopDeallocator ());
  }
}
template <typename T>
std::shared_ptr<const Math::ArrayView<const T, 3> > transformedVolume (const VolumeGen<T>& self, bool forceCopy = false) {
  if (forceCopy || self.VolumeScalingFactor) {
    //std::shared_ptr<Math::Array<T, 3> > copyPtr = std::make_shared<Math::Array<T, 3> > (Volume);
    std::shared_ptr<std::pair<Math::Array<T, 3>, boost::scoped_ptr<Math::ArrayView<const T, 3> > > > copyPtr = std::make_shared<std::pair<Math::Array<T, 3>, boost::scoped_ptr<Math::ArrayView<const T, 3> > > > ();
    copyPtr->first.recreate (self.Volume);
    self.scaleValues (copyPtr->first);
    copyPtr->second.reset (new Math::ArrayView<const T, 3> (self.transposeVolume (copyPtr->first.view ())));
    //return std::shared_ptr<const Math::ArrayView<const T, 3> > (copyPtr, &copyPtr->view ()); // Only works if Array::view() returns a reference
    return std::shared_ptr<const Math::ArrayView<const T, 3> > (copyPtr, copyPtr->second.get ());
  } else {
    return std::make_shared<const Math::ArrayView<const T, 3> > (transposedUnscaledVolume (self));
  }
}

template <typename T>
std::shared_ptr<VolumeGen<T> > load (const VolumeGen<T, true>& self) {
  std::shared_ptr<VolumeGen<T> > data = std::make_shared<VolumeGen<T> > ();
  data->Type = self.Type;
  data->GridSpacing = self.GridSpacing;
  data->GridOrigin = self.GridOrigin;
  data->VolumeScalingFactor = self.VolumeScalingFactor;
  data->VolumeStorageOrder = self.VolumeStorageOrder;
  data->Volume.recreate (self.Volume.size);
  self.Volume.read (data->Volume.view ());
  return data;
}
template <typename T>
Math::Vector3<size_t> getSize (const VolumeGen<T, true>& self) {
  Math::Vector3<size_t> s (self.Volume.size[0], self.Volume.size[1], self.Volume.size[2]);
  Math::Vector3<size_t> s2;
  if (!self.VolumeStorageOrder) {
    s2 = s;
  } else {
    for (size_t i = 0; i < 3; i++) {
      int32_t val = (*self.VolumeStorageOrder)[i];
      if (val < 0)
        val = -val;
      ASSERT (val > 0 && val <= 3);
      val = val - 1;
      val = 2 - val;
      s2[i] = s[val];
    }
  }
  return s2;
}
template <typename BufferType, typename T, typename Callback>
void loadAndTransformTo (const VolumeGen<T, true>& self, const Math::ArrayView<T, 3>& view, Callback callback) {
  static const bool assertions = false;

  std::vector<int32_t> swapDim13;
  swapDim13.push_back (3);
  swapDim13.push_back (2);
  swapDim13.push_back (1);

  Math::ArrayView<T, 3, false, Math::ArrayConfig, Math::ArrayAssertions<assertions> > transformedView = Math::reorderDimensions (view, self.VolumeStorageOrder ? *self.VolumeStorageOrder : swapDim13, true);

  std::size_t sizeX = transformedView.template size<0> ();
  std::size_t sizeY = transformedView.template size<1> ();
  std::size_t sizeZ = transformedView.template size<2> ();

  ASSERT (sizeX == self.Volume.size[2]);
  ASSERT (sizeY == self.Volume.size[1]);
  ASSERT (sizeZ == self.Volume.size[0]);

  bool useDirectLoad =
    !self.VolumeScalingFactor
    && transformedView.template strideBytes<0> () == (ptrdiff_t) (sizeof (T) * self.Volume.size[0] * self.Volume.size[1])
    && transformedView.template strideBytes<1> () == (ptrdiff_t) (sizeof (T) * self.Volume.size[0])
    && transformedView.template strideBytes<2> () == (ptrdiff_t) (sizeof (T));
  useDirectLoad = false; // Disable direct loads because of HDFFV-9634
  //QDateTime start = QDateTime::currentDateTimeUtc ();
  if (useDirectLoad) {
    //std::cerr << "DIRECT LOAD" << std::endl;
    self.Volume.read (Math::reorderDimensions (transformedView, swapDim13));
  } else {
    //std::cerr << "BUFFERED LOAD" << std::endl;
    std::size_t sliceSizeBytes = sizeZ * sizeY * sizeof (T);
    std::size_t maxBufferSize = 1 * 1024 * 1024; // 1MB
    std::size_t slices = maxBufferSize / sliceSizeBytes;
    if (slices == 0)
      slices = 1;
    std::size_t bufferShape[3] = { slices, sizeY, sizeZ };
    Math::Array<BufferType, 3> buffer (bufferShape, false);
    Math::ArrayView<BufferType, 3, false, Math::ArrayConfig, Math::ArrayAssertions<assertions> > bufferView (buffer.view ());
    HDF5::DataSpace memDataSpace = HDF5::DataSpace::createSimple (slices, sizeY, sizeZ);
    HDF5::DataSpace fileDataSpace = HDF5::DataSpace::createSimple (sizeX, sizeY, sizeZ);

    BufferType scalingFactor = self.VolumeScalingFactor ? (BufferType)(*self.VolumeScalingFactor) : (BufferType)1;

    for (std::size_t firstSlice = 0; firstSlice < sizeX; firstSlice += slices) {
      callback(firstSlice, sizeX);
      std::size_t slicesCur = slices;
      if (firstSlice + slices > sizeX) {
        slicesCur = sizeX - firstSlice;
        memDataSpace = HDF5::DataSpace::createSimple (slicesCur, sizeY, sizeZ);
      }

      hsize_t start[3] = { firstSlice, 0, 0 };
      hsize_t count[3] = { slicesCur, sizeY, sizeZ };
      fileDataSpace.selectHyperslab (H5S_SELECT_SET, count, start);

      self.Volume.dataSet.read (buffer.data (), HDF5::getH5Type<BufferType> (), memDataSpace, fileDataSpace);

      if (self.VolumeScalingFactor) {
        for (std::size_t x = 0; x < slicesCur; x++)
          for (std::size_t y = 0; y < sizeY; y++)
            for (std::size_t z = 0; z < sizeZ; z++)
              transformedView (firstSlice + x, y, z) = bufferView (x, y, z) * scalingFactor;
      } else {
        for (std::size_t x = 0; x < slicesCur; x++)
          for (std::size_t y = 0; y < sizeY; y++)
            for (std::size_t z = 0; z < sizeZ; z++)
              transformedView (firstSlice + x, y, z) = bufferView (x, y, z);
      }
    }
    callback(sizeX, sizeX);
  }
  //QDateTime end = QDateTime::currentDateTimeUtc ();
  //qDebug() << (end.toMSecsSinceEpoch() - start.toMSecsSinceEpoch());
}
static inline void callbackDoNothing(UNUSED size_t pos, UNUSED size_t count) {}
template <typename BufferType, typename T>
void loadAndTransformTo (const VolumeGen<T, true>& self, const Math::ArrayView<T, 3>& view) {
  loadAndTransformTo<BufferType, T>(self, view, callbackDoNothing);
}

#endif // !CT_DATAFILES_HPP
