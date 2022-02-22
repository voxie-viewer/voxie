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
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include "VolumeDataVoxel.hpp"

#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieBackend/Data/HistogramProvider.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/PlaneInfo.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>
#include <VoxieBackend/Data/VolumeStructureVoxel.hpp>

#include <VoxieBackend/OpenCL/CLInstance.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QUuid>

#include <time.h>

using namespace vx;

namespace vx {
namespace internal {
class VolumeDataVoxelAdaptorImpl : public VolumeDataVoxelAdaptor {
  Q_OBJECT

  VolumeDataVoxel* object;

 public:
  VolumeDataVoxelAdaptorImpl(VolumeDataVoxel* object)
      : VolumeDataVoxelAdaptor(object), object(object) {}
  virtual ~VolumeDataVoxelAdaptorImpl() {}

  vx::TupleVector<double, 3> gridSpacing() const override;

  // type needs to be fully qualified for moc
  vx::TupleVector<quint64, 3> arrayShape() const override;

  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64, quint64>,
             std::tuple<qint64, qint64, qint64>, QMap<QString, QDBusVariant>>
  GetDataReadonly(const QMap<QString, QDBusVariant>& options) override;
  std::tuple<QMap<QString, QDBusVariant>, qint64,
             std::tuple<QString, quint32, QString>,
             std::tuple<quint64, quint64, quint64>,
             std::tuple<qint64, qint64, qint64>, QMap<QString, QDBusVariant>>
  GetDataWritable(const QDBusObjectPath& update,
                  const QMap<QString, QDBusVariant>& options) override;
};

class AsyncHistogramCalculator : public QObject, public QRunnable {
  Q_OBJECT
 public:
  AsyncHistogramCalculator(QSharedPointer<VolumeDataVoxel> volume,
                           quint32 bucketCount)
      : volume(volume), bucketCount(bucketCount) {
    qRegisterMetaType<vx::HistogramProvider::DataPtr>();
  }

  void run() override {
    if (thread() != nullptr) {
      qCritical() << "AsyncHistogramCalculator already has a thread:" << this;
    } else {
      moveToThread(QThread::currentThread());
    }

    volume->performInGenericContext([this](auto& data) {
      auto minMax = data.getMinMaxValue();

      auto histogram = HistogramProvider::generateData(
          minMax.first, minMax.second, bucketCount, data.getData(),
          data.getData() + data.getSize(),
          [](auto value) { return static_cast<double>(value); });

      Q_EMIT this->finished(histogram);
    });
  }

 Q_SIGNALS:
  void finished(vx::HistogramProvider::DataPtr data);

 private:
  QSharedPointer<VolumeDataVoxel> volume;
  quint32 bucketCount = 0;
};

}  // namespace internal
}  // namespace vx

using namespace vx::internal;

static size_t calcSizeBytes(size_t width, size_t height, size_t depth,
                            DataType typeReference) {
  return checked_mul(checked_mul(checked_mul(width, height), depth),
                     getElementSizeBytes(typeReference));
}

VolumeDataVoxel::VolumeDataVoxel(size_t width, size_t height, size_t depth,
                                 DataType dataType)
    : vx::VolumeData(
          VolumeStructureVoxel::create(vx::VectorSizeT3(width, height, depth))),
      // data(nullptr),
      size(width * height * depth),
      dimensions(width, height, depth),
      spacing(1.0f, 1.0f, 1.0f),
      dataSH(createQSharedPointer<SharedMemory>(
          calcSizeBytes(width, height, depth, dataType))) {
  setDimensionsMetric(QVector3D(width, height, depth));
  setOrigin(QVector3D(0.0f, 0.0f, 0.0f));
  new VolumeDataVoxelAdaptorImpl(this);
  qRegisterMetaType<QSharedPointer<VolumeDataVoxel>>();
  connect(this, &VolumeDataVoxel::changed, this, &VolumeDataVoxel::invalidate);
  connect(this, &VolumeDataVoxel::changed, this,
          &VolumeDataVoxel::updateAllHistograms);
}

VolumeDataVoxel::~VolumeDataVoxel() {}

QList<QString> VolumeDataVoxel::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.VolumeDataVoxel",
      "de.uni_stuttgart.Voxie.VolumeData",
  };
}

QSharedPointer<HistogramProvider> VolumeDataVoxel::getHistogramProvider(
    quint32 bucketCount) {
  if (auto provider = histogramProviders.value(bucketCount)) {
    return provider.lock();
  }

  auto provider = QSharedPointer<vx::HistogramProvider>::create();
  histogramProviders.insert(bucketCount, provider);
  updateHistogram(provider, bucketCount);

  return provider;
}

