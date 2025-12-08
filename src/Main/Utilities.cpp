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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "Utilities.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusVariant.hpp>
#include <VoxieClient/ToString.hpp>

#include <VoxieBackend/Data/Buffer.hpp>
#include <VoxieBackend/Data/BufferType.hpp>
#include <VoxieBackend/Data/ImageDataPixel.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/InterpolationMethod.hpp>
#include <VoxieBackend/Data/VolumeData.hpp>

#include <Voxie/Data/Spectrum.hpp>

using namespace vx;

class UtilitiesAdaptorImpl : public UtilitiesAdaptor {
  Q_OBJECT

  Utilities* object;

 public:
  UtilitiesAdaptorImpl(Utilities* object)
      : UtilitiesAdaptor(object), object(object) {}
  ~UtilitiesAdaptorImpl() {}

  void ExtractLine(const QDBusObjectPath& volume,
                   const vx::TupleVector<double, 3>& startPoint,
                   const vx::TupleVector<double, 3>& endPoint,
                   const QDBusObjectPath& outputBuffer,
                   quint64 outputBufferOffset, quint64 outputBufferCount,
                   const QMap<QString, QDBusVariant>& options) override;

  void ExtractSlice(const QDBusObjectPath& volume,
                    const vx::TupleVector<double, 3>& origin,
                    const vx::TupleVector<double, 4>& rotation,
                    const vx::TupleVector<quint64, 2>& outputSize,
                    const vx::TupleVector<double, 2>& pixelSize,
                    const QDBusObjectPath& outputImage,
                    const QMap<QString, QDBusVariant>& options) override;

  QList<std::tuple<quint64, double>> DebugFindMatchingSpectrums(
      const QDBusObjectPath& dimension, double start, double end,
      const QMap<QString, QDBusVariant>& options) override;

  QDBusVariant DebugReturnValue(qint32 mode,
                                const QDBusVariant& value) override;
};

Utilities::Utilities(Root* root)
    : ExportedObject("Utilities", nullptr, true), root_(root) {
  new UtilitiesAdaptorImpl(this);
}
Utilities::~Utilities() {}

void UtilitiesAdaptorImpl::ExtractLine(
    const QDBusObjectPath& volume, const vx::TupleVector<double, 3>& startPoint,
    const vx::TupleVector<double, 3>& endPoint,
    const QDBusObjectPath& outputBuffer, quint64 outputBufferOffset,
    quint64 outputBufferCount, const QMap<QString, QDBusVariant>& options) {
  try {
    vx::ExportedObject::checkOptions(options, "Interpolation");

    auto volumeObj = vx::VolumeData::lookup(volume);

    // TODO: Move code vor Interpolation option somewhere else
    InterpolationMethod interpolation = vx::InterpolationMethod::Linear;
    if (ExportedObject::hasOption(options, "Interpolation")) {
      auto interpolationVal =
          ExportedObject::getOptionValue<QString>(options, "Interpolation");
      if (interpolationVal == "NearestNeighbor")
        interpolation = vx::InterpolationMethod::NearestNeighbor;
      else if (interpolationVal == "Linear")
        interpolation = vx::InterpolationMethod::Linear;
      else
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOptionValue",
                            "Invalid value for 'Interpolation' option");
    }

    auto outputBufferObj = vx::Buffer::lookup(outputBuffer);

    // TODO: support other types
    auto array = outputBufferObj->asArray1<Float32Buffer>();

    if (outputBufferOffset > std::numeric_limits<size_t>::max())
      throw Exception("de.uni_stuttgart.Voxie.Overflow",
                      "outputBufferOffset parameter too large");
    size_t outputBufferOffsetS = outputBufferOffset;
    if (outputBufferCount > std::numeric_limits<size_t>::max())
      throw Exception("de.uni_stuttgart.Voxie.Overflow",
                      "outputBufferCount parameter too large");
    size_t outputBufferCountS = outputBufferCount;

    if (outputBufferOffsetS + outputBufferCountS < outputBufferOffsetS)
      throw Exception(
          "de.uni_stuttgart.Voxie.Overflow",
          "outputBufferOffset + outputBufferCount parameters too large");

    if (outputBufferOffsetS + outputBufferCountS > array.size<0>())
      throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "outputBuffer array too small");

    volumeObj->extractLine(toVector(startPoint), toVector(endPoint),
                           interpolation, array, outputBufferOffset,
                           outputBufferCount);
  } catch (vx::Exception& e) {
    e.handle(object);
  }
}

