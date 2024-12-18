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

#include <VoxieClient/Array.hpp>

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

// TODO: Avoid BlockCache include?
#include <VoxieBackend/Data/BlockCache.hpp>
#include <VoxieBackend/Data/BlockProvider.hpp>
#include <VoxieBackend/Data/BufferType.hpp>
#include <VoxieBackend/Data/DataType.hpp>
#include <VoxieBackend/Data/InterpolationMethod.hpp>
// TODO: Avoid SlidingBlockCache include?
#include <VoxieBackend/Data/SlidingBlockCache.hpp>
#include <VoxieBackend/Data/VolumeData.hpp>
#include <VoxieBackend/Data/VoxelAccessor.hpp>

#include <VoxieBackend/VoxieBackend.hpp>

#include <QtCore/QFileInfo>
#include <QtCore/QMutex>

#include <memory>

namespace vx {
class VOXIEBACKEND_EXPORT VolumeDataBlock : public VolumeData,
                                            public BlockProvider {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  struct SupportedTypes;

  struct VOXIEBACKEND_EXPORT BlockID {
    using Type = vx::Vector<std::uint64_t, 3>;

    VX_BUFFER_TYPE_DECLARE
  };

  struct VOXIEBACKEND_EXPORT BlockOffsetEntry {
    struct VOXIEBACKEND_EXPORT Type {
      VolumeDataBlock::BlockID::Type BlockID;
      vx::Vector<std::uint64_t, 3> Offset;
      vx::Vector<std::uint64_t, 3> BlockShape;

      static QList<BufferTypeStruct::Member> getMembers();
    };

    VX_BUFFER_TYPE_DECLARE
  };

  class Accessor : public vx::VoxelAccessor<Accessor, float> {
    QSharedPointer<VolumeDataBlock> object;
    // TODO: Avoid copying the accessor and put "SlidingBlockCache sbc" here?
    QSharedPointer<SlidingBlockCache> sbc;

   public:
    using DataType = vx::VoxelAccessor<Accessor, float>::DataType;

    Accessor(const QSharedPointer<VolumeDataBlock>& object,
             const QSharedPointer<BlockProviderOrCache>& cache)
        : object(object),
          sbc(createQSharedPointer<SlidingBlockCache>(object, cache)) {}

    // TODO: Copy values for performance?
    const vx::Vector<size_t, 3>& arrayShape() const {
      return object->arrayShape();
    }
    const vx::Vector<double, 3>& volumeOrigin() const {
      return object->volumeOrigin();
    }
    const vx::Vector<double, 3>& gridSpacing() const {
      return object->gridSpacing();
    }

    inline DataType getVoxelUnchecked(const vx::Vector<size_t, 3>& pos) const {
      return sbc->getVoxelUnchecked(pos);
    }
  };

 protected:
  QFileInfo fileInfo;

 private:
  // TODO: Remove?
  QSharedPointer<BlockProviderInfo> info_;

  // TODO: names?
  vx::Vector<size_t, 3> arrayShape_;
  vx::Vector<size_t, 3> blockShape_;  // Is in BlockProviderInfo // TODO
  vx::Vector<double, 3> gridSpacing_;
  // TODO: Keep dataType?
  // DataType dataType;
  vx::Vector<size_t, 3> blockCount_;
  size_t overallBlockCount_;
  size_t overallBlockSize_;

 protected:
  VolumeDataBlock(const vx::Vector<size_t, 3>& arrayShape,
                  const vx::Vector<size_t, 3>& blockShape,
                  // DataType dataType,
                  const vx::Vector<double, 3>& volumeOrigin,
                  const vx::Vector<double, 3>& gridSpacing);

 public:
  ~VolumeDataBlock();

  // Note: The returned accessor is single-threaded
  Accessor accessor(const QSharedPointer<BlockProviderOrCache>& cache =
                        vx::defaultBlockCache()) {
    return Accessor(thisShared(), cache);
  }

  QSharedPointer<BlockProviderInfo> info() override final { return info_; }

  const vx::Vector<size_t, 3>& arrayShape() final override {
    return arrayShape_;
  }

  // The size of a full block
  const vx::Vector<size_t, 3>& blockShape() final override {
    return blockShape_;
  }

  const vx::Vector<double, 3>& gridSpacing() { return gridSpacing_; }

  const vx::Vector<size_t, 3>& blockCount() final override {
    return blockCount_;
  }

  size_t overallBlockCount() { return overallBlockCount_; }

  size_t overallBlockSize() { return overallBlockSize_; }

  // Get actual size of a block, which might be smaller than blockShape() for
  // the last (incomplete) blocks
  vx::Vector<size_t, 3> getBlockShape(const vx::Vector<size_t, 3>& blockId);

  // TODO: Clean up, move some methods out of this class?

  virtual void decodeBlockNoCache(const vx::Vector<size_t, 3>& blockId,
                                  DataType dataType,
                                  const vx::Array3<void>& data) = 0;

  void decodeBlock(BlockCache* cache, const vx::Vector<size_t, 3>& blockId,
                   DataType dataType, const vx::Array3<void>& data,
                   bool putIntoCache);

  QSharedPointer<DecodedBlock> getDecodedBlock(
      BlockProvider* provider, const vx::Vector<size_t, 3>& blockId,
      DataType dataType, bool failIfNotInCache = false) override;

  QSharedPointer<DecodedBlock> getDecodedBlock(
      const vx::Vector<size_t, 3>& blockId, DataType dataType,
      bool failIfNotInCache = false);

  template <typename T>
  DecodedBlockReference<T> getDecodedBlockInst(
      const vx::Vector<size_t, 3>& blockId) {
    return defaultBlockCache()->getDecodedBlockInst<T>(this, blockId);
  }

  void encodeBlock(const vx::Vector<size_t, 3>& blockId, DataType dataType,
                   const vx::Array3<const void>& data);

  void extractSlice(const QVector3D& origin, const QQuaternion& rotation,
                    const QSize& outputSize, double pixelSizeX,
                    double pixelSizeY, InterpolationMethod interpolation,
                    FloatImage& outputImage) override;

  double getStepSize(const vx::Vector<double, 3>& dir) override;

  void extractGrid(const QVector3D& origin, const QQuaternion& rotation,
                   const QSize& outputSize, double pixelSizeX,
                   double pixelSizeY, QImage& outputImage, QRgb color) override;

  // Will always return 'float'
  DataType getDataType() override;

 protected:
  // Same as encodeBlock(), but does not invalidate the cache entry
  virtual void encodeBlockImpl(const vx::Vector<size_t, 3>& blockId,
                               DataType dataType,
                               const vx::Array3<const void>& data) = 0;
};
}  // namespace vx