vx::Array3Info VolumeDataVoxel::dataFd(bool rw) {
  vx::Array3Info info;
  dataSH->getHandle(rw, info.handle);
  info.offset = 0;

  this->performInGenericContext([&](auto& data) {
    typedef
        typename std::remove_reference<decltype(data)>::type::VoxelType Type;

    getDataTypeInfo(data.getDataType(), info.dataType, info.dataTypeSize,
                    info.byteorder);

    info.sizeX = dimensions.x;
    info.sizeY = dimensions.y;
    info.sizeZ = dimensions.z;
    info.strideX = sizeof(Type);
    info.strideY = info.strideX * info.sizeX;
    info.strideZ = info.strideY * info.sizeY;
  });
  return info;
}

void VolumeDataVoxel::extractSlice(const QVector3D& origin1,
                                   const QQuaternion& rotation,
                                   const QSize& outputSize, double pixelSizeX,
                                   double pixelSizeY,
                                   InterpolationMethod interpolation,
                                   FloatImage& outputImage) {
  PlaneInfo plane(origin1, rotation);
  QRectF sliceArea(0, 0, outputSize.width() * pixelSizeX,
                   outputSize.height() * pixelSizeY);

  if ((size_t)outputSize.width() > outputImage.getWidth() ||
      (size_t)outputSize.height() > outputImage.getHeight())
    throw vx::Exception("de.uni_stuttgart.Voxie.IndexOutOfRange",
                        "Index is out of range");

  bool useCL = true;
  // When OpenCL is not available, fall back to CPU
  if (useCL) useCL = vx::opencl::CLInstance::getDefaultInstance()->isValid();
  // When OpenCL is available but allocating the OpenCL image for the current
  // dataset failed, fall back to CPU
  if (useCL)
    this->performInGenericContext(
        [&useCL](auto& data) { useCL = data.getCLImage()() != nullptr; });

  bool clFailed = false;

  if (useCL) {
    using namespace vx::opencl;
    try {
      CLInstance* clInstance = CLInstance::getDefaultInstance();

      QString progId("slice_image_generator");
      if (!clInstance->hasProgramID(progId)) {
        clInstance->createProgramFromFile(":/cl_kernels/sliceimagegenerator.cl",
                                          "", progId);
      }
      cl::Kernel kernel = clInstance->getKernel(
          progId, (interpolation == vx::InterpolationMethod::NearestNeighbor
                       ? "extract_slice_nearest"
                       : "extract_slice_linear"));

      cl::Image3D clvolume;
      this->performInGenericContext(
          [&clvolume](auto& data) { clvolume = data.getCLImage(); });

      size_t numElements = outputSize.width() * outputSize.height();
      outputImage.switchMode(FloatImage::CLMEMORY_MODE, false, clInstance);
      cl::Buffer clbuffer = outputImage.getCLBuffer();

      QVector3D origin2 = (plane.origin - this->origin());

      cl_uint index = 0;
      kernel.setArg(index++, clvolume);
      kernel.setArg(index++, clbuffer);
      QVector3D volDims = this->getDimensionsMetric();
      kernel.setArg(index++, clVec4f(volDims.x(), volDims.y(), volDims.z(),
                                     outputImage.getWidth()));
      kernel.setArg<cl_uint>(index++, outputImage.getWidth());
      kernel.setArg(index++, qVec3_To_clVec3f(origin2));
      kernel.setArg(index++, qVec3_To_clVec3f(plane.tangent()));
      kernel.setArg(index++, qVec3_To_clVec3f(plane.cotangent()));
      kernel.setArg(index++, qRectF_To_clVec4f(sliceArea));
      // set whether to use floats or integers
      this->performInGenericContext([&](auto& data) {
        if (data.getDataType() == DataType::Float16 ||
            data.getDataType() == DataType::Float32 ||
            data.getDataType() == DataType::Float64) {
          kernel.setArg(index++, 0);
        } else {
          kernel.setArg(index++, 1);
        }
      });

      clInstance->executeKernel(kernel, cl::NDRange(numElements));
    } catch (CLException& ex) {
      qWarning() << ex;
      clFailed = true;
    } catch (IOException& ex) {
      qWarning() << ex;
      clFailed = true;
    }
  }

  if (!useCL || clFailed) {
    if (outputImage.getMode() != SliceImage::STDMEMORY_MODE) {
      outputImage.switchMode(false);  // switch mode without syncing memory
    }
    // qDebug() << "VolumeDataVoxel no OpenCl";
    FloatBuffer buffer = outputImage.getBuffer();

    this->performInGenericContext([&outputSize, &sliceArea, &buffer,
                                   &outputImage, &plane,
                                   &interpolation](auto& data) {
      for (size_t y = 0; y < (size_t)outputSize.height(); y++) {
        for (size_t x = 0; x < (size_t)outputSize.width(); x++) {
          QPointF planePoint;
          SliceImage::imagePoint2PlanePoint(x, y, outputSize, sliceArea,
                                            planePoint, false);
          QVector3D volumePoint =
              plane.get3DPoint(planePoint.x(), planePoint.y());
          buffer[y * outputImage.getWidth() + x] = (float)data.getVoxelMetric(
              volumePoint.x(), volumePoint.y(), volumePoint.z(), interpolation);
        }
      }
    });
  }
}

