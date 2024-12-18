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

#include <QtCore/QMutex>

// TODO: Change the caching / reference counting system so that blocks which are
// kept alive by some reference are also kept in the cache?

namespace vx {
class VOXIEBACKEND_EXPORT BlockCache : public BlockProviderOrCache {
  class CacheEntry;

  QSharedPointer<BlockProviderOrCache> backend;

  QMutex cacheMutex;
  quint64 cacheTimestamp = 0;
  // TODO: Use BlockProvider* here? What happens if the BlockProvider is
  // deleted?
  QMap<std::tuple<BlockProvider*, quint64>, QSharedPointer<CacheEntry>> cache;

  void printStatisticsWithLockMaybe();
  void printStatisticsWithLock();
  quint64 statCacheHits = 0;
  quint64 statCacheMisses = 0;
  quint64 statCacheCleanups = 0;
  quint64 statCacheAll = 0;

 public:
  BlockCache(const QSharedPointer<BlockProviderOrCache>& backend);
  virtual ~BlockCache();

  QSharedPointer<DecodedBlock> getDecodedBlock(
      BlockProvider* provider, const vx::Vector<size_t, 3>& blockId,
      DataType dataType, bool failIfNotInCache = false) final override;

  void invalidateCacheEntry(BlockProvider* provider,
                            const vx::Vector<size_t, 3>& blockId,
                            DataType dataType);

  void printStatistics();
};

VOXIEBACKEND_EXPORT QSharedPointer<BlockCache> defaultBlockCache();
}  // namespace vx
