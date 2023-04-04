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
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusConnection>

#include "RawData2DVisualizer.hpp"

#include <PluginVisRaw/ImageSelectionWidget.hpp>
#include <PluginVisRaw/InfoWidget.hpp>

#include <PluginVisRaw/Prototypes.hpp>

#include <VoxieClient/JsonDBus.hpp>

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/ImageInterpolate.hpp>
#include <Voxie/Data/InitializeColorizeWorker.hpp>

#include <VoxieBackend/Data/ImageDataPixel.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>
#include <VoxieBackend/Data/TomographyRawData2DRegular.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>

#include <Voxie/IVoxie.hpp>

#include <Voxie/Vis/FilterChain2DWidget.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QMetaType>
#include <QtCore/QThreadPool>

#include <QtGui/QColor>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QRgb>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QSizePolicy>

using namespace vx;
using namespace vx::visualization;

// TODO: Copied from TomographyRawDataNodeView.cpp
class JsonInfoDialog : public QDialog {
 public:
  JsonInfoDialog(const QString& data) {
    this->resize(500 / 96.0 * this->logicalDpiX(),
                 450 / 96.0 * this->logicalDpiY());
    QVBoxLayout* layout = new QVBoxLayout();
    this->setLayout(layout);

    auto edit = new QPlainTextEdit(data, this);
    layout->addWidget(edit);
    edit->setReadOnly(true);
  }
  ~JsonInfoDialog() {
    // qDebug() << "~JsonInfoDialog()";
  }
};

