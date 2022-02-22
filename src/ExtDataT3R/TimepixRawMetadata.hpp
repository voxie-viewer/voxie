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

#include <QJsonObject>

#include <array>
#include <string>
#include <vector>

namespace vx {
namespace t3r {

struct TimepixRawStreamMetadata {
 public:
  TimepixRawStreamMetadata() = default;

  void read(const QJsonObject& json);

  const std::vector<QString>& getData() const;
  const QJsonObject& getMetadata() const;

 private:
  std::vector<QString> data;
  QJsonObject metadata;
};

struct TimepixRawMetadata {
 public:
  TimepixRawMetadata() = default;

  void read(const QJsonObject& json);

  const QString& getType() const;
  const std::vector<QJsonObject>& getDetectors() const;
  const std::vector<TimepixRawStreamMetadata>& getStreams() const;

  std::size_t getTotalStreamCount() const;

  const QJsonObject& getMetadata() const;

 private:
  QString type;
  std::vector<QJsonObject> detectors;
  std::vector<TimepixRawStreamMetadata> streams;

  QJsonObject metadata;
};

}  // namespace t3r
}  // namespace vx
