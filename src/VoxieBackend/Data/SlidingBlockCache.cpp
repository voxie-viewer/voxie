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

#include "SlidingBlockCache.hpp"

#include <VoxieBackend/Data/BlockProvider.hpp>

vx::SlidingBlockCache::SlidingBlockCache(
    const QSharedPointer<BlockProvider>& provider,
    const QSharedPointer<BlockProviderOrCache>& cache)
    : provider(provider) {
  if (!provider)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "provider is null");

  this->cache = cache;
  if (!this->cache) this->cache = this->provider;

  this->arrayShape = this->provider->arrayShape();
  this->blockShape = this->provider->blockShape();
  this->blockCount = this->provider->blockCount();
}
vx::SlidingBlockCache::~SlidingBlockCache() {}

Q_NORETURN void vx::SlidingBlockCache::throwError() {
  throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                      "SlidingBlockCache: Internal error");
}

// TODO: Can this code be cleaned up?
void vx::SlidingBlockCache::shiftBase(const vx::Vector<size_t, 3>& newBase) {
  bool doReset = false;
  for (size_t dim = 0; dim < 3; dim++) {
    if (newBase[dim] < base[dim]) {
      if (base[dim] - newBase[dim] >= count) doReset = true;
    } else {
      if (newBase[dim] - base[dim] >= count) doReset = true;
    }
  }

  if (doReset) {
    for (size_t x = 0; x < count; x++)
      for (size_t y = 0; y < count; y++)
        for (size_t z = 0; z < count; z++)
          cached[x][y][z] = QSharedPointer<DecodedBlock>();
    base = newBase;
  }

  vx::Vector<ptrdiff_t, 3> diff =
      vectorCastNarrow<ptrdiff_t>(newBase) - vectorCastNarrow<ptrdiff_t>(base);

  QSharedPointer<DecodedBlock> old[count][count][count];
  for (size_t x = 0; x < count; x++)
    for (size_t y = 0; y < count; y++)
      for (size_t z = 0; z < count; z++) old[x][y][z] = cached[x][y][z];

  for (ptrdiff_t x = 0; x < (ptrdiff_t)count; x++) {
    for (ptrdiff_t y = 0; y < (ptrdiff_t)count; y++) {
      for (ptrdiff_t z = 0; z < (ptrdiff_t)count; z++) {
        vx::Vector<ptrdiff_t, 3> pos{x, y, z};
        vx::Vector<ptrdiff_t, 3> res = pos + diff;

        bool found = true;
        for (size_t dim = 0; dim < 3; dim++) {
          if (res[dim] < 0 || res[dim] >= (ptrdiff_t)count) found = false;
        }

        if (found) {
          cached[x][y][z] = old[res[0]][res[1]][res[2]];
        } else {
          cached[x][y][z] = QSharedPointer<DecodedBlock>();
        }
      }
    }
  }
  base = newBase;
}