RawVisualizer::RawVisualizer()
    : SimpleVisualizer(getPrototypeSingleton()),
      properties(new vx::visualizer_prop::TomographyRawDataProperties(this)),
      histogramProvider(new HistogramProvider) {
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &vx::visualizer_prop::TomographyRawDataProperties::rawData,
      &vx::visualizer_prop::TomographyRawDataProperties::rawDataChanged,
      &DataNode::dataChangedFinished, this,
      &RawVisualizer::rawDataChangedFinished);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &vx::visualizer_prop::TomographyRawDataProperties::rawData,
      &vx::visualizer_prop::TomographyRawDataProperties::rawDataChanged,
      &Node::displayNameChanged, this,
      &RawVisualizer::rawDataDisplayNameChanged);

  // Redraw when shown part of image changes
  connect(this->properties,
          &vx::visualizer_prop::TomographyRawDataProperties::centerPointChanged,
          this, &SimpleVisualizer::triggerRedraw);
  connect(
      this->properties,
      &vx::visualizer_prop::TomographyRawDataProperties::verticalSizeChanged,
      this, &SimpleVisualizer::triggerRedraw);

  // Redraw when data changes
  connect(this, &RawVisualizer::rawDataChangedFinished, this,
          &SimpleVisualizer::triggerRedraw);

  // Redraw when color mapping changes
  QObject::connect(this->properties,
                   &vx::visualizer_prop::TomographyRawDataProperties::
                       valueColorMappingChanged,
                   this, &SimpleVisualizer::triggerRedraw);

  // Redraw when interpolation changes
  QObject::connect(
      this->properties,
      &vx::visualizer_prop::TomographyRawDataProperties::interpolationChanged,
      this, &SimpleVisualizer::triggerRedraw);

  // Change of currentImage does *not* trigger a redraw, it only will trigger
  // loading the new image (and when that is finished it will trigger a redraw)

  qRegisterMetaType<vx::FloatImage>();
  qRegisterMetaType<vx::PlaneInfo>();
  qRegisterMetaType<QVector<int>>();
  qRegisterMetaType<QVector<float>>();

  this->view()->setMinimumSize(300, 200);

  info = new InfoWidget(this, nullptr);
  QObject::connect(this, &QObject::destroyed, info, &QObject::deleteLater);

  imageSelectionWidget = new ImageSelectionWidget(nullptr, this);
  QObject::connect(this, &QObject::destroyed, imageSelectionWidget,
                   &QObject::deleteLater);

  showPerImageMetadata = new QPushButton("Show per-image metadata");
  QObject::connect(this, &QObject::destroyed, showPerImageMetadata,
                   &QObject::deleteLater);
  QObject::connect(showPerImageMetadata, &QPushButton::clicked, this, [this]() {
    // qDebug() << "showPerImageMetadata";

    auto data = this->data();
    QString stream;
    quint64 id;
    QJsonObject listMetadata;
    std::tie(stream, id, listMetadata) = getCurrentStreamAndImage();
    if (!data) return;
    QString result;
    try {
      QJsonObject metadata = {
          {"PerImageMetadata", data->getPerImageMetadata(stream, id)},
          {"ListMetadata", listMetadata},
      };
      result = QJsonDocument(metadata).toJson();
    } catch (vx::Exception& e) {
      result = e.message();
    }
    auto dialog = new JsonInfoDialog(result);
    dialog->show();
    QObject::connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
  });

  showImageKind = new QPushButton("Show image kind");
  QObject::connect(this, &QObject::destroyed, showImageKind,
                   &QObject::deleteLater);
  QObject::connect(showImageKind, &QPushButton::clicked, this, [this]() {
    auto obj = this->properties->imageKind();
    auto result = QJsonDocument(obj).toJson();
    auto dialog = new JsonInfoDialog(result);
    dialog->show();
    QObject::connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
  });

  QObject::connect(this->view(), &VisualizerView::forwardWheelEvent, this,
                   [this](QWheelEvent* e) {
                     qreal zoom = (e->delta() < 0) ? 1.1f : (1 / 1.1f);
                     this->zoomPlaneArea(zoom);
                   });
  QObject::connect(this->view(), &VisualizerView::forwardMousePressEvent, this,
                   [this](QMouseEvent* e) {
                     dragStart = e->pos();
                     dragStartValid = true;
                   });
  QObject::connect(this->view(), &VisualizerView::forwardMouseMoveEvent, this,
                   [this](QMouseEvent* e) {
                     if (dragStartValid) {
                       int deltaX = e->pos().x() - dragStart.x();
                       int deltaY = e->pos().y() - dragStart.y();
                       this->moveArea(-deltaX, +deltaY);
                       dragStart = e->pos();
                     }
                   });
  QObject::connect(this->view(), &VisualizerView::forwardMouseReleaseEvent,
                   this, [this](QMouseEvent* e) {
                     Q_UNUSED(e);
                     dragStartValid = false;
                   });

  this->initializeWorker = new InitializeColorizeWorker();
  this->initializeWorker->setAutoDelete(false);
  // TODO
  // connect(initializeWorker, &InitializeColorizeWorker::init, this,
  //        &RawVisualizer::initializeFinished);
  // TODO
  /*
  connect(this->_colorizerWidget, &RawImageColorizerWidget::startInitProcess,
          [this]() {
            if (_rawImage) {
              this->initializeWorker->changeInitSet(_rawImage);
              QThreadPool::globalInstance()->start(initializeWorker);
            }
          });
  */

  //**** HISTOGRAMWIDGET ***
  _histogramWidget = new HistogramWidget();
  _histogramWidget->setHistogramProvider(histogramProvider);
  QObject::connect(this, &QObject::destroyed, _histogramWidget,
                   &QObject::deleteLater);
  auto name = this->_histogramWidget->windowTitle();
  name.append(" - ");
  // name.append(this->rawObject()->displayName());//TODO
  this->_histogramWidget->setWindowTitle(name);

  //*****   ****

  connect(properties,
          &vx::visualizer_prop::TomographyRawDataProperties::
              valueColorMappingChanged,
          this, [&](QList<vx::ColorizerEntry> entries) {
            QSharedPointer<Colorizer> colorizer =
                QSharedPointer<Colorizer>(new Colorizer());
            colorizer->setEntries(entries);
            _histogramWidget->setColorizer(colorizer);
          });

  connect(this, &RawVisualizer::rawDataDisplayNameChanged, this, [this]() {
    auto rawObject = properties->rawData();

    QString name2;
    if (rawObject)
      name2 = "RawVisualizer - " + rawObject->displayName();
    else
      name2 = "RawVisualizer - Not connected";
    this->setAutomaticDisplayName(name2);
  });

  cache = RawImageCache::create();
  connect(
      properties,
      &vx::visualizer_prop::TomographyRawDataProperties::currentImageChanged,
      this, [this](quint64 listPos) {
        if (this->setupFinishedCalled) {
          QString stream;
          quint64 id;
          QJsonObject metadata;
          std::tie(stream, id, metadata) = this->getStreamAndImage(
              this->properties->currentImageList(), listPos);
          this->cache->setMainImage(stream, id, metadata,
                                    this->properties->imageKind());
        }
      });
  connect(properties,
          &vx::visualizer_prop::TomographyRawDataProperties::
              currentImageListChanged,
          this, [this](const std::tuple<QString, QJsonObject>& list) {
            if (this->setupFinishedCalled) {
              QString stream;
              quint64 id;
              QJsonObject metadata;
              std::tie(stream, id, metadata) = this->getStreamAndImage(
                  list, this->properties->currentImage());
              this->cache->setMainImage(stream, id, metadata,
                                        this->properties->imageKind());
            }
          });
  connect(properties,
          &vx::visualizer_prop::TomographyRawDataProperties::imageKindChanged,
          this, [this](const QJsonObject& imageKind) {
            if (this->setupFinishedCalled) {
              QString stream;
              quint64 id;
              QJsonObject metadata;
              std::tie(stream, id, metadata) = this->getCurrentStreamAndImage();
              this->cache->setMainImage(stream, id, metadata, imageKind);
            }
          });
  connect(cache.data(), &RawImageCache::imageLoaded, this,
          &RawVisualizer::updateCachedImage);
  connect(cache.data(), &RawImageCache::imageChanged, this,
          &RawVisualizer::updateCachedImage);
  // cache->setData(this->data());

  connect(this, &RawVisualizer::rawDataChangedFinished, this, [this]() {
    auto data = this->data();

    if (this->setupFinishedCalled) {
      cache->setData(data);
      // TODO: Do this here?
      QString stream;
      quint64 id;
      QJsonObject metadata;
      std::tie(stream, id, metadata) = this->getCurrentStreamAndImage();
      cache->setMainImage(stream, id, metadata, this->properties->imageKind());
    }
  });

  connect(this, &RawVisualizer::rawDataChangedFinished, this,
          &RawVisualizer::selectInitialImageKindAndList);
  selectInitialImageKindAndList();

  connect(this, &RawVisualizer::rawDataChangedFinished, this, [this]() {
    auto data = this->data();
    if (!data) return;

    // Don't do anything if VerticalSize is already initialized
    if (this->properties->verticalSize()) return;

    if (!data->hasStream("") || !data->numberOfImages("")) return;
    auto bbox = getProjectionArea(data, "", 0, QJsonObject{});

    this->properties->setCenterPoint(bbox.center());
    this->properties->setVerticalSize(bbox.height());
  });

  //**** HISTOGRAMWIDGET ***
  connect(this, &RawVisualizer::signalRequestHistogram, _histogramWidget,
          [&](const QSharedPointer<vx::ImageDataPixel>& image) {
            if (!image) {
              histogramProvider->setData(
                  createQSharedPointer<HistogramProvider::Data>());
              return;
            }
            auto imageCast =
                qSharedPointerDynamicCast<ImageDataPixelInst<float, 1>>(image);
            if (!imageCast)
              throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                                  "Unsupported image type for raw image");
            histogramProvider->setDataFromFloatImage(
                imageCast->image(), HistogramProvider::DefaultBucketCount);
          });
}

