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

#ifndef RAWDATAFILES_H
#define RAWDATAFILES_H


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

#include <QVector>

template <typename T, bool delayLoading = false>
struct RawGen;

template <typename T, bool delayLoading>
struct RawGen {
    // TODO: make data type dynamic?
    typedef typename boost::mpl::if_c<delayLoading, HDF5::DelayedArray<T, 3>, Math::Array<T, 3> >::type ArrayType;

    boost::optional<std::string> Type; // = "Varies"

    ArrayType Image;

    boost::optional<std::vector<double>> Angle;

    boost::optional<double> DetectorPixelSizeX;

    boost::optional<double> DetectorPixelSizeY;

    boost::optional<double> DistanceSourceAxis;

    boost::optional<double> DistanceSourceDetector;

    boost::optional<std::string> Dimension;

    // TODO: Use ObjectPosition and ObjectRotation?

    boost::optional<std::vector<Math::Vector3<double>>>
        ObjectPosition;  // Position of object in rotated coordinate system

    // boost::optional<std::vector<Math::Quaternion<double>>>
    //    ObjectRotation;  // Rotation of object grid in world coordinate system

    boost::optional<bool> MirroredYAxis;

#define MEMBERS(m)   \
    m (Image)           \
    m (Angle)   \
    m (DetectorPixelSizeX) \
    m (DetectorPixelSizeY) \
    m (DistanceSourceAxis) \
    m (DistanceSourceDetector) \
    m (Dimension) \
    m (Type) \
    m (ObjectPosition) \
    /* m (ObjectRotation) */ \
    m (MirroredYAxis)
    HDF5_MATLAB_DECLARE_TYPE (RawGen, MEMBERS)
#undef MEMBERS
};



template <typename T>
std::shared_ptr<RawGen<T,true> > loadTo (const RawGen<T, true>& self) {
    std::shared_ptr<RawGen<T,true> > data = std::make_shared<RawGen<T,true> > ();
    data->Type = self.Type;
    //  data->Angle = self.Angle;
    //  data->DetectorPixelSizeX = self.DetectorPixelSizeX;
    //  data->DetectorPixelSizeY = self.DetectorPixelSizeY;
    //  data->DistanceSourceAxis = self.DistanceSourceAxis;
    //  data->DistanceSourceDetector = self.DistanceSourceDetector;
    data->Image.recreate (self.Image.size);
    self.Image.read (maybeMirrorYAxis(data->Image.view (), self.MirroredYAxis));
    return data;
}
template <typename T>
Math::Vector3<size_t> getSize (const RawGen<T, true>& self) {
    Math::Vector3<size_t> s (self.Image.size[0], self.Image.size[1], self.Image.size[2]);
    return s;
}

template <typename T2, typename Config, typename Assert>
static Math::ArrayView<T2, 3, false, Config, Assert> mirrorY(
    const Math::ArrayView<T2, 3, false, Config, Assert>& view) {
  if (view.template size<1>() == 0) return view;
  typename Config::ArithmeticPointer ptr = view.arithData();
  ptr += (view.template size<1>() - 1) * view.template strideBytes<1>();
  std::ptrdiff_t stridesBytes[] = {view.template strideBytes<0>(),
                                   -view.template strideBytes<1>(),
                                   view.template strideBytes<2>()};
  return Math::ArrayView<T2, 3, false, Config, Assert>(
      Config::template Type<T2>::fromArith(ptr), view.shape(), stridesBytes);
}

template <typename T, typename Config, typename Assert>
static Math::ArrayView<T, 3, false, Config, Assert> maybeMirrorYAxis(
    const Math::ArrayView<T, 3, false, Config, Assert>& view,
    const boost::optional<bool>& MirroredYAxis) {
  if (MirroredYAxis && *MirroredYAxis) {
    // qDebug() << "MIRROR";
    return mirrorY(view);
  } else {
    // qDebug() << "NOMIRROR";
    return view;
  }
}

