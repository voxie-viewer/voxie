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

#include <VoxieBackend/Data/BlockProvider.hpp>

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieClient/Optional.hpp>
#include <VoxieClient/Vector.hpp>

#include <QtCore/QSharedPointer>

namespace vx {
class BlockProvider;
class BlockProviderOrCache;

// This class provides a cache which contains a rectangular area of blocks of a
// single volume.
// The class is not thread-safe.
class VOXIEBACKEND_EXPORT SlidingBlockCache {
  Q_DISABLE_COPY(SlidingBlockCache)

 public:
  // TODO: Support different types?
  using VoxelType = float;

  // TODO: Make this dynamic?
  static const size_t count = 2;

 private:
  QSharedPointer<BlockProvider> provider;
  QSharedPointer<BlockProviderOrCache> cache;

  // TODO: Remove arrayShape?
  vx::Vector<size_t, 3> arrayShape;
  vx::Vector<size_t, 3> blockShape;
  vx::Vector<size_t, 3> blockCount;

  vx::Vector<size_t, 3> base = {0, 0, 0};
  QSharedPointer<DecodedBlock> cached[count][count][count];

  Q_NORETURN void throwError();

  void shiftBase(const vx::Vector<size_t, 3>& newBase);

  const QSharedPointer<DecodedBlock>& getBlock(
      const vx::Vector<size_t, 3>& blockId) {
    vx::Vector<size_t, 3> newBase = base;
    for (size_t dim = 0; dim < 3; dim++) {
      if (blockId[dim] < base[dim]) {
        newBase[dim] = blockId[dim];
      } else if (blockId[dim] >= base[dim] + count) {
        newBase[dim] = blockId[dim] + 1 - count;
      }
    }

    if (newBase != base) {
      shiftBase(newBase);
    }
    vx::Vector<size_t, 3> rel = blockId - base;
    for (size_t dim = 0; dim < 3; dim++)
      if (rel[dim] >= count) throwError();

    QSharedPointer<DecodedBlock>& ref = cached[rel[0]][rel[1]][rel[2]];
    if (!ref)
      ref = cache->getDecodedBlock(
          provider.data(), blockId,
          vx::DataTypeTraitsByType<float>::getDataType());
    // TODO: Add a check whether ref is the correct block?
    return ref;
  }

 public:
  SlidingBlockCache(const QSharedPointer<BlockProvider>& provider,
                    const QSharedPointer<BlockProviderOrCache>& cache);
  virtual ~SlidingBlockCache();

  VoxelType getVoxelUnchecked(const vx::Vector<size_t, 3>& pos) {
    size_t x = pos[0];
    size_t y = pos[1];
    size_t z = pos[2];

    vx::Vector<size_t, 3> blockId = {
        x / blockShape[0],
        y / blockShape[1],
        z / blockShape[2],
    };
    vx::Vector<size_t, 3> blockOffset = {
        x % blockShape[0],
        y % blockShape[1],
        z % blockShape[2],
    };
    const auto& block = getBlock(blockId);
    // TODO: Avoid type check etc.?
    DecodedBlockReference<float> blockTyped(block);
    return blockTyped(blockOffset);
  }
};
}  // namespace vx
