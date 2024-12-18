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

#include "VolumeDataBlock.hpp"

#include <VoxieClient/Array.hpp>
#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/RunParallel.hpp>

#include <VoxieClient/ObjectExport/DBusCallUtil.hpp>

#include <VoxieBackend/DebugOptions.hpp>

// TODO: Use different VolumeStructure?
#include <VoxieBackend/Data/VolumeStructureVoxel.hpp>

#include <VoxieBackend/Data/Buffer.hpp>
#include <VoxieBackend/Data/BufferTypeInst.hpp>
#include <VoxieBackend/Data/ExtractSlice.hpp>
#include <VoxieBackend/Data/PlaneInfo.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>
#include <VoxieBackend/Data/SlidingBlockCache.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

namespace vx {
namespace internal {
class VolumeDataBlockAdaptorImpl : public VolumeDataBlockAdaptor {
  VolumeDataBlock* object;

 public:
  VolumeDataBlockAdaptorImpl(VolumeDataBlock* object)
      : VolumeDataBlockAdaptor(object), object(object) {}
  virtual ~VolumeDataBlockAdaptorImpl() {}

  std::tuple<quint64, quint64, quint64> arrayShape() const override {
    try {
      return toTupleVector(vectorCast<quint64>(object->arrayShape()));
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  std::tuple<quint64, quint64, quint64> blockCount() const override {
    try {
      return toTupleVector(vectorCast<quint64>(object->blockCount()));
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  std::tuple<quint64, quint64, quint64> blockShape() const override {
    try {
      return toTupleVector(vectorCast<quint64>(object->blockShape()));
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  std::tuple<double, double, double> gridSpacing() const override {
    try {
      return toTupleVector(object->gridSpacing());
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  // TODO: Provide some way of progress reporting?
  void DecodeBlocks(const QDBusObjectPath& output,
                    const QDBusObjectPath& updateOutput, quint64 count,
                    const QDBusObjectPath& blocksBuffer, quint64 blocksOffset,
                    const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options, "PutIntoCache");
      bool putIntoCache = ExportedObject::getOptionValueOrDefault<bool>(
          options, "PutIntoCache", true);

      auto blocksBufferObj = vx::Buffer::lookup(blocksBuffer);
      auto blocks =
          blocksBufferObj->asArray1<const VolumeDataBlock::BlockOffsetEntry>();

      if (count > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "count parameter too large");
      size_t countS = count;
      if (blocksOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blocksOffset parameter too large");
      size_t blocksOffsetS = blocksOffset;

      if (blocksOffsetS + countS < blocksOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blocksOffset / count parameters too large");

      if (blocksOffsetS + countS > blocks.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "blocks array too small");

      auto outputObj = vx::VolumeDataVoxel::lookup(output);

      auto updateOutputObj = vx::DataUpdate::lookup(updateOutput);
      updateOutputObj->validateCanUpdate(outputObj);

      handleDBusCallOnBackgroundThreadVoid(
          object, [self = object->thisShared(), countS, blocks, blocksOffsetS,
                   outputObj, putIntoCache] {
            // for (size_t i = 0; i < countS; i++) {
            runParallelDynamic(nullptr, nullptr, countS, [&](size_t i) {
              // TODO: Do these have to be volatile reads or something like that
              // to prevent the compiler from assuming that the data in
              // blocks(i) does not change?
              auto entry = blocks(blocksOffsetS + i);
              auto blockID = entry.BlockID;
              auto offset = entry.Offset;
              auto blockShape = entry.BlockShape;
              if (blockID.access<0>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<1>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<2>() > std::numeric_limits<size_t>::max())
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "BlockID parameter too large");
              auto blockIDVec = vectorCastNarrow<size_t>(blockID);

              if (offset.access<0>() > std::numeric_limits<size_t>::max() ||
                  offset.access<1>() > std::numeric_limits<size_t>::max() ||
                  offset.access<2>() > std::numeric_limits<size_t>::max())
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "Offset parameter too large");
              auto offsetVec = vectorCastNarrow<size_t>(offset);

              if (blockShape.access<0>() > std::numeric_limits<size_t>::max() ||
                  blockShape.access<1>() > std::numeric_limits<size_t>::max() ||
                  blockShape.access<2>() > std::numeric_limits<size_t>::max())
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "BlockShape parameter too large");
              auto blockShapeVec = vectorCastNarrow<size_t>(blockShape);
              // qDebug() << blockShapeVec << self->getBlockShape(blockIDVec);
              if (blockShapeVec != self->getBlockShape(blockIDVec))
                throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                                    "Invalid block shape");

              auto outputBlock =
                  outputObj->getBlockVoid(offsetVec, blockShapeVec);

              self->decodeBlock(vx::defaultBlockCache().data(), blockIDVec,
                                outputObj->getDataType(), outputBlock,
                                putIntoCache);
            });
            //}
          });
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }
  // TODO: Provide some way of progress reporting?
  void EncodeBlocks(const QDBusObjectPath& update, const QDBusObjectPath& input,
                    quint64 count, const QDBusObjectPath& blocksBuffer,
                    quint64 blocksOffset,
                    const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      auto updateObj = vx::DataUpdate::lookup(update);
      updateObj->validateCanUpdate(object);

      auto blocksBufferObj = vx::Buffer::lookup(blocksBuffer);
      auto blocks =
          blocksBufferObj->asArray1<const VolumeDataBlock::BlockOffsetEntry>();

      if (count > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "count parameter too large");
      size_t countS = count;
      if (blocksOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blocksOffset parameter too large");
      size_t blocksOffsetS = blocksOffset;

      if (blocksOffsetS + countS < blocksOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blocksOffset / count parameters too large");

      if (blocksOffsetS + countS > blocks.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "blocks array too small");

      auto inputObj = vx::VolumeDataVoxel::lookup(input);

      handleDBusCallOnBackgroundThreadVoid(object, [self = object->thisShared(),
                                                    countS, blocks,
                                                    blocksOffsetS, inputObj] {
        // for (size_t i = 0; i < countS; i++) {
        runParallelDynamic(nullptr, nullptr, countS, [&](size_t i) {
          // TODO: Do these have to be volatile reads or something like that to
          // prevent the compiler from assuming that the data in blocks(i) does
          // not change?
          auto entry = blocks(blocksOffsetS + i);
          auto blockID = entry.BlockID;
          auto offset = entry.Offset;
          auto blockShape = entry.BlockShape;

          if (blockID.access<0>() > std::numeric_limits<size_t>::max() ||
              blockID.access<1>() > std::numeric_limits<size_t>::max() ||
              blockID.access<2>() > std::numeric_limits<size_t>::max())
            throw Exception("de.uni_stuttgart.Voxie.Overflow",
                            "BlockID parameter too large");
          auto blockIDVec = vectorCastNarrow<size_t>(blockID);

          if (offset.access<0>() > std::numeric_limits<size_t>::max() ||
              offset.access<1>() > std::numeric_limits<size_t>::max() ||
              offset.access<2>() > std::numeric_limits<size_t>::max())
            throw Exception("de.uni_stuttgart.Voxie.Overflow",
                            "Offset parameter too large");
          auto offsetVec = vectorCastNarrow<size_t>(offset);

          if (blockShape.access<0>() > std::numeric_limits<size_t>::max() ||
              blockShape.access<1>() > std::numeric_limits<size_t>::max() ||
              blockShape.access<2>() > std::numeric_limits<size_t>::max())
            throw Exception("de.uni_stuttgart.Voxie.Overflow",
                            "BlockShape parameter too large");
          auto blockShapeVec = vectorCastNarrow<size_t>(blockShape);
          // qDebug() << blockShapeVec << self->getBlockShape(blockIDVec);
          if (blockShapeVec != self->getBlockShape(blockIDVec))
            throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                                "Invalid block shape");

          auto inputBlock = inputObj->getBlockVoid(offsetVec, blockShapeVec);

          self->encodeBlock(blockIDVec, inputObj->getDataType(), inputBlock);
        });
        //}
      });
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }
};
}  // namespace internal
}  // namespace vx

vx::VolumeDataBlock::VolumeDataBlock(const vx::Vector<size_t, 3>& arrayShape,
                                     const vx::Vector<size_t, 3>& blockShape,
                                     // DataType dataType,
                                     const vx::Vector<double, 3>& volumeOrigin,
                                     const vx::Vector<double, 3>& gridSpacing)
    : vx::VolumeData(
          volumeOrigin,
          elementwiseProduct(vectorCastNarrow<double>(arrayShape), gridSpacing),
          VolumeStructureVoxel::create(arrayShape)),
      arrayShape_(arrayShape),
      blockShape_(blockShape),
      gridSpacing_(gridSpacing) {
  new vx::internal::VolumeDataBlockAdaptorImpl(this);

  overallBlockCount_ = 1;
  overallBlockSize_ = 1;
  for (size_t dim = 0; dim < 3; dim++) {
    if (blockShape_[dim] == 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                          "Got zero block size");
    if (arrayShape_[dim] + (blockShape_[dim] - 1) < arrayShape_[dim])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Overflow while calculating blockCount");
    blockCount_[dim] =
        (arrayShape_[dim] + (blockShape_[dim] - 1)) / blockShape_[dim];
    overallBlockCount_ = checked_mul(overallBlockCount_, blockCount_[dim]);
    overallBlockSize_ = checked_mul(overallBlockSize_, blockShape_[dim]);
  }
}

vx::VolumeDataBlock::~VolumeDataBlock() {}

void vx::VolumeDataBlock::extractSlice(const QVector3D& origin1,
                                       const QQuaternion& rotation,
                                       const QSize& outputSize,
                                       double pixelSizeX, double pixelSizeY,
                                       InterpolationMethod interpolation,
                                       FloatImage& outputImage) {
  // TODO: Should this profiling code be here or in extractSliceCpu()?
  auto pixelCount = outputSize.width() * outputSize.height();
  if (vx::debug_option::Log_ExtractSliceTime()->get())
    qDebug() << "[Blk]extractSlice() start" << pixelCount << "pixel";
  QElapsedTimer timer;
  timer.start();

  extractSliceCpu(*this, origin1, rotation, outputSize, pixelSizeX, pixelSizeY,
                  interpolation, outputImage);

  defaultBlockCache()->printStatistics();

  auto time = timer.nsecsElapsed();
  if (vx::debug_option::Log_ExtractSliceTime()->get())
    qDebug() << "[Blk]extractSlice()" << (time / 1e9)
             << "s"
                //" for" << pixelCount << "pixel"
                ","
             << (pixelCount / (time / 1e9) / 1e6) << "MPix/s";
}

double vx::VolumeDataBlock::getStepSize(const vx::Vector<double, 3>& dir) {
  auto dirNorm = vx::normalize(dir);
  return dirNorm.access<0>() * dirNorm.access<0>() * gridSpacing().access<0>() +
         dirNorm.access<1>() * dirNorm.access<1>() * gridSpacing().access<1>() +
         dirNorm.access<2>() * dirNorm.access<2>() * gridSpacing().access<2>();
}

// TODO: Mostly duplicated from VolumeDataVoxel.cpp
void vx::VolumeDataBlock::extractGrid(const QVector3D& origin,
                                      const QQuaternion& rotation,
                                      const QSize& outputSize,
                                      double pixelSizeX, double pixelSizeY,
                                      QImage& outputImage, QRgb color) {
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
    // qDebug() << "VolumeDataBlock no OpenCl";
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
        QVector3D voxelIndex =
            (volumePoint - this->origin()) /
            toQVector(vectorCastNarrow<float>(this->gridSpacing()));
        if (voxelIndex.x() < 0 || voxelIndex.y() < 0 || voxelIndex.z() < 0) {
          nodes[(size_t)outputSize.width() * y + x] =
              std::make_tuple((uint64_t)-1, (uint64_t)-1, (uint64_t)-1);
        } else {
          // Round downwards
          // TODO: Handle overflow properly
          size_t xi = (size_t)voxelIndex.x();
          size_t yi = (size_t)voxelIndex.y();
          size_t zi = (size_t)voxelIndex.z();
          if (xi >= arrayShape().access<0>() ||
              yi >= arrayShape().access<1>() ||
              zi >= arrayShape().access<2>()) {
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

vx::DataType vx::VolumeDataBlock::getDataType() {
  return vx::DataTypeTraitsByType<float>::getDataType();
}

vx::Vector<size_t, 3> vx::VolumeDataBlock::getBlockShape(
    const vx::Vector<size_t, 3>& blockId) {
  vx::Vector<size_t, 3> res;
  for (size_t dim = 0; dim < 3; dim++) {
    if (blockId[dim] >= this->blockCount()[dim])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
    if (blockId[dim] + 1 == this->blockCount()[dim])
      res[dim] = arrayShape()[dim] - blockShape()[dim] * blockId[dim];
    else
      res[dim] = blockShape()[dim];
  }
  return res;
}

void vx::VolumeDataBlock::decodeBlock(BlockCache* cache,
                                      const vx::Vector<size_t, 3>& blockId,
                                      DataType dataType,
                                      const vx::Array3<void>& data,
                                      bool putIntoCache) {
  auto expectedBlockShape = this->getBlockShape(blockId);

  if (data.size<0>() != expectedBlockShape[0] ||
      data.size<1>() != expectedBlockShape[1] ||
      data.size<2>() != expectedBlockShape[2])
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Got invalid data shape in VolumeDataBlock::decodeBlock()");

  auto block = cache->getDecodedBlock(this, blockId, dataType, !putIntoCache);

  if (block) {
    // TODO: Do this more generic?
    if (dataType != vx::DataTypeTraitsByType<float>::getDataType())
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.NotImplemented",
          "Not implemented: getDecodedBlock() currently only supports float");
    vx::Array3<float> dataFloat(data);
    DecodedBlockReference<float> blockFloat(std::move(block));

    for (size_t z = 0; z < expectedBlockShape[2]; z++)
      for (size_t y = 0; y < expectedBlockShape[1]; y++)
        for (size_t x = 0; x < expectedBlockShape[0]; x++)
          dataFloat(x, y, z) = blockFloat(x, y, z);
  } else {
    decodeBlockNoCache(blockId, dataType, data);
  }
}

QSharedPointer<vx::DecodedBlock> vx::VolumeDataBlock::getDecodedBlock(
    BlockProvider* provider, const vx::Vector<size_t, 3>& blockId,
    DataType dataType, bool failIfNotInCache) {
  if (provider != this)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "VolumeDataBlock::getDecodedBlock() called with another volume");
  return getDecodedBlock(blockId, dataType, failIfNotInCache);
}

QSharedPointer<vx::DecodedBlock> vx::VolumeDataBlock::getDecodedBlock(
    const vx::Vector<size_t, 3>& blockId, DataType dataType,
    bool failIfNotInCache) {
  if (failIfNotInCache) return QSharedPointer<vx::DecodedBlock>();

  if (dataType != vx::DataTypeTraitsByType<float>::getDataType())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.NotImplemented",
        "Not implemented: getDecodedBlock() currently only supports float");

  for (std::size_t i = 0; i < 3; i++) {
    if (blockId[i] >= blockCount()[i])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
  }

  // Note: Allocate the full block shape even for incomplete blocks
  vx::Array3<float> data(blockShape().asArray());
  /*
  auto data = new float[overallBlockSize()];
  std::shared_ptr<void> ptr(data, [](void* p) { delete[](float*) p; });
  */

  auto block = createQSharedPointer<DecodedBlock>(
      info(), vx::DataTypeTraitsByType<float>::getDataType(),
      vx::Array3<void>(data));

  auto thisBlockShape = this->getBlockShape(blockId);

  // Note: Strides are based on blockShape() even for incomplete blocks
  vx::Array3<float> array((float*)block->array().data(),
                          thisBlockShape.asArray(),
                          {
                              block->array().strideBytes<0>(),
                              block->array().strideBytes<1>(),
                              block->array().strideBytes<2>(),
                          },
                          // TODO: Pass in a null pointer here?
                          block->array().getBackend());
  // TODO: Should this be done always?
  if (thisBlockShape != this->blockShape() || true) {
    // Incomplete block, initialize with NaN
    vx::Array3<float> fullArray(block->array());
    for (size_t z = 0; z < fullArray.size<2>(); z++)
      for (size_t y = 0; y < fullArray.size<1>(); y++)
        for (size_t x = 0; x < fullArray.size<0>(); x++)
          fullArray(x, y, z) = std::numeric_limits<float>::quiet_NaN();
  }

  decodeBlockNoCache(blockId, vx::DataTypeTraitsByType<float>::getDataType(),
                     vx::Array3<void>(array));

  return block;
}

void vx::VolumeDataBlock::encodeBlock(const vx::Vector<size_t, 3>& blockId,
                                      DataType dataType,
                                      const vx::Array3<const void>& data) {
  encodeBlockImpl(blockId, dataType, data);

  // Invalidate cache entry
  // TODO: Also do this for other caches? How to find those caches?
  defaultBlockCache()->invalidateCacheEntry(this, blockId, dataType);
}

QList<vx::BufferTypeStruct::Member>
vx::VolumeDataBlock::BlockOffsetEntry::Type::getMembers() {
  return {
      VX_BUFFER_TYPE_MEMBER(BlockID),
      VX_BUFFER_TYPE_MEMBER(Offset),
      VX_BUFFER_TYPE_MEMBER(BlockShape),
  };
}

VX_BUFFER_TYPE_DEFINE(vx::VolumeDataBlock::BlockID,
                      "de.uni_stuttgart.Voxie.VolumeDataBlock.BlockID")
VX_BUFFER_TYPE_DEFINE(vx::VolumeDataBlock::BlockOffsetEntry,
                      "de.uni_stuttgart.Voxie.VolumeDataBlock.BlockOffsetEntry")