void RawVisualizer::updateCachedImage() {
  QSharedPointer<vx::DataVersion> dataVersion;
  QString stream;
  qint64 imageId;
  QJsonObject metadata;
  QJsonObject imageKind;
  QSharedPointer<vx::ImageDataPixel> rawImage;
  std::tie(dataVersion, stream, imageId, metadata, imageKind, rawImage) =
      cache->getMainImage();

  QJsonValue versionMetadata = QJsonObject();
  // TODO: This should not need to check fakeTomographyRawData2DRegular()
  if (rawImage)
    versionMetadata = rawImage->fakeTomographyRawData2DRegular()
                          ->currentVersion()
                          ->metadata();
  auto data = this->data();
  this->info->setCurrentStreamImageId(data && rawImage, data, stream, imageId,
                                      metadata, versionMetadata);
  this->triggerRedraw();
  Q_EMIT this->signalRequestHistogram(rawImage);
}

QRectF RawVisualizer::getProjectionArea(
    const QSharedPointer<TomographyRawData2DAccessor>& data,
    const QString& stream, qint64 imageId,
    const QJsonObject& projectionGeometry) {
  // TODO: Get values from Geometry?

  vx::VectorSizeT2 size;
  try {
    size = data->imageSize(stream, imageId);
  } catch (vx::Exception& e) {
    qWarning() << "Error while getting image size" << e.message();
    return QRectF();
  }

  // return QRectF(QPointF(0, 0), QSize(size.x, size.y));

  // TODO: Get this from somewhere else, use per-image values?
  // TODO: Default values?
  double detectorPixelSizeX = 1e-3;
  double detectorPixelSizeY = 1e-3;
  auto metadata = data->metadata();
  if (metadata.contains("Info")) {
    auto info = metadata["Info"].toObject();
    if (info.contains("DetectorPixelSize")) {
      auto detectorPixelSize = info["DetectorPixelSize"].toArray();
      if (detectorPixelSize.size() == 2) {
        detectorPixelSizeX = detectorPixelSize[0].toDouble();
        detectorPixelSizeY = detectorPixelSize[1].toDouble();
      }
    }
  }

  QSizeF sizeF(size.x * detectorPixelSizeX, size.y * detectorPixelSizeY);
  QPointF origin(-sizeF.width() / 2.0, -sizeF.height() / 2.0);

  // TODO: This should be done in a better way (handling DetectorRotation etc.),
  // probably the visualizer plane should be the detector plane moved parallel
  // to the detector plane such that a line though the visualizer plane
  // orthogonal to the detector plane goes through the global origin.
  // Or, as an alternative, through the table or object origin (to account for
  // movements of the table / object).
  if (projectionGeometry.contains("DetectorPosition")) {
    auto detectorPosition = projectionGeometry["DetectorPosition"].toArray();
    if (detectorPosition.size() == 2)
      origin += QPointF(detectorPosition[0].toDouble(),
                        detectorPosition[1].toDouble());
  }

  return QRectF(origin, sizeF);
}

