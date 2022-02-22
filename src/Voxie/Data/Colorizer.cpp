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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "Colorizer.hpp"

#include <VoxieBackend/Data/ImageDataPixel.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/VolumeDataInst.hpp>

#include <cmath>
#include <functional>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QXmlStreamWriter>

#include <QtCore/qsharedpointer.h>
#include <qrgb.h>

#include <Voxie/Data/ColorizerEntry.hpp>
#include <Voxie/Data/Slice.hpp>
#include <VoxieBackend/Data/FloatBuffer.hpp>
#include <VoxieBackend/Data/FloatImage.hpp>

using namespace vx;

int numColorizers = 0;

Colorizer::Colorizer(QObject* parent)
    : QObject(parent), nanColor(qRgba(0, 0, 0, 0)) {
  this->setObjectName("Colorizer" + QString::number(numColorizers++));
}

void Colorizer::clear() {
  entries.clear();
  Q_EMIT mappingChanged();
}

void Colorizer::setEntries(std::vector<ColorizerEntry> entries) {
  // Filter out NaN entries and use them to set the NaN color instead
  for (auto it = entries.begin(); it != entries.end();) {
    if (std::isnan(it->value())) {
      this->nanColor = it->color().asQColor().rgba();
      it = entries.erase(it);
    } else {
      ++it;
    }
  }

  this->entries = std::move(entries);
  Q_EMIT mappingChanged();
}

void Colorizer::setEntries(const QList<ColorizerEntry>& entries) {
  setEntries(toStdVector(entries));
}

std::vector<ColorizerEntry> Colorizer::getEntriesIncludeNaN() const {
  auto entriesWithNan = getEntries();
  entriesWithNan.insert(
      entriesWithNan.begin(),
      ColorizerEntry(std::numeric_limits<double>::quiet_NaN(),
                     QColor(getNanColor()), ColorInterpolator::RGB));
  return entriesWithNan;
}

QList<ColorizerEntry> Colorizer::getEntriesAsQList() const {
  return toQList(getEntries());
}

const std::vector<ColorizerEntry>& Colorizer::getEntries() const {
  return entries;
}

int Colorizer::putMapping(ColorizerEntry entry) {
  auto position =
      entries.insert(std::upper_bound(entries.begin(), entries.end(), entry),
                     std::move(entry));
  Q_EMIT mappingChanged();
  return std::distance(entries.begin(), position);
}

int Colorizer::putMapping(double value, Color color,
                          ColorInterpolator interpolator) {
  return putMapping(ColorizerEntry(value, color, interpolator));
}

void Colorizer::changeMapping(int index, Color color) {
  if (isValidIndex(index)) {
    entries[index] = ColorizerEntry(entries[index].value(), color,
                                    entries[index].interpolator());
    Q_EMIT mappingChanged();
  }
}

void Colorizer::changeValueMapping(int index, double value) {
  if (isValidIndex(index)) {
    entries[index] = ColorizerEntry(value, entries[index].color(),
                                    entries[index].interpolator());
    Q_EMIT mappingChanged();
  }
}

void Colorizer::changeInterpolator(int index, ColorInterpolator interpolator) {
  if (isValidIndex(index)) {
    entries[index] = ColorizerEntry(entries[index].value(),
                                    entries[index].color(), interpolator);
    Q_EMIT mappingChanged();
  }
}

void Colorizer::removeMapping(int index) {
  if (isValidIndex(index)) {
    entries.erase(entries.begin() + index);
    Q_EMIT mappingChanged();
  }
}

bool Colorizer::isValidIndex(int index) const {
  return index >= 0 && std::size_t(index) < entries.size();
}

ColorizerEntry Colorizer::getMapping(int index) const {
  if (entries.empty()) {
    return ColorizerEntry(
        ColorizerEntry(std::numeric_limits<double>::quiet_NaN(),
                       QColor(getNanColor()), ColorInterpolator::RGB));
  } else {
    return entries[std::max<int>(0, std::min<int>(index, entries.size() - 1))];
  }
}

int Colorizer::getEntryCount() const { return entries.size(); }

void Colorizer::setNanColor(QRgb color) {
  this->nanColor = color;
  Q_EMIT mappingChanged();
}

QRgb Colorizer::getNanColor() const { return nanColor; }

void Colorizer::rescale(double sourceMin, double sourceMax, double targetMin,
                        double targetMax) {
  if (qFuzzyCompare(sourceMin, targetMin) &&
      qFuzzyCompare(sourceMax, targetMax)) {
    // Source/dest intervals are almost identical? Do nothing.
    return;
  }

  auto entries = getEntriesIncludeNaN();

  double sourceRange = sourceMax - sourceMin;
  double targetRange = targetMax - targetMin;

  for (auto& entry : entries) {
    entry = ColorizerEntry(
        (entry.value() - sourceMin) / sourceRange * targetRange + targetMin,
        entry.color(), entry.interpolator());
  }

  setEntries(entries);
}