void VolumeDataVoxel::updateHistogram(
    QSharedPointer<HistogramProvider> histogramProvider, quint32 bucketCount) {
  if (!histogramProvider) {
    qWarning()
        << "VolumeDataVoxel::updateHistogram: histogramProvider is nullptr";
    return;
  }

  // TODO: ensure some sort of thread-safety?
  // This may be susceptible to race conditions on rapidly repeated updates,
  // as well as data corruption when another thread writes to the volume
  auto calculator = new AsyncHistogramCalculator(thisShared(), bucketCount);

  connect(calculator, &AsyncHistogramCalculator::finished, this,
          [=](HistogramProvider::DataPtr histogram) {
            if (histogramProvider) histogramProvider->setData(histogram);
          });

  calculator->setAutoDelete(true);
  calculator->moveToThread(nullptr);
  // After this function call there must not be any usage of calculator (because
  // it might be deleted)
  QThreadPool::globalInstance()->start(calculator);
}

void VolumeDataVoxel::updateAllHistograms() {
  for (auto it = histogramProviders.begin(); it != histogramProviders.end();
       ++it) {
    auto provider = it.value().lock();
    if (!provider) {
      // Provider has been destroyed. Should it be removed from
      // histogramProviders?
    } else {
      updateHistogram(provider, it.key());
    }
  }
}

AffineMap<double, 3UL, 3UL> VolumeDataVoxel::getVoxelToObjectTrafo() {
  auto trans = mapCast<double>(createTranslation(toVector(this->origin())));
  // TODO: fix overload problem
  auto scale = Matrix<double, 3, 3>({this->getSpacing().x(), 0, 0},
                                    {0, this->getSpacing().y(), 0},
                                    {0, 0, this->getSpacing().z()});
  return trans * createLinearMap(scale);
}

vx::TupleVector<double, 3> VolumeDataVoxelAdaptorImpl::gridSpacing() const {
  return toTupleVector(object->getSpacing());
}

vx::TupleVector<quint64, 3> VolumeDataVoxelAdaptorImpl::arrayShape() const {
  return object->getDimensions().toTupleVector();
}

std::tuple<QMap<QString, QDBusVariant>, qint64,
           std::tuple<QString, quint32, QString>,
           std::tuple<quint64, quint64, quint64>,
           std::tuple<qint64, qint64, qint64>, QMap<QString, QDBusVariant>>
VolumeDataVoxelAdaptorImpl::GetDataReadonly(
    const QMap<QString, QDBusVariant>& options) {
  try {
    vx::ExportedObject::checkOptions(options);
    return object->dataFd(false).toDBus();
  } catch (vx::Exception& e) {
    e.handle(object);
    return vx::Array3Info().toDBus();
  }
}
std::tuple<QMap<QString, QDBusVariant>, qint64,
           std::tuple<QString, quint32, QString>,
           std::tuple<quint64, quint64, quint64>,
           std::tuple<qint64, qint64, qint64>, QMap<QString, QDBusVariant>>
VolumeDataVoxelAdaptorImpl::GetDataWritable(
    const QDBusObjectPath& update, const QMap<QString, QDBusVariant>& options) {
  try {
    vx::ExportedObject::checkOptions(options);

    auto updateObj = vx::DataUpdate::lookup(update);
    if (updateObj->data().data() != object)
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Given DataUpdate is for another object");
    if (!updateObj->running())
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                          "Given DataUpdate is already finished");

    return object->dataFd(true).toDBus();
  } catch (vx::Exception& e) {
    e.handle(object);
    return vx::Array3Info().toDBus();
  }
}

QList<QSharedPointer<SharedMemory>> VolumeDataVoxel::getSharedMemorySections() {
  return {this->dataSH};
}