void UtilitiesAdaptorImpl::ExtractSlice(
    const QDBusObjectPath& volume, const vx::TupleVector<double, 3>& origin,
    const vx::TupleVector<double, 4>& rotation,
    const vx::TupleVector<quint64, 2>& outputSize,
    const vx::TupleVector<double, 2>& pixelSize,
    const QDBusObjectPath& outputImage,
    const QMap<QString, QDBusVariant>& options) {
  try {
    vx::ExportedObject::checkOptions(options, "Interpolation", "UpdateBuffer");

    auto volumeObj = vx::VolumeData::lookup(volume);

    InterpolationMethod interpolation = vx::InterpolationMethod::Linear;
    if (ExportedObject::hasOption(options, "Interpolation")) {
      auto interpolationVal =
          ExportedObject::getOptionValue<QString>(options, "Interpolation");
      if (interpolationVal == "NearestNeighbor")
        interpolation = vx::InterpolationMethod::NearestNeighbor;
      else if (interpolationVal == "Linear")
        interpolation = vx::InterpolationMethod::Linear;
      else
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOptionValue",
                            "Invalid value for 'Interpolation' option");
    }

    bool updateBuffer = true;
    if (ExportedObject::hasOption(options, "UpdateBuffer"))
      updateBuffer =
          ExportedObject::getOptionValue<bool>(options, "UpdateBuffer");

    auto image = vx::ImageDataPixel::lookup(outputImage);

    // TODO: support other types
    auto imageCast =
        qSharedPointerDynamicCast<vx::ImageDataPixelInst<float, 1>>(image);

    volumeObj->extractSlice(
        toQtVector(origin), toQtVector(rotation),
        QSize(std::get<0>(outputSize), std::get<1>(outputSize)),
        std::get<0>(pixelSize), std::get<1>(pixelSize), interpolation,
        imageCast->image());

    if (updateBuffer) imageCast->image().switchMode(FloatImage::STDMEMORY_MODE);
  } catch (vx::Exception& e) {
    e.handle(object);
  }
}

QList<std::tuple<quint64, double>>
UtilitiesAdaptorImpl::DebugFindMatchingSpectrums(
    const QDBusObjectPath& dimension, double start, double end,
    const QMap<QString, QDBusVariant>& options) {
  try {
    vx::ExportedObject::checkOptions(options);

    auto dimensionObj = vx::SeriesDimension::lookup(dimension);

    auto result = findMatchingSpectrums(dimensionObj, start, end);

    QList<std::tuple<quint64, double>> res;
    for (const auto& r : result) res.append(std::make_tuple(r.key, r.weight));
    return res;
  } catch (vx::Exception& e) {
    return e.handle(object);
  }
}

QDBusVariant UtilitiesAdaptorImpl::DebugReturnValue(qint32 mode,
                                                    const QDBusVariant& value) {
  try {
    qDebug() << "Mode=" << mode;
    // qDebug() << value.variant().userType() << qMetaTypeId<QDBusArgument>();
    // qDebug() << "GOT" << value;

    if (mode == 0) {
      return value;
    } else if (mode == 1) {
      auto variant = DBusVariant::fromQDBusVariant(value);
      qDebug() << toString(variant);
      return value;
    } else if (mode == 2) {
      auto variant = DBusVariant::fromQDBusVariant(value);
      qDebug() << toString(variant);
      return variant.asQDBusVariant();
    } else {
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Unexpected mode value");
    }
  } catch (vx::Exception& e) {
    return e.handle(object);
  }
}

#include "Utilities.moc"