Color Colorizer::getColor(double in) const {
  if (std::isnan(in)) {
    return QColor::fromRgba(nanColor);
  }

  // Draw with default black-to-white gradient if no entries are available
  if (entries.size() == 0) {
    in = (in < 0 ? 0 : in > 1 ? 1 : in);
    return Color(in, in, in, 1.0);
  }

  int length = entries.size();

  int i = -1;
  while (i + 1 < length && entries[i + 1].value() < in) {
    i++;
  }

  if (i < 0) {
    return entries[0].color();
  } else if (i + 1 == length) {
    return entries[i].color();
  } else {
    const auto& e1 = entries[i];
    const auto& e2 = entries[i + 1];

    double in1 = e1.value();
    double in2 = e2.value();
    double rel = (in - in1) / (in2 - in1);

    // Use left color entry's interpolator to interpolate across the interval
    return e1.interpolator().interpolate(e1.color(), e2.color(), rel);
  }
}

void Colorizer::initDefaultMapping() {
  entries.clear();
  entries.emplace_back(0, QColor(0, 0, 0, 255), ColorInterpolator::RGB);
  entries.emplace_back(1, QColor(255, 255, 255, 255), ColorInterpolator::RGB);
  Q_EMIT this->mappingChanged();
}

QPair<QPair<float, QRgb>, QPair<float, QRgb>> Colorizer::initByAlgorithm(
    vx::VolumeNode* nonGenericData) {
  QVector<float> voxelValues = QVector<float>();

  // TODO: Avoid creating a list of all voxels here

  auto volumeData = nonGenericData->volumeData();
  if (volumeData) {
    volumeData->forEachVoxel([&](const auto& value,
                                 const vx::Vector<double, 3>& position,
                                 const vx::Vector<double, 3>& size) {
      (void)position;
      (void)size;
// TODO: Warning suppressed
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"
      voxelValues.append(value);
// TODO: Warning suppressed
#pragma GCC diagnostic pop
    });
  }

  // TODO: int probably is not large enough here, check this
  QMap<float, int> valueMap;
  float lowerbound = 0;
  float upperbound = 0;

  float max, min;
  max = *std::max_element(voxelValues.begin(), voxelValues.end());
  min = *std::min_element(voxelValues.begin(), voxelValues.end());
  // adjusts dynamicly the precision range to 0.1% between the max and the min.
  float precision = (max - min) / 1000;
  for (float i = min; i < max; i += precision) {
    valueMap.insert(i, 0);
  }

  // counts how many values are in the range of precision
  for (int64_t i = 0; i < voxelValues.size(); i++) {
    valueMap.insert(
        (float)((int)(voxelValues.at(i) / precision)) * precision,
        valueMap.value((float)((int)(voxelValues.at(i) / precision)) *
                       precision) +
            1);
  }

  float countedValues = 0;
  for (int64_t i = 0; i < valueMap.keys().size(); i++) {
    countedValues += valueMap.values().at(i);

    if (countedValues >= ((voxelValues.size() / 1000) * 5)) {
      lowerbound = valueMap.keys().at(i);
      break;
    }
  }
  countedValues = 0;
  for (int64_t i = valueMap.keys().size() - 1; i >= 0; i--) {
    countedValues += valueMap.values().at(i);

    if (countedValues >= ((voxelValues.size() / 1000) * 5)) {
      upperbound = valueMap.keys().at(i);
      break;
    }
  }

  return QPair<QPair<float, QRgb>, QPair<float, QRgb>>(
      QPair<float, QRgb>(lowerbound, qRgba(0, 0, 0, 255)),
      QPair<float, QRgb>(upperbound, qRgba(255, 255, 255, 255)));
}