void VolumeDataVoxel::getVolumeInfo(
    QList<std::tuple<QString, QString>>& fields) {
  // int precision = 6; // Takes too much space
  int precision = 5;

  fields << std::make_tuple(
      "Dimension", QString::number(this->getDimensions().x) + " x " +
                       QString::number(this->getDimensions().y) + " x " +
                       QString::number(this->getDimensions().z));

  fields << std::make_tuple(
      "Spacing",
      QString::number(this->getSpacing().x(), 'g', precision) + " x " +
          QString::number(this->getSpacing().y(), 'g', precision) + " x " +
          QString::number(this->getSpacing().z(), 'g', precision));

  fields << std::make_tuple("Data Type",
                            DataTypeNames.value(this->getDataType()));
}

double VolumeDataVoxel::getStepSize(const vx::Vector<double, 3>& dir) {
  auto dirNorm = vx::normalize(dir);
  return dirNorm.access<0>() * dirNorm.access<0>() * spacing.x() +
         dirNorm.access<1>() * dirNorm.access<1>() * spacing.y() +
         dirNorm.access<2>() * dirNorm.access<2>() * spacing.z();
}

void VolumeDataVoxel::extractGrid(const QVector3D& origin,
                                  const QQuaternion& rotation,
                                  const QSize& outputSize, double pixelSizeX,
                                  double pixelSizeY, QImage& outputImage,
                                  QRgb color) {
  PlaneInfo plane(origin, rotation);
  QRectF sliceArea(0, 0, outputSize.width() * pixelSizeX,
                   outputSize.height() * pixelSizeY);

  if (outputSize.width() > outputImage.width() ||
      outputSize.height() > outputImage.height())
    throw vx::Exception("de.uni_stuttgart.Voxie.IndexOutOfRange",
                        "Index is out of range");

  // TODO: OpenCL implementation?
  /*
  bool useCL = true;
  if (useCL) useCL = vx::opencl::CLInstance::getDefaultInstance()->isValid();

  bool clFailed = false;

  if (useCL) {
    using namespace vx::opencl;
  }
  if (!useCL || clFailed) {
    // qDebug() << "VolumeDataVoxel no OpenCl";
    */

  // TODO: Clean up

  std::vector<vx::TupleVector<size_t, 3>> nodes((size_t)outputSize.width() *
                                                (size_t)outputSize.height());

  try {
    for (size_t y = 0; y < (size_t)outputSize.height(); y++) {
      for (size_t x = 0; x < (size_t)outputSize.width(); x++) {
        QPointF planePoint;
        SliceImage::imagePoint2PlanePoint(x, y, outputSize, sliceArea,
                                          planePoint, false);
        QVector3D volumePoint =
            plane.get3DPoint(planePoint.x(), planePoint.y());
        QVector3D voxelIndex = (volumePoint - this->origin()) / this->spacing;
        if (voxelIndex.x() < 0 || voxelIndex.y() < 0 || voxelIndex.z() < 0) {
          nodes[(size_t)outputSize.width() * y + x] =
              std::make_tuple((uint64_t)-1, (uint64_t)-1, (uint64_t)-1);
        } else {
          // Round downwards
          // TODO: Handle overflow properly
          size_t xi = (size_t)voxelIndex.x();
          size_t yi = (size_t)voxelIndex.y();
          size_t zi = (size_t)voxelIndex.z();
          if (xi >= dimensions.x || yi >= dimensions.y || zi >= dimensions.z) {
            nodes[(size_t)outputSize.width() * y + x] =
                std::make_tuple((uint64_t)-1, (uint64_t)-1, (uint64_t)-1);
          } else {
            nodes[(size_t)outputSize.width() * y + x] =
                std::make_tuple(xi, yi, zi);
          }
        }
      }
    }

    for (size_t y = 0; y < (size_t)outputSize.height(); y++) {
      for (size_t x = 0; x < (size_t)outputSize.width(); x++) {
        bool draw = false;

        if (nodes[(size_t)outputSize.width() * y + x] !=
            std::make_tuple((uint64_t)-1, (uint64_t)-1, (uint64_t)-1)) {
          // check only with right pos
          if (x < (size_t)outputSize.width() - 1) {
            if (nodes[(size_t)outputSize.width() * y + x] !=
                nodes[(size_t)outputSize.width() * y + x + 1])
              draw = true;
          }

          // check only with upper pos
          if (y > 0) {
            if (nodes[(size_t)outputSize.width() * y + x] !=
                nodes[(size_t)outputSize.width() * (y - 1) + x])
              draw = true;
          }
        }

        QRgb transparent = 0;  // TODO: is this correct?
        outputImage.setPixel(x, outputSize.height() - y - 1,
                             draw ? color : transparent);
      }
    }
  } catch (std::exception& e) {
    qWarning() << "Error while creating volume grid on CPU:" << e.what();
  }
}

#include "VolumeDataVoxel.moc"