QSharedPointer<vx::TomographyRawData2DAccessor> RawVisualizer::data() {
  vx::checkOnMainThread("RawVisualizer::data()");

  auto rawObject = dynamic_cast<TomographyRawDataNode*>(properties->rawData());
  if (!rawObject)
    return QSharedPointer<vx::TomographyRawData2DAccessor>();
  else
    return rawObject->dataAccessor();
}

QWidget* RawVisualizer::getCustomPropertySectionContent(const QString& name) {
  if (name == "de.uni_stuttgart.Voxie.RawVisualizer.Histogram") {
    return _histogramWidget;
  } else if (name ==
             "de.uni_stuttgart.Voxie.RawVisualizer.ImageSelectionButtons") {
    return imageSelectionWidget;
  } else if (name ==
             "de.uni_stuttgart.Voxie.RawVisualizer.ShowPerImageMetadata") {
    return showPerImageMetadata;
  } else if (name == "de.uni_stuttgart.Voxie.RawVisualizer.ShowImageKind") {
    return showImageKind;
  } else if (name ==
             "de.uni_stuttgart.Voxie.Visualizer.TomographyRawData.Info") {
    return info;
  } else {
    return Node::getCustomPropertySectionContent(name);
  }
}

QSharedPointer<QObject> RawVisualizer::getPropertyUIData(QString propertyName) {
  if (propertyName ==
      "de.uni_stuttgart.Voxie.Visualizer.TomographyRawData.ValueColorMapping") {
    return histogramProvider;
  } else {
    return Node::getPropertyUIData(propertyName);
  }
}