QPair<QPair<float, QRgb>, QPair<float, QRgb>> Colorizer::initByAlgorithm(
    const QSharedPointer<vx::ImageDataPixel>& data) {
  QVector<float> pixelValues = QVector<float>();

  // TODO: support other image types
  auto imageCast =
      qSharedPointerDynamicCast<ImageDataPixelInst<float, 1>>(data);
  if (!imageCast)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Unsupported image type for raw image");

  for (size_t i = 0; i < imageCast->height(); i++) {
    for (size_t j = 0; j < imageCast->width(); j++) {
      pixelValues.append(std::get<0>(imageCast->array()(j, i)));
    }
  }

  // TODO: int probably is not large enough here, check this
  QMap<float, int> valueMap;
  float lowerbound = 0;
  float upperbound = 0;

  // adjusts dynamicly the precision range to 0.1% between the max and the min.
  float precision =
      (pixelValues.at(pixelValues.size() - 1) - pixelValues.at(0)) / 100;
  for (float i = pixelValues.at(0); i < pixelValues.at(pixelValues.size() - 1);
       i += precision) {
    valueMap.insert(i, 0);
  }

  // counts how many values are in the range of precision
  for (int64_t i = 0; i < pixelValues.size(); i++) {
    valueMap.insert(
        (float)((int)(pixelValues.at(i) / precision)) * precision,
        valueMap.value((float)((int)(pixelValues.at(i) / precision)) *
                       precision) +
            1);
  }

  float countedValues = 0;
  for (int64_t i = 0; i < valueMap.keys().size(); i++) {
    countedValues += valueMap.values().at(i);

    if (countedValues >= (pixelValues.size() / 1000 * 5)) {
      lowerbound = valueMap.keys().at(i);
      break;
    }
  }
  countedValues = 0;
  for (int64_t i = valueMap.keys().size() - 1; i >= 0; i--) {
    countedValues += valueMap.values().at(i);

    if (countedValues >= (pixelValues.size() / 1000 * 5)) {
      upperbound = valueMap.keys().at(i);
      break;
    }
  }
  return QPair<QPair<float, QRgb>, QPair<float, QRgb>>(
      QPair<float, QRgb>(lowerbound, qRgba(0, 0, 0, 255)),
      QPair<float, QRgb>(upperbound, qRgba(255, 255, 255, 255)));
}

void Colorizer::toXML(QIODevice* ioDevice) const {
  if (!ioDevice->isOpen()) {
    if (!ioDevice->open(QIODevice::WriteOnly)) {
      qDebug() << "cannot open ioDevice for xml writing";
      return;
    }
  } else {
    if (ioDevice->openMode() == QIODevice::ReadOnly) {
      qDebug()
          << "cannot write to ioDevice for xml writing, is opened read only";
      return;
    } else {
      qDebug()
          << "ioDevice already open, writing xml may have undesired effect";
    }
  }

  QXmlStreamWriter xml(ioDevice);

  auto writeMapping = [&xml](double key, QRgb value, int interpolator) {
    xml.writeStartElement("key");
    xml.writeCharacters(QString::number(key, 'f', 8));
    xml.writeEndElement();
    xml.writeStartElement("value");
    xml.writeCharacters(QString::number(value, 16));
    xml.writeEndElement();
    xml.writeStartElement("interpolator");
    xml.writeCharacters(QString::number(interpolator));
    xml.writeEndElement();
  };

  xml.writeStartDocument();
  xml.writeDTD("<!DOCTYPE colorizer>");

  xml.writeStartElement("colorizermap");

  xml.writeStartElement("nanmapping");
  xml.writeStartElement("value");
  xml.writeCharacters(QString::number(this->getNanColor()));
  xml.writeEndElement();
  xml.writeEndElement();

  for (const auto& entry : entries) {
    xml.writeStartElement("mapping");
    writeMapping(entry.value(), entry.color().asQColor().rgba(),
                 entry.interpolator().getType());
    xml.writeEndElement();
  }

  xml.writeEndElement();
  xml.writeEndDocument();
  ioDevice->close();
}

void Colorizer::toXML(QString filename) const {
  QFile file(filename);
  if (file.exists()) {
    toXML(&file);
  }
}

bool Colorizer::loadFromXML(QIODevice* ioDevice) {
  if (!ioDevice->isOpen()) {
    if (!ioDevice->open(QIODevice::ReadOnly)) {
      qDebug() << "cannot open ioDevice for xml reading";
      return false;
    }
  } else if (ioDevice->openMode() == QIODevice::WriteOnly) {
    qDebug()
        << "cannot write to ioDevice for xml reading, is opened write only";
    return false;
  }

  QXmlStreamReader xml(ioDevice);

  float key = 0;
  QRgb value = 0;
  bool nanMapping = false;
  bool readKey = false;
  bool readValue = false;
  bool readInterpolator = false;
  while (!xml.atEnd()) {
    QXmlStreamReader::TokenType token = xml.readNext();
    if (token == QXmlStreamReader::StartElement) {
      QString name = xml.name().toString();
      // qDebug() << name;
      if (name == "nanmapping") {
        nanMapping = true;
      } else if (name == "mapping") {
        nanMapping = false;
      } else if (name == "key") {
        key = xml.readElementText().toFloat();
        readKey = true;
      } else if (name == "value") {
        value = xml.readElementText().toUInt(nullptr, 16);
        readValue = true;
      } else if (name == "interpolator") {
        value = static_cast<ColorInterpolator::InterpolationType>(
            xml.readElementText().toInt(nullptr));
        readInterpolator = true;
      }
    }
    // --
    if (nanMapping && readValue) {
      setNanColor(value);
      readKey = readValue = readInterpolator = false;
    } else if (readValue && readKey) {
      putMapping(key, QColor(value));
      readKey = readValue = readInterpolator = false;
    }
  }

  ioDevice->close();
  return true;
}

