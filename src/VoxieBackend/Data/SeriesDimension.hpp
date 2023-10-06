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

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <VoxieBackend/VoxieBackend.hpp>

namespace vx {
class PropertyType;

class VOXIEBACKEND_EXPORT SeriesDimension : public RefCountedObject {
  Q_OBJECT
  REFCOUNTEDOBJ_DECL(SeriesDimension)

  // A SeriesDimension could be e.g.:
  // - Time
  // - Spectral distibution

  QString name_;
  QString displayName_;
  QSharedPointer<PropertyType> type_;

  // The raw values, see type() for interpretation
  // The entries are ordered in the way in which they will be shown in the GUI.
  // Entries are identified using an integer (EntryKey).
  // Note: this cannot be changed without creating a new SeriesData object.
  // Otherwise the EntryKey (an integer) would not be able to identify values
  // reliably (especially if entries are supposed to be sorted).
  QList<QVariant> entries_;

 public:
  using EntryKey = quint64;
  static const constexpr EntryKey invalidEntryKey =
      std::numeric_limits<quint64>::max();

  QString name() { return name_; }
  QString displayName() { return displayName_; }
  QSharedPointer<PropertyType> type() { return type_; }
  QList<QVariant> entries() { return entries_; }
  EntryKey length() { return entries().length(); }

 public:
  // TODO: should this be private so that only the create() method can call it?
  SeriesDimension(const QString& name, const QString& displayName,
                  const QSharedPointer<PropertyType>& type,
                  const QList<QVariant>& entries);

  QVariant getEntryValue(EntryKey key);
  EntryKey lookupEntryByValueOrInvalid(const QVariant& value);

 public:
  ~SeriesDimension();
};
}  // namespace vx