QRectF RawVisualizer::getCurrentPlaneArea(
    vx::visualizer_prop::TomographyRawDataPropertiesBase* properties,
    const QSize& canvasSize) {
  int widthC = canvasSize.width();
  int heightC = canvasSize.height();
  float aspectRatio = (float)widthC / heightC;
  float heightF = properties->verticalSize();
  float widthF = heightF * aspectRatio;
  auto center = properties->centerPoint();
  return QRectF(center.x() - widthF / 2, center.y() - heightF / 2, widthF,
                heightF);
}

void RawVisualizer::zoomPlaneArea(qreal multiplier) {
  // this->properties->setVerticalSize(this->properties->verticalSize() /
  //                                  multiplier);
  this->properties->setVerticalSize(this->properties->verticalSize() *
                                    multiplier);
}

void RawVisualizer::moveArea(  // vx::TomographyRawDataPropertiesBase*
                               // properties,const QSize& canvasSize,
    qreal pixelDeltaX, qreal pixelDeltaY) {
  auto properties = this->properties;
  auto canvasSize = this->view()->size();

  qreal relx = pixelDeltaX / canvasSize.width();
  qreal rely = pixelDeltaY / canvasSize.height();
  auto area = getCurrentPlaneArea(properties, canvasSize);
  relx *= area.width();
  rely *= area.height();
  // this->properties->setCenterPoint(this->properties->centerPoint() -
  //                                 QPointF(relx, rely));
  this->properties->setCenterPoint(this->properties->centerPoint() +
                                   QPointF(relx, rely));
}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
RawVisualizer::getRenderFunction() {
  return [cache = this->cache](
             const QSharedPointer<vx::ImageDataPixel>& outputImage,
             const vx::VectorSizeT2& outputRegionStart,
             const vx::VectorSizeT2& size,
             const QSharedPointer<vx::ParameterCopy>& parameters,
             const QSharedPointer<vx::VisualizerRenderOptions>& options) {
    vx::visualizer_prop::TomographyRawDataPropertiesCopy properties(
        parameters->properties()[parameters->mainNodePath()]);

    /* Currently no properties in TomographyRawDataNode
    TomographyRawDataNodePropertiesCopy rawProperties(
        parameters->properties()[properties.rawDataRaw()]);
    */

    auto data = qSharedPointerDynamicCast<TomographyRawData2DAccessor>(
        parameters->getData(properties.rawDataRaw()).data());
    if (!data)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error", "data == nullptr");
    auto dataVersion = parameters->getData(properties.rawDataRaw()).version();

    auto interpolation = getInterpolationFromString(properties.interpolation());

    quint64 width = size.x;
    quint64 height = size.y;

    QString stream;
    qint64 imageId;
    QJsonObject metadata;
    QJsonObject imageKind;
    QSharedPointer<vx::ImageDataPixel> rawImage;
    if (options->isMainView()) {
      // Might return a previous image if the current one is not yet loaded
      std::tie(dataVersion, stream, imageId, metadata, imageKind, rawImage) =
          cache->getMainImage();
    } else {
      std::tie(stream, imageId, metadata) = getStreamAndImage(
          data, properties.currentImageList(), properties.currentImage());
      imageKind = properties.imageKind();
      rawImage = cache->getImage(data, dataVersion, stream, imageId, imageKind);
    }

    auto projArea = getProjectionArea(
        data, stream, imageId, metadata["ProjectionGeometry"].toObject());

    auto area = getCurrentPlaneArea(&properties, QSize(width, height));

    if (rawImage) {
      auto image = imageInterpolate(rawImage, projArea, area,
                                    QSize(width, height), interpolation);

      // TODO: Use ImageDataPixel for colorization
      auto imageCast =
          qSharedPointerDynamicCast<ImageDataPixelInst<float, 1>>(image);
      if (!imageCast)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Unsupported image type for raw image");
      auto imageFloatImage = imageCast->image();

      Colorizer colorizer;
      colorizer.setEntries(properties.valueColorMapping());
      auto targetImage = colorizer.toQImage(imageFloatImage);

      outputImage->fromQImage(targetImage, outputRegionStart);
    } else {
      QImage qimage(width, height, QImage::Format_ARGB32);
      qimage.fill(qRgba(0, 0, 0, 0));  // Fill will transparent

      outputImage->fromQImage(qimage, outputRegionStart);
    }
  };
}

