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

#include "FilenameFilter.hpp"

#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

using namespace vx::io;

FilenameFilter::FilenameFilter(const QString& description,
                               const QStringList& patterns)
    : description_(description), patterns_(patterns) {
  QString patternsStr = " (" + this->patterns().join(" ") + ")";
  filterString_ = this->description() + patternsStr;
  filterStringDouble_ = this->filterString_ + patternsStr;
#if !defined(Q_OS_WIN)
  Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
  Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif
  for (const QString& pattern : patterns_) {
    compiledPatterns_ << QRegExp(pattern, caseSensitivity, QRegExp::Wildcard);
    compiledPatternsCaseInsensitive_
        << QRegExp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
  }
}

FilenameFilter::FilenameFilter(const QVariantMap& filter)
    : FilenameFilter(filter["Description"].toString(),
                     filter["Patterns"].toStringList()) {}

FilenameFilter::~FilenameFilter() {}

FilenameFilter::operator QMap<QString, QDBusVariant>() const {
  QMap<QString, QDBusVariant> filter;
  filter["Description"] = dbusMakeVariant<QString>(description());
  filter["Patterns"] = dbusMakeVariant<QStringList>(patterns());
  return filter;
}

bool FilenameFilter::matches(const QString& filename) const {
  QString filename2 = QFileInfo(filename).fileName();
  for (const QRegExp& regexp : compiledPatterns())
    if (regexp.exactMatch(filename2)) return true;
  return false;
}
bool FilenameFilter::matchesCaseInsensitive(const QString& filename) const {
  QString filename2 = QFileInfo(filename).fileName();
  for (const QRegExp& regexp : compiledPatternsCaseInsensitive())
    if (regexp.exactMatch(filename2)) return true;
  return false;
}

static QString fillPattern(const QString& str) {
  QString res;
  int inSet = 0;
  for (const auto& c : str) {
    if (inSet != 0) {
      if (c == ']') {
        inSet = 0;
      } else if (inSet == 1) {
        // Add first character of set
        res += c;
        inSet = 2;
      } else {
        // Ignore character
      }
    } else {
      if (c == '*') {
        // Do nothing (* are replace by empty string)
      } else if (c == '?') {
        res += 'X';  // Add some arbitrary char
      } else if (c == '[') {
        inSet = 1;
      } else {
        res += c;
      }
    }
  }
  return res;
}
QString FilenameFilter::forceMatch(const QString& file) const {
  // No need to add extension
  if (matches(file)) return file;

  if (patterns().isEmpty()) {
    qWarning() << "FilenameFilter::forceMatch: Empty pattern list";
    return file;
  }

  QFileInfo info(file);
  QString filename = info.fileName();
  QDir dir = info.dir();
  for (const auto& pattern : patterns()) {
    auto pos = pattern.indexOf('*');
    if (pos == -1) continue;
    QString resFilename = fillPattern(pattern.mid(0, pos)) + filename +
                          fillPattern(pattern.mid(pos + 1));
    if (dir.path() == ".")
      return resFilename;
    else
      return dir.filePath(resFilename);
  }
  qWarning() << "FilenameFilter::forceMatch: No pattern with '*' found";
  return file;
}