bool Colorizer::loadFromXML(QString filename) {
  QFile file(filename);
  if (file.exists()) {
    return loadFromXML(&file);
  } else {
    return false;
  }
}

Colorizer* Colorizer::fromXML(QIODevice* ioDevice) {
  Colorizer* colorizer = new Colorizer;
  if (colorizer->loadFromXML(ioDevice)) {
    return colorizer;
  } else {
    delete colorizer;
    return nullptr;
  }
}

QImage Colorizer::toQImageGray(const FloatImage& image, float lowestValue,
                               float highestValue) {
  Colorizer* colorizer = new Colorizer();
  colorizer->putMapping(lowestValue, QColor(0, 0, 0));
  colorizer->putMapping(highestValue, QColor(255, 255, 255));
  QImage img = colorizer->toQImage(image);
  delete colorizer;
  return img;
}

QImage Colorizer::toQImage(const FloatImage& image) {
  if (this->getEntryCount() < 1) {
    return toQImageGray(image);
  }

  QImage qimage(image.getDimension(), QImage::Format_ARGB32);
  QRgb* qimgbuffer = (QRgb*)qimage.bits();
  //
  bool oclFailed = false;
  if (image.imageData->mode == FloatImage::CLMEMORY_MODE) {
    // ocl
    try {
      opencl::CLInstance* clInstance = image.imageData->clInstance;
      Q_ASSERT(clInstance != nullptr);
      if (!clInstance)
        throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                            "clInstance == nullptr");

      QString progID("colorizer");
      if (!clInstance->hasProgramID(progID)) {
        clInstance->createProgramFromFile(":/cl_kernels/colorizer.cl", "",
                                          progID);
      }

      cl::Kernel kernel = clInstance->getKernel(progID, "colorize");
      cl::Buffer qimgCLBuffer = clInstance->createBuffer(
          qimage.width() * qimage.height() * sizeof(QRgb), qimgbuffer);

      QVector<float> colorKeys;
      QVector<QRgb> colorValues;
      for (const auto& entry : this->getEntries()) {
        colorKeys.append(entry.value());
        colorValues.append(entry.color().asQColor().rgba());
      }

      cl::Buffer colorKeysCL = clInstance->createBuffer(
          colorKeys.length() * sizeof(float), colorKeys.data());
      cl::Buffer colorValuesCL = clInstance->createBuffer(
          colorValues.length() * sizeof(QRgb), colorValues.data());

      // cl_int error;
      cl::Buffer tempBuffer = image.imageData->clPixels;
      // error =
      kernel.setArg(0, tempBuffer);
      // error =
      kernel.setArg(1, qimgCLBuffer);
      // error =
      kernel.setArg(2, colorKeysCL);
      // error =
      kernel.setArg(3, colorValuesCL);
      // error =
      kernel.setArg<cl_int>(4, (cl_int)colorKeys.length());
      // error =
      kernel.setArg<cl_uint>(5, (cl_uint)this->getNanColor());
      // Q_UNUSED(error);

      clInstance->executeKernel(
          kernel, cl::NDRange(image.getWidth() * image.getHeight()));
      // read back
      clInstance->readBuffer(qimgCLBuffer, qimgbuffer);

    } catch (opencl::CLException& ex) {
      qWarning() << ex;
      oclFailed = true;
    } catch (opencl::IOException& ex) {
      qWarning() << ex;
      oclFailed = true;
    }
  }
  if (oclFailed || image.getMode() == FloatImage::STDMEMORY_MODE) {
    //
    FloatBuffer buffer = image.getBufferCopy();
    for (size_t i = 0; i < buffer.length(); i++) {
      float value = buffer[i];
      QRgb color = this->getColor(value).asQColor().rgba();
      qimgbuffer[i] = color;
    }
  }

  // Mirror y axis (the y axis of FloatImage goes from bottom to top, the y axis
  // of QImage goes from top to bottom)
  for (std::size_t y = 0; y < (std::size_t)qimage.height() / 2; y++) {
    for (std::size_t x = 0; x < (std::size_t)qimage.width(); x++) {
      std::size_t y2 = qimage.height() - 1 - y;
      std::swap(qimgbuffer[y * qimage.width() + x],
                qimgbuffer[y2 * qimage.width() + x]);
    }
  }

  return qimage;
}
