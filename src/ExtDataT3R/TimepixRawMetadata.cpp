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

#include "TimepixRawMetadata.hpp"

#include "EventListTypes.hpp"

#include <VoxieClient/Exception.hpp>

#include <QJsonArray>

namespace vx {
namespace t3r {

void TimepixRawStreamMetadata::read(const QJsonObject& json) {
  if (json["Data"].isArray()) {
    auto parsedDataArr = json["Data"].toArray();
    data.clear();
    for (auto curData : parsedDataArr) {
      if (curData.isString()) {
        data.push_back(curData.toString());
      }
    }
  }

  // Pass on metadata (without 'Data' object)
  metadata = json;
  metadata.remove("Data");

  // Add default fixed-size bounding box to contain all activation events
  metadata["BoundingBox"] =
      QJsonArray({0, 0, int(MatrixColumnCount), int(MatrixRowCount)});
  // Pixel size is always 1 for raw data
  metadata["PixelSize"] = 1.0;
}

const std::vector<QString>& TimepixRawStreamMetadata::getData() const {
  return data;
}

const QJsonObject& TimepixRawStreamMetadata::getMetadata() const {
  return metadata;
}

void TimepixRawMetadata::read(const QJsonObject& json) {
  if (json["Type"].isString()) {
    type = json["Type"].toString();
  } else {
    throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R.Error",
                        "Missing type declaration in Raw Timepix JSON file");
  }

  if (json["Detectors"].isArray()) {
    auto parsed = json["Detectors"].toArray();
    detectors.clear();
    for (int i = 0; i < parsed.size(); ++i) {
      if (parsed[i].isObject()) {
        detectors.push_back(parsed[i].toObject());
      } else {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtDataT3R.Error",
            QStringLiteral("Detector %1 is not a JSON object").arg(i));
      }
    }
  } else {
    throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R.Error",
                        "Missing detectors in Raw Timepix JSON file");
  }

  if (json["Streams"].isArray()) {
    auto parsed = json["Streams"].toArray();
    streams.clear();
    for (int i = 0; i < parsed.size(); ++i) {
      if (parsed[i].isObject()) {
        TimepixRawStreamMetadata stream;
        stream.read(parsed[i].toObject());

        if (stream.getData().size() != detectors.size()) {
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.ExtDataT3R.Error",
              QStringLiteral("Stream %1 has a mismatched number of data files "
                             "(expected %2, got %3)")
                  .arg(i)
                  .arg(detectors.size())
                  .arg(stream.getData().size()));
        }

        streams.push_back(stream);
      } else {
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.ExtDataT3R.Error",
            QStringLiteral("Stream %1 is not a JSON object").arg(i));
      }
    }
  } else {
    throw vx::Exception("de.uni_stuttgart.Voxie.ExtDataT3R.Error",
                        "Missing streams in Raw Timepix JSON file");
  }

  // Pass on metadata (without 'Streams' object)
  metadata = json;
  metadata.remove("Streams");
}

const QString& TimepixRawMetadata::getType() const { return type; }

const std::vector<QJsonObject>& TimepixRawMetadata::getDetectors() const {
  return detectors;
}

const std::vector<TimepixRawStreamMetadata>& TimepixRawMetadata::getStreams()
    const {
  return streams;
}

std::size_t TimepixRawMetadata::getTotalStreamCount() const {
  std::size_t streamCount = 0;
  for (auto& stream : getStreams()) {
    streamCount += stream.getData().size();
  }
  return streamCount;
}

const QJsonObject& TimepixRawMetadata::getMetadata() const { return metadata; }

}  // namespace t3r
}  // namespace vx
