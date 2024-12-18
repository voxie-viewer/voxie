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
#include <VoxieClient/Vector.hpp>

#include <VoxieBackend/Data/DataType.hpp>

#include <VoxieBackend/VoxieBackend.hpp>

namespace vx {

// This class contains parts of a BlockProvider which have to be kept alive for
// some time after the BlockProvider is destroyed.
// TODO: Check whether this is really needed
class VOXIEBACKEND_EXPORT BlockProviderInfo {
 private:
  vx::Vector<size_t, 3> blockShape_;

 public:
  BlockProviderInfo(const vx::Vector<size_t, 3>& blockShape);

  const vx::Vector<size_t, 3>& blockShape() { return blockShape_; }
};

class VOXIEBACKEND_EXPORT DecodedBlock {
 private:
  QSharedPointer<BlockProviderInfo> info_;

  vx::DataType dataType_;
  vx::Array3<void> array_;

 public:
  DecodedBlock(const QSharedPointer<BlockProviderInfo>& info,
               vx::DataType dataType, const vx::Array3<void>& array);

  // const QSharedPointer<BlockProviderInfo>& info() { return info_; }
  vx::DataType dataType() { return dataType_; }
  const vx::Array3<void>& array() { return array_; }
};

class VOXIEBACKEND_EXPORT DecodedBlockReferenceBase {
  friend class VolumeDataBlock;

  QSharedPointer<DecodedBlock> block_;

 protected:
  Q_NORETURN void raiseMismatchingDataTypeException(vx::DataType expected);

 public:
  DecodedBlockReferenceBase(const DecodedBlockReferenceBase& o);
  DecodedBlockReferenceBase(DecodedBlockReferenceBase&& o);
  DecodedBlockReferenceBase(const QSharedPointer<DecodedBlock>& block);
  DecodedBlockReferenceBase(QSharedPointer<DecodedBlock>&& block);
  ~DecodedBlockReferenceBase();

  DecodedBlockReferenceBase& operator=(const DecodedBlockReferenceBase& x) =
      delete;

  bool valid() const { return block_; }

  const QSharedPointer<DecodedBlock>& block() const { return block_; }
};

template <class T>
class VOXIEBACKEND_EXPORT DecodedBlockReference
    : public DecodedBlockReferenceBase {
  void checkDataType() {
    if (!block() ||
        block()->dataType() != vx::DataTypeTraitsByType<T>::getDataType())
      raiseMismatchingDataTypeException(
          vx::DataTypeTraitsByType<T>::getDataType());
  }

 public:
  DecodedBlockReference(const DecodedBlockReference& o)
      : DecodedBlockReferenceBase(o) {}
  DecodedBlockReference(DecodedBlockReference&& o)
      : DecodedBlockReferenceBase(o) {}
  explicit DecodedBlockReference(QSharedPointer<DecodedBlock>&& block)
      : DecodedBlockReferenceBase(std::move(block)) {
    checkDataType();
  }
  // TODO: Remove?
  explicit DecodedBlockReference(const QSharedPointer<DecodedBlock>& block)
      : DecodedBlockReferenceBase(block) {
    checkDataType();
  }
  ~DecodedBlockReference() {}

  const T& operator()(const vx::Vector<size_t, 3>& pos) {
    return (*this)(pos[0], pos[1], pos[2]);
  }

  inline const T& operator()(size_t x, size_t y, size_t z) const {
    // auto& info = block()->info();
    /*
    if (x >= info->blockShape()[0] || y >= info->blockShape()[1] ||
        z >= info->blockShape()[2])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Out of bound block array access");
    */
    if (x >= block()->array().template size<0>() ||
        y >= block()->array().template size<1>() ||
        z >= block()->array().template size<2>())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Out of bound block array access");
    return *(T*)(((char*)block()->array().data()) +
                 (x * block()->array().template strideBytes<0>()) +
                 (y * block()->array().template strideBytes<1>()) +
                 (z * block()->array().template strideBytes<2>()));
  }
};

class BlockProvider;

class VOXIEBACKEND_EXPORT BlockProviderOrCache {
 public:
  virtual ~BlockProviderOrCache();

  virtual QSharedPointer<DecodedBlock> getDecodedBlock(
      BlockProvider* provider, const vx::Vector<size_t, 3>& blockId,
      DataType dataType, bool failIfNotInCache = false) = 0;

  template <typename T>
  DecodedBlockReference<T> getDecodedBlockInst(
      BlockProvider* provider, const vx::Vector<size_t, 3>& blockId) {
    return DecodedBlockReference<T>(std::move(getDecodedBlock(
        provider, blockId, vx::DataTypeTraitsByType<T>::getDataType())));
  }
};

class VOXIEBACKEND_EXPORT BlockProvider : public BlockProviderOrCache {
 public:
  virtual ~BlockProvider();

  // TODO: remove?
  virtual QSharedPointer<BlockProviderInfo> info() = 0;

  virtual const vx::Vector<size_t, 3>& arrayShape() = 0;
  virtual const vx::Vector<size_t, 3>& blockShape() = 0;
  virtual const vx::Vector<size_t, 3>& blockCount() = 0;
};
}  // namespace vx
