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

#include "FilterChain2D.hpp"

#include <Voxie/IVoxie.hpp>

#include <Voxie/Component/MetaFilter2D.hpp>
#include <Voxie/Component/Plugin.hpp>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QThreadPool>
#include <QtCore/QtMath>

#include <QtWidgets/QMessageBox>

using namespace vx::filter;
using namespace vx;
using namespace vx::plugin;

FilterChain2D::~FilterChain2D() {}

void FilterChain2D::applyTo(FloatImage slice) {
  Filter2D* activeFilter;
  for (int x = 0; x < this->filters.length(); x++) {
    activeFilter = this->filters.at(x);
    if (activeFilter->isEnabled()) {
      slice = activeFilter->applyToCopy(slice);
    }
  }
  this->outputFloatImage = slice;
  Q_EMIT this->allFiltersApplied();  // signals that chain has finished work so
                                     // that application can get output
}

void FilterChain2D::applyTo(SliceImage slice) {
  Filter2D* activeFilter;
  for (int x = 0; x < this->filters.length(); x++) {
    activeFilter = this->filters.at(x);
    if (activeFilter->isEnabled()) {
      slice = activeFilter->applyToCopy(slice);
    }
  }
  this->outputSliceImage = slice;
  Q_EMIT this->allFiltersApplied();  // signals that chain has finished work so
                                     // that application can get output
}

void FilterChain2D::addFilter(Filter2D* filter) {
  lastID++;
  filter->filterID = lastID;

  this->filters.append(filter);
  connect(filter, &Filter2D::filterChanged, this,
          &FilterChain2D::filterChanged);
  Q_EMIT this->filterListChanged();
}

void FilterChain2D::removeFilter(Filter2D* filter) {
  int filterPos = this->filters.indexOf(filter, 0);
  disconnect(filter, &Filter2D::filterChanged, this,
             &FilterChain2D::filterChanged);
  this->filters.remove(filterPos);
  Q_EMIT this->filterListChanged();
}

void FilterChain2D::changePosition(Filter2D* filter, int pos) {
  int filterPos = this->filters.indexOf(filter, 0);
  if (filterPos != -1) {
    this->filters.remove(filterPos);
    this->filters.insert(pos, filter);
    Q_EMIT this->filterListChanged();
  }
}

QVector<Filter2D*> FilterChain2D::getFilters() { return this->filters; }

Filter2D* FilterChain2D::getFilter(int pos) { return this->filters.at(pos); }

Filter2D* FilterChain2D::getFilterByIDOrNull(quint64 id) {
  for (const auto& filter : this->filters)
    if (filter->filterID == id) return filter;
  return nullptr;
}

FloatImage& FilterChain2D::getOutputImage() { return this->outputFloatImage; }

SliceImage& FilterChain2D::getOutputSlice() { return this->outputSliceImage; }

void FilterChain2D::onPlaneChanged(const Slice* slice,
                                   const vx::PlaneInfo& oldPlane,
                                   const vx::PlaneInfo& newPlane,
                                   bool equivalent) {
  Q_UNUSED(slice);  // TODO: slice parameter seems to be unused (both by this
                    // function and by other functions which are connected to
                    // Slice::planeChanged()), should probably be removed
  if (equivalent) {
    if (oldPlane.origin != newPlane.origin) {
      // translation
      QPointF difference = newPlane.get2DPlanePoint(oldPlane.origin);
      for (Filter2D* filter : this->filters) {
        filter->getMask()->translateOrigin(difference.x(), difference.y());
      }
    }
    if ((oldPlane.normal() - newPlane.normal()).lengthSquared() < 1e-5 &&
        oldPlane.tangent() != newPlane.tangent()) {
      qreal cos = QVector3D::dotProduct(oldPlane.tangent(), newPlane.tangent());
      qreal angle = qRadiansToDegrees(qAcos(cos));
      if (std::isnan(angle)) {
        return;
      }
      QPointF oldXAxis =
          newPlane.get2DPlanePoint(newPlane.origin + oldPlane.tangent());
      angle =
          angle * (oldXAxis.y() > 0 ? 1 : -1);  // clockwise or counterclockwise
      for (Filter2D* filter : this->filters) {
        filter->getMask()->rotate(angle);
      }
    }
  }
}

