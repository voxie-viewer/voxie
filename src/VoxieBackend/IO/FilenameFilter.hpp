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

#include <VoxieBackend/VoxieBackend.hpp>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariantMap>

#include <QtDBus/QDBusVariant>

namespace vx {
namespace io {

class VOXIEBACKEND_EXPORT FilenameFilter {
  QString description_;
  QStringList patterns_;
  QList<QRegExp> compiledPatterns_;
  QList<QRegExp> compiledPatternsCaseInsensitive_;

  QString filterString_;
  QString filterStringDouble_;

 public:
  FilenameFilter() {}
  FilenameFilter(const QString& description, const QStringList& patterns);
  FilenameFilter(const QVariantMap& filter);
  operator QMap<QString, QDBusVariant>() const;
  ~FilenameFilter();

  const QString& description() const { return description_; }
  const QStringList& patterns() const { return patterns_; }
  const QList<QRegExp>& compiledPatterns() const { return compiledPatterns_; }
  const QList<QRegExp>& compiledPatternsCaseInsensitive() const {
    return compiledPatternsCaseInsensitive_;
  }

  // "Description (*.ext *.x)"
  const QString& filterString() const { return filterString_; }
  // "Description (*.ext *.x) (*.ext *.x)"
  const QString& filterStringDouble() const { return filterStringDouble_; }

  bool matches(const QString& filename) const;
  bool matchesCaseInsensitive(const QString& filename) const;

  // Add the filter extension to the filename
  QString addExtension(const QString& file) const;
};

}  // namespace io
}  // namespace vx

Q_DECLARE_METATYPE(vx::io::FilenameFilter)
