/*
 * Copyright (c) 2014-2023 The Voxie Authors
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

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieBackend/Data/Data.hpp>
#include <VoxieBackend/Data/ReplaceMode.hpp>
#include <VoxieBackend/Data/SeriesDimension.hpp>

#include <algorithm>

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
// This operator does not exist in Qt 5.5
template <typename T>
static inline bool operator<(const QList<T>& l1, const QList<T>& l2) {
  return std::lexicographical_compare(l1.begin(), l2.end(), l1.begin(),
                                      l2.end());
}
#endif

namespace vx {

class VOXIEBACKEND_EXPORT SeriesData : public Data, public DataContainer {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(SeriesData)

 public:
  using EntryKeyList = QList<SeriesDimension::EntryKey>;

 private:
  QList<QSharedPointer<SeriesDimension>> dimensions_;
  QMap<QString, QSharedPointer<SeriesDimension>> dimensionsByName_;

  QMap<EntryKeyList, QSharedPointer<Data>> entries_;

 public:
  const QList<QSharedPointer<SeriesDimension>>& dimensions() {
    return dimensions_;
  }
  const QMap<QString, QSharedPointer<SeriesDimension>>& dimensionsByName() {
    return dimensionsByName_;
  }
  int dimensionCount() { return dimensions().length(); }

  const QMap<EntryKeyList, QSharedPointer<Data>> entries() {
    return entries_;
  }

  void verifyKey(const EntryKeyList& key);

  void addEntry(const EntryKeyList& key, const QSharedPointer<Data>& data,
                ReplaceMode replaceMode = ReplaceMode::Insert);

  // Returns null if there is no such entry
  QSharedPointer<Data> lookupEntry(const EntryKeyList& key);
  // Throws if there is no such entry
  QSharedPointer<Data> lookupEntryRequired(const EntryKeyList& key);

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;

 protected:
  SeriesData(const QList<QSharedPointer<SeriesDimension>>& dimensions);

  ~SeriesData();

  virtual void checkNewData(const QSharedPointer<Data>& data) = 0;
};
}  // namespace vx