void RawVisualizer::setupFinished() {
  this->setupFinishedCalled = true;
  cache->setData(this->data());
  QString stream;
  quint64 id;
  QJsonObject metadata;
  std::tie(stream, id, metadata) = getCurrentStreamAndImage();
  this->cache->setMainImage(stream, id, metadata,
                            this->properties->imageKind());
}

bool RawVisualizer::isUpToDate() {
  // TODO: This will currently return 'true' even if the image has not been
  // rendered yet (and because render calls are compressed, it might never be
  // rendered)

  vx::checkOnMainThread("RawVisualizer::isUpToDate()");

  // Get information what is being displayed currently
  QSharedPointer<vx::DataVersion> dataVersion;
  QString stream;
  qint64 imageId;
  QJsonValue metadata1;
  QJsonObject imageKind;
  QSharedPointer<vx::ImageDataPixel> rawImage;
  std::tie(dataVersion, stream, imageId, metadata1, imageKind, rawImage) =
      cache->getMainImage();

  // Get information what should be displayed according to the settings
  QString targetStream;
  quint64 targetId;
  QJsonValue metadata2;
  std::tie(targetStream, targetId, metadata2) = getCurrentStreamAndImage();

  // Ignore metadata, the same image from different image lists (which might
  // differ in metadata) are considered the same

  return rawImage && stream == targetStream && imageId == (qint64)targetId &&
         imageKind == this->properties->imageKind();
}