void FilterChain2D::toXML(QXmlStreamWriter* xml) {
  xml->setAutoFormatting(true);

  xml->writeStartDocument();
  xml->writeDTD("<!DOCTYPE filterchain>");
  xml->writeStartElement("filterchain2d");
  xml->writeAttribute("version", "1.0");

  for (int i = 0; i < this->getFilters().size(); i++) {
    xml->writeStartElement(this->getFilter(i)->getMetaName());
    xml->writeAttribute("type", "filter2d");
    xml->writeAttribute(
        "selectionJson",
        QString::fromUtf8(
            QJsonDocument(this->getFilter(i)->getMask()->getMaskJson())
                .toJson()));
    xml->writeAttribute("filterID",
                        QString::number(this->getFilter(i)->filterID));
    xml->writeAttribute("enabled",
                        this->getFilter(i)->isEnabled() ? "true" : "false");
    xml->writeAttributes(this->getFilter(i)->exportFilterSettingsXML());
    xml->writeEndElement();
  }

  xml->writeEndDocument();
}

void FilterChain2D::toXML(QString fileName) {
  QFile file(fileName);
  file.open(QIODevice::WriteOnly);

  QXmlStreamWriter xml;
  xml.setDevice(&file);

  toXML(&xml);

  file.flush();
  file.close();
}

QString FilterChain2D::toXMLString() {
  QString result;
  QXmlStreamWriter xml(&result);

  toXML(&xml);

  return result;
}

void FilterChain2D::fromXML(QXmlStreamReader* xml) {
  if (xml->readNextStartElement()) {
    if (!(xml->name() == "filterchain2d" &&
          xml->attributes().value("version") == "1.0")) {
      xml->raiseError("The file is no valid filterchain.");
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                          "The file is no valid filterchain.");
    }
  }

  this->filters.clear();
  quint64 maxID = 0;
  while (xml->readNextStartElement()) {
    if (xml->attributes().value("type").compare(QString("filter2d")) == 0) {
      // TODO: Probably should use getComponentTyped instead of
      // listComponentsTyped
      for (const auto& metaFilter :
           vx::voxieRoot()
               .components()
               ->listComponentsTyped<vx::plugin::MetaFilter2D>()) {
        if (metaFilter->objectName().compare(xml->name()) == 0) {
          Filter2D* filter = metaFilter->createFilter();
          this->addFilter(filter);

          filter->filterID = xml->attributes().value("filterID").toULongLong();
          if (!filter->filterID)
            throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                "Filter ID is not set");
          maxID = std::max(maxID, filter->filterID);

          QString enabledStr = xml->attributes().value("enabled").toString();
          if (enabledStr == "") {
            filter->setEnabled(true);
          } else {
            if (enabledStr != "true" && enabledStr != "false")
              throw vx::Exception(
                  "de.uni_stuttgart.Voxie.Error",
                  "Invalid value for 'enabled': '" + enabledStr + "'");
            filter->setEnabled(enabledStr == "true");
          }

          filter->importFilterSettingsXML(xml->attributes());
          QByteArray mask = xml->attributes().value("selectionJson").toUtf8();
          if (mask != "") {
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(mask, &error);
            if (doc.isNull())
              throw vx::Exception(
                  "de.uni_stuttgart.Voxie.Error",
                  "Error parsing selection mask JSON: " + error.errorString() +
                      "\nJSON string:\n" + mask);
            filter->getMask()->addMaskFromJson(doc.object());
          }
        }
      }
    }
    xml->skipCurrentElement();
  }
  lastID = maxID;

  Q_EMIT filterListChanged();
}

void FilterChain2D::fromXML(QString fileName) {
  QFile file(fileName);
  file.open(QIODevice::ReadOnly);
  QXmlStreamReader xml;
  xml.setDevice(&file);

  try {
    fromXML(&xml);
  } catch (vx::Exception& e) {
    QMessageBox messageBox;
    messageBox.critical(0, "Error", e.message());
    messageBox.setFixedSize(500, 200);
    return;
  }
}

void FilterChain2D::fromXMLString(const QString& data) {
  QXmlStreamReader xml(data);

  fromXML(&xml);
}