template <typename BufferType, typename T, typename Callback>
void loadAndTransformTo (const RawGen<T, true>& self, const Math::ArrayView<T, 3>& view, size_t initAmount, Callback callback) {
    static const bool assertions = false;
    std::vector<int32_t> swapDim13;
    swapDim13.push_back (3);
    swapDim13.push_back (2);
    swapDim13.push_back (1);

    Math::ArrayView<T, 3, false, Math::ArrayConfig, Math::ArrayAssertions<assertions> > transformedView = Math::reorderDimensions (view, swapDim13, true);

    std::size_t sizeX = transformedView.template size<0> ();
    std::size_t sizeY = transformedView.template size<1> ();
    std::size_t sizeZ = transformedView.template size<2> ();

    //qDebug() << transformedView.template size<0>() << transformedView.template size<1>() << transformedView.template size<2>();

    ASSERT (sizeX == self.Image.size[2]);
    ASSERT (sizeY == self.Image.size[1]);
    ASSERT (sizeZ == self.Image.size[0]);

    bool useDirectLoad = false; // Disable direct loads because of HDFFV-9634
    //QDateTime start = QDateTime::currentDateTimeUtc ();
    if (useDirectLoad) {
        //std::cerr << "DIRECT LOAD" << std::endl;
        self.Image.read(maybeMirrorYAxis(
            Math::reorderDimensions(transformedView, swapDim13),
            self.MirroredYAxis));
    } else {
        //std::cerr << "BUFFERED LOAD" << std::endl;
        //std::size_t sliceSizeBytes = sizeZ * sizeY * sizeof (T);
        //std::size_t maxBufferSize = 1 * 1024 * 1024; // 1MB
        std::size_t slices = 1;
        std::size_t bufferShape[3] = { slices, sizeY, sizeZ };
        Math::Array<BufferType, 3> buffer (bufferShape, false);
        Math::ArrayView<BufferType, 3, false, Math::ArrayConfig, Math::ArrayAssertions<assertions> > bufferViewMaybeMirrored (maybeMirrorYAxis(buffer.view (), self.MirroredYAxis));
        HDF5::DataSpace memDataSpace = HDF5::DataSpace::createSimple (slices, sizeY, sizeZ);
        HDF5::DataSpace fileDataSpace = HDF5::DataSpace::createSimple (sizeX, sizeY, sizeZ);

        for (std::size_t firstSlice = 0; firstSlice < initAmount; firstSlice += slices) {
            callback(firstSlice, initAmount);
            std::size_t slicesCur = slices;
            if (firstSlice + slices > sizeX) {
                slicesCur = sizeX - firstSlice;
                memDataSpace = HDF5::DataSpace::createSimple (slicesCur, sizeY, sizeZ);
            }

            hsize_t start[3] = { firstSlice, 0, 0 };
            hsize_t count[3] = { slicesCur, sizeY, sizeZ };
            fileDataSpace.selectHyperslab (H5S_SELECT_SET, count, start);

            self.Image.dataSet.read (buffer.data (), HDF5::getH5Type<BufferType> (), memDataSpace, fileDataSpace);

            for (std::size_t x = 0; x < slicesCur; x++)
                for (std::size_t y = 0; y < sizeY; y++)
                    for (std::size_t z = 0; z < sizeZ; z++)
                        transformedView (firstSlice + x, y, z) = bufferViewMaybeMirrored (x, y, z);

        }
        callback(sizeX, sizeX);
    }
    //QDateTime end = QDateTime::currentDateTimeUtc ();
    //qDebug() << (end.toMSecsSinceEpoch() - start.toMSecsSinceEpoch());
}

template <typename BufferType, typename T>
void loadAndTransformSingleTo (const RawGen<T, true>& self, const Math::ArrayView<T, 3>& view, int picNumber) {
    static const bool assertions = false;
    std::vector<int32_t> swapDim13;
    swapDim13.push_back (3);
    swapDim13.push_back (2);
    swapDim13.push_back (1);

    Math::ArrayView<T, 3, false, Math::ArrayConfig, Math::ArrayAssertions<assertions> > transformedView = Math::reorderDimensions (view, swapDim13, true);

    std::size_t sizeX = transformedView.template size<0> ();
    std::size_t sizeY = transformedView.template size<1> ();
    std::size_t sizeZ = transformedView.template size<2> ();

    //qDebug() << transformedView.template size<0>() << transformedView.template size<1>() << transformedView.template size<2>();

    //ASSERT (sizeX == self.Image.size[2]);
    ASSERT (sizeX == 1);
    ASSERT (sizeY == self.Image.size[1]);
    ASSERT (sizeZ == self.Image.size[0]);

    bool useDirectLoad = false; // Disable direct loads because of HDFFV-9634
    //QDateTime start = QDateTime::currentDateTimeUtc ();
    if (useDirectLoad) {
        //std::cerr << "DIRECT LOAD" << std::endl;
        self.Image.read (maybeMirrorYAxis(Math::reorderDimensions (transformedView, swapDim13), self.MirroredYAxis));
    } else {
        //std::cerr << "BUFFERED LOAD" << std::endl;
        //std::size_t sliceSizeBytes = sizeZ * sizeY * sizeof (T);
        //std::size_t maxBufferSize = 1 * 1024 * 1024; // 1MB
        std::size_t slices = 1;
        std::size_t bufferShape[3] = { slices, sizeY, sizeZ };
        Math::Array<BufferType, 3> buffer (bufferShape, false);
        Math::ArrayView<BufferType, 3, false, Math::ArrayConfig, Math::ArrayAssertions<assertions> > bufferViewMaybeMirrored (maybeMirrorYAxis(buffer.view (), self.MirroredYAxis));
        HDF5::DataSpace memDataSpace = HDF5::DataSpace::createSimple (slices, sizeY, sizeZ);
        HDF5::DataSpace fileDataSpace = HDF5::DataSpace::createSimple (self.Image.size[2], sizeY, sizeZ);

        std::size_t firstSlice = picNumber;
        std::size_t slicesCur = slices;
        /*
        if (firstSlice + slices > sizeX) {
            slicesCur = sizeX - firstSlice;
            memDataSpace = HDF5::DataSpace::createSimple (slicesCur, sizeY, sizeZ);
        }
        */

        hsize_t start[3] = { firstSlice, 0, 0 };
        hsize_t count[3] = { slicesCur, sizeY, sizeZ };
        fileDataSpace.selectHyperslab (H5S_SELECT_SET, count, start);

        self.Image.dataSet.read (buffer.data (), HDF5::getH5Type<BufferType> (), memDataSpace, fileDataSpace);

        for (std::size_t x = 0; x < slicesCur; x++)
            for (std::size_t y = 0; y < sizeY; y++)
                for (std::size_t z = 0; z < sizeZ; z++)
                    transformedView (x, y, z) = bufferViewMaybeMirrored (x, y, z);

    }

    //QDateTime end = QDateTime::currentDateTimeUtc ();
    //qDebug() << (end.toMSecsSinceEpoch() - start.toMSecsSinceEpoch());
}



static inline void callbackDooNothing(UNUSED size_t pos, UNUSED size_t count) {}

template <typename BufferType, typename T>
void loadAndTransformTo (const RawGen<T, true>& self, const Math::ArrayView<T, 3>& view, size_t initAmount) {
    loadAndTransformTo<BufferType, T>(self, view, initAmount, callbackDooNothing);
}

#endif // RAWDATAFILES_H