std::tuple<QString, quint64, QJsonObject> RawVisualizer::getStreamAndImage(
    const QSharedPointer<vx::TomographyRawData2DAccessor>& data,
    const std::tuple<QString, QJsonObject>& list, quint64 listPos) {
  auto errorObj = std::make_tuple("", (quint64)-1, QJsonObject{});

  if (!data) {
    qWarning() << "RawVisualizer::getStreamAndImage called with nullptr data";
    return errorObj;
  }

  if (std::get<0>(list) == "" && std::get<1>(list) == QJsonObject{}) {
    qWarning() << "RawVisualizer::getStreamAndImage: trying to get image for "
                  "None stream";
    return errorObj;
  } else if (std::get<0>(list) ==
             "de.uni_stuttgart.Voxie.TomographyRawDataImageListType."
             "ImageStream") {
    QString name = std::get<1>(list)["StreamName"].toString();
    QJsonObject metadata{};  // No metadata available
    return std::make_tuple(name, listPos, metadata);
  } else if (std::get<0>(list) ==
             "de.uni_stuttgart.Voxie.TomographyRawDataImageListType."
             "GeometryImageList") {
    QString geometryType = std::get<1>(list)["GeometryType"].toString();
    QJsonArray path = std::get<1>(list)["Path"].toArray();
    auto list2 = data->getImageList(geometryType, path);
    if (!list2) {
      qWarning() << "RawVisualizer::getStreamAndImage: image list is nullptr";
      return errorObj;
    }
    if (listPos >= (quint64)list2->count()) {
      qWarning() << "RawVisualizer::getStreamAndImage: image list index is out "
                    "of range";
      return errorObj;
    }
    auto image = (*list2)[listPos];
    if (!image) {
      qWarning() << "RawVisualizer::getStreamAndImage: got nullptr image";
      return errorObj;
    }
    QJsonObject metadata{
        //{"GeometryImageListPath", path},
        {"ProjectionGeometry", image->geometry()},
    };
    return std::make_tuple(image->stream(), image->id(), metadata);
  } else {
    qWarning() << "Unknown TomographyRawDataImageListType:"
               << std::get<0>(list);
    return errorObj;
  }
}
std::tuple<QString, quint64, QJsonObject> RawVisualizer::getStreamAndImage(
    const std::tuple<QString, QJsonObject>& imageList, quint64 listPos) {
  return getStreamAndImage(this->data(), imageList, listPos);
}
std::tuple<QString, quint64, QJsonObject>
RawVisualizer::getCurrentStreamAndImage() {
  return getStreamAndImage(this->properties->currentImageList(),
                           this->properties->currentImage());
}

quint64 RawVisualizer::getCurrentImageListCount() {
  auto list = this->properties->currentImageList();

  if (std::get<0>(list) == "" && std::get<1>(list) == QJsonObject{}) {
    return 0;
  } else if (std::get<0>(list) ==
             "de.uni_stuttgart.Voxie.TomographyRawDataImageListType."
             "ImageStream") {
    QString name = std::get<1>(list)["StreamName"].toString();
    auto data = this->data();
    return data ? data->numberOfImages(name) : 0;
  } else if (std::get<0>(list) ==
             "de.uni_stuttgart.Voxie.TomographyRawDataImageListType."
             "GeometryImageList") {
    QString geometryType = std::get<1>(list)["GeometryType"].toString();
    QJsonArray path = std::get<1>(list)["Path"].toArray();
    auto data = this->data();
    if (!data) return 0;
    auto list2 = data->getImageList(geometryType, path);
    if (!list2) {
      qWarning()
          << "RawVisualizer::getCurrentImageListCount: image list is nullptr";
      return 0;
    }
    return list2->count();
  } else {
    qWarning() << "Unknown TomographyRawDataImageListType:"
               << std::get<0>(list);
    return 0;
  }
}

void RawVisualizer::selectInitialImageKindAndList() {
  auto data = this->data();
  if (!data) return;

  // If the currently selected image kind is not valid, choose the last one
  {
    auto availableImageKinds = data->availableImageKinds();

    auto current = this->properties->imageKind();
    bool imageKindFound = false;
    for (const auto& kind : availableImageKinds) {
      if (current == kind) {
        imageKindFound = true;
        break;
      }
    }

    if (!imageKindFound && availableImageKinds.size() > 0)
      this->properties->setImageKind(
          availableImageKinds[availableImageKinds.size() - 1]);
  }

  // If the currently selected image list is not valid, choose the first one
  {
    auto availableImageLists = data->availableImageLists();

    auto current = this->properties->currentImageList();
    bool imageListFound = false;
    for (const auto& list : availableImageLists) {
      if (current == list) {
        imageListFound = true;
        break;
      }
    }

    if (!imageListFound && availableImageLists.size() > 0)
      this->properties->setCurrentImageList(availableImageLists[0]);
  }
}

NODE_PROTOTYPE_IMPL_SEP(visualizer_prop::TomographyRawData, RawVisualizer)
