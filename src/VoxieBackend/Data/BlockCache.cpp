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

#include "BlockCache.hpp"

#include <VoxieClient/Format.hpp>

#include <VoxieBackend/DebugOptions.hpp>

class vx::BlockCache::CacheEntry {
  friend class vx::BlockCache;

 private:
  // TODO: This probably should not use a maybe-freed pointer
  BlockProvider* providerMaybeFreed;
  quint64 id;

  std::tuple<BlockProvider*, quint64> key() {
    return std::make_tuple(providerMaybeFreed, id);
  }

  quint64 timestamp = 0;

  QMutex initializationLock;
  bool isInitialized = false;

  QSharedPointer<DecodedBlock> block;

  CacheEntry(BlockProvider* provider, quint64 id)
      : providerMaybeFreed(provider), id(id) {}
};

vx::BlockCache::BlockCache(const QSharedPointer<BlockProviderOrCache>& backend)
    : backend(backend) {}
vx::BlockCache::~BlockCache() {}

QSharedPointer<vx::DecodedBlock> vx::BlockCache::getDecodedBlock(
    BlockProvider* provider, const vx::Vector<size_t, 3>& blockId,
    DataType dataType, bool failIfNotInCache) {
  if (!provider)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "provider is null");

  // TODO: How should these value be determined? Should this be based on the
  // overall number of blocks?
  // TODO: This probably should be dynamic in some way and should track the
  // volumes which are currently in this cache
  std::size_t maxEntryCount = 10000;
  std::size_t maxEntryCountAdd = maxEntryCount / 10;

  // Note: This would require keeping track of the data type in the cache also
  // (or implement converting between different decoded types)
  if (dataType != vx::DataTypeTraitsByType<float>::getDataType())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.NotImplemented",
        "Not implemented: getDecodedBlock() currently only supports float");

  auto blockCount = provider->blockCount();

  for (std::size_t i = 0; i < 3; i++) {
    if (blockId[i] >= blockCount[i])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
  }
  quint64 pos =
      blockId[0] + blockCount[0] * (blockId[1] + blockCount[1] * blockId[2]);

  auto key = std::make_tuple(provider, pos);

  QSharedPointer<CacheEntry> entry;
  {
    QMutexLocker locker(&cacheMutex);
    if (!cache.contains(key)) {
      // TODO: Should this be considered a miss for statistics purposes or
      // should this be ignored?
      if (failIfNotInCache) return QSharedPointer<DecodedBlock>();

      statCacheMisses++;
      statCacheAll++;
      // qDebug() << "cache miss";
      if ((size_t)cache.size() > maxEntryCount + maxEntryCountAdd) {
        statCacheCleanups++;
        // qDebug() << "cleanup" << cache.size() << maxEntryCount <<
        // maxEntryCountAdd; Remove entries from cache
        // TODO: Make this faster / use better LRU structure
        QList<QSharedPointer<CacheEntry>> entries = cache.values();
        std::sort(entries.begin(), entries.end(),
                  [](const QSharedPointer<CacheEntry>& b1,
                     const QSharedPointer<CacheEntry>& b2) {
                    return b1->timestamp < b2->timestamp;
                  });
        for (size_t i = 0; i + maxEntryCount < (size_t)entries.size(); i++) {
          // qDebug() << "cleanup: " << i << std::get<0>(entries[i]->key()) <<
          // std::get<1>(entries[i]->key());
          cache.remove(entries[i]->key());
        }
        // qDebug() << "after cleanup" << cache.size() << entries.size();
        if ((size_t)cache.size() != maxEntryCount)
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.InternalError",
              format("Cache cleanup failed: Got {} entries, expected {}",
                     cache.size(), maxEntryCount));
      }
      printStatisticsWithLockMaybe();

      entry = QSharedPointer<CacheEntry>(new CacheEntry(provider, pos));

      cacheTimestamp++;
      entry->timestamp = cacheTimestamp;

      cache[key] = entry;
    } else {
      statCacheHits++;
      statCacheAll++;
      printStatisticsWithLockMaybe();

      entry = cache[key];

      cacheTimestamp++;
      entry->timestamp = cacheTimestamp;
    }
  }
  if (!entry) {
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "entry is null");
  }

  {
    QMutexLocker locker(&entry->initializationLock);

    if (!entry->isInitialized) {
      BlockProviderOrCache* p = backend.data();
      if (!p) p = provider;

      entry->block = p->getDecodedBlock(provider, blockId, dataType, false);
      if (!entry->block)
        throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                            "entry->block is null");

      entry->isInitialized = true;
    }
  }

  return entry->block;
}

void vx::BlockCache::invalidateCacheEntry(BlockProvider* provider,
                                          const vx::Vector<size_t, 3>& blockId,
                                          DataType dataType) {
  // Note: This would require keeping track of the data type in the cache also
  // (or implement converting between different decoded types)
  if (dataType != vx::DataTypeTraitsByType<float>::getDataType())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.NotImplemented",
        "Not implemented: getDecodedBlock() currently only supports float");

  auto blockCount = provider->blockCount();
  for (std::size_t i = 0; i < 3; i++) {
    if (blockId[i] >= blockCount[i])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
  }
  quint64 pos =
      blockId[0] + blockCount[0] * (blockId[1] + blockCount[1] * blockId[2]);

  auto key = std::make_tuple(provider, pos);

  {
    QMutexLocker locker(&cacheMutex);
    cache.remove(key);
  }
}

void vx::BlockCache::printStatisticsWithLockMaybe() {
  if (statCacheAll >= 10000) printStatisticsWithLock();
}

void vx::BlockCache::printStatisticsWithLock() {
  if (!vx::debug_option::Log_BlockCache_Statistics()->get()) return;

  // if (statCacheAll == 0) return;

  qDebug() << format(
      "BlockCache: Got {} hits, {} misses, {} cleanups, {} total",
      statCacheHits, statCacheMisses, statCacheCleanups, statCacheAll);

  statCacheHits = 0;
  statCacheMisses = 0;
  statCacheCleanups = 0;
  statCacheAll = 0;
}

void vx::BlockCache::printStatistics() {
  if (!vx::debug_option::Log_BlockCache_Statistics()->get()) return;

  {
    QMutexLocker locker(&cacheMutex);
    printStatisticsWithLock();
  }
}

QSharedPointer<vx::BlockCache> vx::defaultBlockCache() {
  // Note: This will currently never be freed.
  // TODO: Should it be freed?
  static QSharedPointer<BlockCache> cache =
      createQSharedPointer<BlockCache>(QSharedPointer<BlockProviderOrCache>());
  return cache;
}
