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

#include "SliceVisualizer.hpp"

#include <PluginVisSlice/ImagePaintWidget.hpp>

#include <PluginVisSlice/DefaultTool.hpp>
#include <PluginVisSlice/GeometricPrimitiveLayer.hpp>
#include <PluginVisSlice/Ruler.hpp>
#include <PluginVisSlice/SliceCenterLayer.hpp>
#include <PluginVisSlice/SurfaceVisualizerTool.hpp>
#include <PluginVisSlice/TextLayer.hpp>
#include <PluginVisSlice/ToolSelection.hpp>
#include <PluginVisSlice/View3DPropertiesConnection.hpp>
#include <PluginVisSlice/ViewCenterLayer.hpp>
#include <PluginVisSlice/VolumeGrid.hpp>

#include <PluginVisSlice/InfoWidget.hpp>
#include <PluginVisSlice/PointProperties.hpp>

#include <PluginVisSlice/Prototypes.hpp>

#include <Voxie/Data/Colorizer.hpp>
#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/VolumeNode.hpp>

#include <Voxie/Vis/View3D.hpp>

#include <VoxieBackend/Data/HistogramProvider.hpp>
#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <Voxie/Vis/FilterChain2DWidget.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QMetaType>
#include <QtCore/QThreadPool>

#include <QtGui/QColor>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QRgb>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>

VX_NODE_INSTANTIATION(SliceVisualizer)

using namespace vx::visualization;
using namespace vx;

SliceVisualizer::SliceVisualizer()
    : VisualizerNode(getPrototypeSingleton()),
      properties(new vx::SliceProperties(this)) {
  qRegisterMetaType<vx::FloatImage>();
  qRegisterMetaType<vx::SliceImage>();
  qRegisterMetaType<vx::PlaneInfo>();
  qRegisterMetaType<QVector<int>>();
  qRegisterMetaType<QVector<float>>();
  qRegisterMetaType<vx::HistogramProvider::DataPtr>();

  View3DValues initial;
  initial.setFieldOfView(0);       // orthogonal
  initial.setViewSizeUnzoomed(1);  // is kept constant
  view3d_ = makeSharedQObject<View3D>(
      this,
      vx::visualization::View3DProperty::LookAt |
          vx::visualization::View3DProperty::Orientation |
          vx::visualization::View3DProperty::ZoomLog,
      initial);
  // Slow down panning with mouse wheel a lot (default is currently 3)
  view3d()->wheelPanFactor = 0.3;
  // Same for key panning
  view3d()->keyPanFactor = 0.0125;
  // Keep Z values when resetting view
  view3d()->resetKeepViewZ = true;

  new vx::vis_slice::View3DPropertiesConnection(this, this->properties,
                                                view3d().data());

  connect(properties, &SliceProperties::centerPointChanged, this,
          &SliceVisualizer::signalRequestSliceImageUpdate);
  connect(properties, &SliceProperties::verticalSizeChanged, this,
          &SliceVisualizer::signalRequestSliceImageUpdate);

  forwardSignalFromPropertyOnReconnect(this, properties->volumePropertyTyped(),
                                       &VolumeNode::boundingBoxGlobalChanged,
                                       this,
                                       &SliceVisualizer::updateBoundingBox);

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::geometricPrimitive,
      &SliceProperties::geometricPrimitiveChanged,
      &GeometricPrimitiveNode::measurementPrimitivesChanged, this,
      &SliceVisualizer::newMeasurement);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::geometricPrimitive,
      &SliceProperties::geometricPrimitiveChanged,
      &GeometricPrimitiveNode::selectedPrimitiveChanged, this,
      &SliceVisualizer::currentPointChanged);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::geometricPrimitive,
      &SliceProperties::geometricPrimitiveChanged,
      &DataNode::dataChangedFinished, this,
      &SliceVisualizer::gpoDataChangedFinished);

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::plane, &SliceProperties::planeChanged,
      &vx::PlaneNode::rotationChanged, this,
      &SliceVisualizer::rotationChangedForward);
  QObject::connect(
      this, &SliceVisualizer::rotationChangedForward, this, [this]() {
        // qDebug() << "&SliceVisualizer::rotationChangedForward";
        this->emitCustomPropertyChanged(
            this->properties->orientationProperty());
        // Keep standalone value updated to keep value when plane is
        // disconnected
        this->standaloneOrientation =
            PropertyValueConvertRaw<vx::TupleVector<double, 4>,
                                    QQuaternion>::toRaw(this->properties
                                                            ->orientation());
      });

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::plane, &SliceProperties::planeChanged,
      &vx::PlaneNode::originChanged, this,
      &SliceVisualizer::originChangedForward);
  QObject::connect(
      this, &SliceVisualizer::originChangedForward, this, [this]() {
        // qDebug() << "&SliceVisualizer::originChangedForward";
        this->emitCustomPropertyChanged(this->properties->originProperty());
        // Keep standalone value updated to keep value when plane is
        // disconnected
        this->standaloneOrigin =
            PropertyValueConvertRaw<vx::TupleVector<double, 3>,
                                    QVector3D>::toRaw(this->properties
                                                          ->origin());
      });

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::volume, &SliceProperties::volumeChanged,
      &DataNode::dataChangedFinished, this,
      &SliceVisualizer::volumeDataChangedFinished);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::volume, &SliceProperties::volumeChanged,
      &VolumeNode::rotationChanged, this,
      &SliceVisualizer::volumeDataRotationChanged);
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::volume, &SliceProperties::volumeChanged,
      &VolumeNode::translationChanged, this,
      &SliceVisualizer::volumeDataTranslationChanged);

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::labelContainer,
      &SliceProperties::labelContainerChanged, &DataNode::dataChangedFinished,
      this, &SliceVisualizer::labelContainerChangedFinished);

  forwardSignalFromPropertyNodeOnReconnect(
      properties, &SliceProperties::volume, &SliceProperties::volumeChanged,
      &Node::displayNameChanged, this,
      &SliceVisualizer::volumeDisplayNameChanged);

  // TODO: Remove?
  QObject::connect(this, &SliceVisualizer::volumeDataChangedFinished, this,
                   &SliceVisualizer::signalRequestSliceImageUpdate);

  this->view = new QWidget();
  this->setAutomaticDisplayName("Slice Visualizer");

  connect(this, &SliceVisualizer::volumeDisplayNameChanged, this, [this]() {
    // Set name of slice visualizer
    auto node = this->properties->volume();
    if (node) {
      this->setAutomaticDisplayName(node->displayName());
    } else {
      this->setAutomaticDisplayName("Slice Visualizer");
    }
  });

  connect(
      properties, &SliceProperties::volumeChanged, this,
      [this](vx::Node* newVolume) {
        (void)newVolume;

        // Update bounding box on volume change
        // updateBoundingBox() is needed otherwise the zoom will be reset to the
        // wrong value
        this->updateBoundingBox();
        view3d()->resetView(View3DProperty::LookAt | View3DProperty::ZoomLog);

        // TODO: Clean up redraw code here
        this->redraw();
        Q_EMIT this->signalRequestSliceImageUpdate();

        for (const auto& layer : this->layers()) {
          layer->triggerRedraw();  // Make sure all layers are drawn at least
                                   // once
        }

        // TODO: This was broken by the removal of the vx::Slice class, fix
        // this? (Without this, the filter won't be notified if the plane gets
        // changed.)
        /*
        connect(this->_slice, &Slice::planeChanged,
                this->_filterChain2DWidget->getFilterChain(),
                &vx::filter::FilterChain2D::onPlaneChanged,
        Qt::DirectConnection);
        */
      });

  this->view->setMinimumSize(300 / 96.0 * this->view->logicalDpiX(),
                             200 / 96.0 * this->view->logicalDpiY());

  this->_imageDisplayingWidget =
      new ImagePaintWidget(this);  // has dependencies in tools
  this->view->setFocusProxy(
      this->_imageDisplayingWidget);  // <- receives all keyboard events

  QSizePolicy pol(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
  pol.setHorizontalStretch(0);
  pol.setVerticalStretch(0);
  this->_imageDisplayingWidget->setSizePolicy(pol);

  imageLayer = ImageLayer::create(this);
  imageLayer->setObjectName(this->imageLayerName);
  this->layers_.append(imageLayer);

  this->layers_.append(LegendLayer::create(this));

  this->defaultTool = new DefaultTool(view, this);
  this->defaultTool->setObjectName("DefaultTool");
  this->_tools.append(this->defaultTool);
  this->layers_.append(SliceCenterLayer::create(this));
  this->layers_.append(ViewCenterLayer::create(this));

  auto sv = SurfaceVisualizerTool::create(this);
  sv->setObjectName("SurfaceVisualizerTool");
  this->layers_.append(sv);  // only elements in layers_ will be visible over
                             // slice

  auto gal = GeometricPrimitiveLayer::create(this);
  this->layers_.append(gal);

  this->selectionTool = new ToolSelection(view, this);
  this->selectionTool->setObjectName("ToolSelection");
  this->_tools.append(this->selectionTool);
  this->layers_.append(this->selectionTool->layer());

  // *** Automatically sets the elements of _tools into the tool view ***
  if (_tools.size() <= 0) {
    qWarning() << "No tool found in slicevisualizer";
  } else {
    toolBar = new QWidget(view);
    QHBoxLayout* l = new QHBoxLayout();
    toolBar->setLayout(l);
    l->setAlignment(Qt::AlignLeft);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);
    l->setMargin(0);

    Visualizer2DTool** data = _tools.data();
    for (int i = 0; i < _tools.size(); ++i) {
      this->toolBar->layout()->addWidget(data[i]);
    }
  }
  // ***Automatically sets of tools view END***

  //*********FilterChain2D***********
  _filterChain2DWidget = new FilterChain2DWidget(view);

  this->dynamicSections().append(this->_filterChain2DWidget);
  connect(this->_filterChain2DWidget,
          &FilterChain2DWidget::requestFilterMaskEditor, this,
          &SliceVisualizer::onFilterMaskRequest);
  connect(this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::filterListChanged, this, [this]() {
            this->emitCustomPropertyChanged(
                this->properties->filter2DConfigurationProperty());
          });
  connect(this->_filterChain2DWidget->getFilterChain(),
          &vx::filter::FilterChain2D::filterChanged, this, [this]() {
            this->emitCustomPropertyChanged(
                this->properties->filter2DConfigurationProperty());
          });

  //*** COLORIZER ****
  // TODO: This connection should probably be made in HistogramWidget
  connect(properties, &SliceProperties::valueColorMappingChanged, this,
          [=](const QList<vx::ColorizerEntry>& value) {
            // after any change on the standard mapping
            // send over the new color mappings to the histogram widget and
            // update it
            QSharedPointer<Colorizer> colorizer =
                makeSharedQObject<Colorizer>();
            colorizer->setEntries(value);
            _histogramWidget->setColorizer(colorizer);
          });

  //**** INFOWIDGET ***
  infoWidget = new InfoWidget(this);
  connect(this, &QObject::destroyed, infoWidget, &QObject::deleteLater);

  //**** HISTOGRAM PROVIDER ****
  histogramProvider = QSharedPointer<vx::HistogramProvider>::create();

  connect(this, &SliceVisualizer::volumeDataChangedFinished, [this]() {
    disconnect(histogramConnection);

    auto volume = dynamic_cast<VolumeNode*>(properties->volume());
    auto data = volume ? volume->data() : QSharedPointer<Data>();

    if (auto volumeDataVoxel =
            qSharedPointerDynamicCast<VolumeDataVoxel>(data)) {
      auto volumeHistogramProvider = volumeDataVoxel->getHistogramProvider(
          HistogramProvider::DefaultBucketCount);

      histogramConnection = connect(
          volumeHistogramProvider.data(), &HistogramProvider::dataChanged,
          histogramProvider.data(), &HistogramProvider::setData);

      histogramProvider->setData(volumeHistogramProvider->getData());
    }
  });

  //**** SLICE HISTOGRAM PROVIDER ****
  sliceHistogramProvider = QSharedPointer<vx::HistogramProvider>::create();
  // TODO: Should the histogram calculation happen in a background thread?
  // This probably would require making sure that there are not multiple updates
  // at the same time and the the last result is used etc.
  // TODO: Currently this histogram is updated even if it is not used. Avoid
  // this?
  connect(this, &SliceVisualizer::signalRequestHistogram, this,
          [this](vx::SliceImage& image) {
            sliceHistogramProvider->setDataFromFloatImage(
                image, HistogramProvider::DefaultBucketCount);
          });

  //**** HISTOGRAMWIDGET ***
  histogramBox = new QWidget();
  QVBoxLayout* histogramLayout = new QVBoxLayout(histogramBox);
  QHBoxLayout* radioButtonLayout = new QHBoxLayout();
  connect(this, &QObject::destroyed, histogramBox, &QObject::deleteLater);
  volumeRadioButton = new QRadioButton("Volume");
  volumeRadioButton->setChecked(true);
  connect(volumeRadioButton, &QRadioButton::toggled, [this](bool checked) {
    // as long as we only have two radiobuttons it is enough to check if one of
    // the two is being checked/unchecked
    if (checked)
      _histogramWidget->setHistogramProvider(histogramProvider);
    else
      _histogramWidget->setHistogramProvider(sliceHistogramProvider);
  });
  radioButtonLayout->addWidget(volumeRadioButton);
  sliceRadioButton = new QRadioButton("Slice");
  radioButtonLayout->addWidget(sliceRadioButton);
  _histogramWidget = new HistogramWidget();
  histogramLayout->addLayout(radioButtonLayout);
  histogramLayout->addWidget(_histogramWidget);

  _histogramWidget->setHistogramProvider(histogramProvider);

  //**** GEOMETRICANALYSIS ***
  PointProperties* _pointListWidget = new PointProperties(view, this);
  QString name = "Geometric Analysis";
  _pointListWidget->setWindowTitle(name);
  this->dynamicSections().append(_pointListWidget);
  connect(_pointListWidget, &PointProperties::newVisibility, gal.data(),
          &GeometricPrimitiveLayer::newVisibility);

  //**** Grid ***
  auto grid = Grid::create(this);
  this->layers_.append(grid);

  //**** RULERWIDGET ***
  auto ruler = Ruler::create(this);
  this->layers_.append(ruler);

  //**** VolumeGridWidget ***
  auto volumegrid = VolumeGrid::create(this);
  this->layers_.append(volumegrid);

  //***** Multivariate Data Widget ****
  this->multivariateDataWidget = new MultivariateDataWidget(this->view, this);

  connect(properties, &SliceProperties::volumeChanged, multivariateDataWidget,
          &MultivariateDataWidget::data_channelsChanged);

  connect(this, &QObject::destroyed, multivariateDataWidget,
          &QObject::deleteLater);

  // Bridge from MultivariateDataWidget to SliceVisualizer to ImageLayer
  connect(multivariateDataWidget,
          &MultivariateDataWidget::multivariateDataVisChanged, this,
          &SliceVisualizer::multivariateDataPropertiesChangedIn);
  //***** Multivariate Data Widget ****

  hobox = new QVBoxLayout(view);
  hobox->setSpacing(0);
  this->view->setLayout(hobox);

  hobox->addWidget(_imageDisplayingWidget);
  hobox->addWidget(toolBar);
  connect(this->_imageDisplayingWidget, &ImagePaintWidget::resized, this,
          &SliceVisualizer::onCanvasResized);

  _resizeTimer.setSingleShot(true);
  connect(&this->_resizeTimer, &QTimer::timeout, this,
          &SliceVisualizer::resized);
  connect(this, &SliceVisualizer::resized, this,
          &SliceVisualizer::signalRequestSliceImageUpdate);

  if (_tools.size() > 0) {
    this->switchToolTo(this->currentTool());
  }

  this->view->show();

  _imageDisplayingWidget->setFocus();
  _imageDisplayingWidget->show();
  toolBar->show();

  createPlaneNodeAction = new QAction("Create plane property node", this);
  QObject::connect(createPlaneNodeAction, &QAction::triggered, this,
                   &SliceVisualizer::createPlaneNode);
  connect(properties, &SliceProperties::planeChanged, this,
          &SliceVisualizer::updateCreatePlaneNode);
  updateCreatePlaneNode();
  addContextMenuAction(createPlaneNodeAction);

  addSegmentationFunctionality(10);

  // Note: This currently has to be the last layer (because it iterates over the
  // layers)
  // TODO: Make sure that there is only one redraw after a layers is finished
  // and not two (because the TextLayer also has to be updated)?
  this->layers_.append(TextLayer::create(this));

  for (const auto& layer : this->layers()) {
    connect(this, &SliceVisualizer::resized, layer.data(),
            [this, layer]() { layer->onResize(this->canvasSize()); });
    Layer* layerPtr = layer.data();
    connect(layerPtr, &Layer::resultImageChanged, this,
            [this, layerPtr](const QImage& image) {
              this->addToDrawStack(layerPtr, image);
              this->redraw();
            });
    connect(
        layerPtr, &Layer::getRenderingParameters, this,
        [this](QSharedPointer<vx::ParameterCopy>& parameters, QSize& size) {
          if (QThread::currentThread() != this->thread()) {
            qCritical() << "Attempting to call getRenderingParameters() "
                           "from wrong thread";
            return;
          }
          size = this->canvasSize();
          parameters = ParameterCopy::getParameters(this);
        },
        Qt::DirectConnection);
  }
}

QWidget* SliceVisualizer::getCustomPropertySectionContent(const QString& name) {
  if (name == "de.uni_stuttgart.Voxie.SliceVisualizer.Histogram") {
    return histogramBox;
  } else if (name == "de.uni_stuttgart.Voxie.SliceVisualizer.Info") {
    return infoWidget;
  } else if (name ==
             "de.uni_stuttgart.Voxie.SliceVisualizer.MultivariateDataWidget") {
    return multivariateDataWidget;
  } else {
    return Node::getCustomPropertySectionContent(name);
  }
}

std::tuple<vx::SlicePropertiesBase*, QSize>
SliceVisualizer::currentImageProperties() {
  vx::checkOnMainThread("SliceVisualizer::currentImageProperties");

  // TODO: Actually return information about the currently shown image instead
  // of the image which will be shown?
  return std::make_tuple(this->properties, this->canvasSize());
}

double SliceVisualizer::getCurrentPixelSize(SlicePropertiesBase* properties,
                                            const QSize& canvasSize) {
  int heightC = canvasSize.height();
  double heightF = properties->verticalSize();
  return heightF / (double)heightC;
}

QRectF SliceVisualizer::getCurrentPlaneArea(SlicePropertiesBase* properties,
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
QRectF SliceVisualizer::currentPlaneArea() {
  return getCurrentPlaneArea(this->properties, this->canvasSize());
}

vx::Vector<double, 2> SliceVisualizer::pixelPosToPlanePos(
    vx::SlicePropertiesBase* properties, const QSize& canvasSize,
    const vx::Vector<double, 2>& pixelPos) {
  auto planeArea = getCurrentPlaneArea(properties, canvasSize);

  double relX = pixelPos[0] / (canvasSize.width() * 1.0);
  double relY = pixelPos[1] / (canvasSize.height() * 1.0);

  return {relX * planeArea.width() + planeArea.left(),
          relY * planeArea.height() + planeArea.top()};
}
vx::Vector<double, 2> SliceVisualizer::pixelPosToPlanePosCurrentImage(
    const vx::Vector<double, 2>& pixelPos) {
  auto prop = this->currentImageProperties();
  return pixelPosToPlanePos(std::get<0>(prop), std::get<1>(prop), pixelPos);
}

vx::Vector<double, 3> SliceVisualizer::planePosTo3DPos(
    vx::SlicePropertiesBase* properties, const QSize& canvasSize,
    const vx::Vector<double, 2>& planePos) {
  // TODO: Dont pass canvasSize?
  (void)canvasSize;

  // TODO: Avoid using PlaneInfo / QVector3D?
  vx::PlaneInfo cuttingPlane(properties->origin(), properties->orientation());

  return vectorCast<double>(
      toVector(cuttingPlane.get3DPoint(planePos[0], planePos[1])));
}
vx::Vector<double, 3> SliceVisualizer::planePosTo3DPosCurrentImage(
    const vx::Vector<double, 2>& planePos) {
  auto prop = this->currentImageProperties();
  return planePosTo3DPos(std::get<0>(prop), std::get<1>(prop), planePos);
}

vx::Vector<double, 3> SliceVisualizer::pixelPosTo3DPos(
    vx::SlicePropertiesBase* properties, const QSize& canvasSize,
    const vx::Vector<double, 2>& pixelPos) {
  return planePosTo3DPos(properties, canvasSize,
                         pixelPosToPlanePos(properties, canvasSize, pixelPos));
}
vx::Vector<double, 3> SliceVisualizer::pixelPosTo3DPosCurrentImage(
    const vx::Vector<double, 2>& pixelPos) {
  auto prop = this->currentImageProperties();
  return pixelPosTo3DPos(std::get<0>(prop), std::get<1>(prop), pixelPos);
}

QPointF SliceVisualizer::planePointToPixel(const QSize& imageSize,
                                           const QRectF& planeArea,
                                           const QPointF& planePoint) {
  QPoint pixel;
  pixel.setY(-(planePoint.y() - planeArea.bottom()) * imageSize.height() /
             (planeArea.height()));
  pixel.setX((planePoint.x() - planeArea.left()) * imageSize.width() /
             (planeArea.width()));
  return pixel;
}

void SliceVisualizer::switchToolTo(Visualizer2DTool* tool) {
  Visualizer2DTool** data = _tools.data();
  int i;
  for (i = 0; i < _tools.size(); ++i) {
    if (data[i] == tool) {
      break;
    }
  }
  if (i == _tools.size()) {
    return;
  }
  if (this->_currentTool != i) data[_currentTool]->deactivateTool();
  this->_currentTool = i;
  data[_currentTool]->activateTool();
  this->_imageDisplayingWidget->setFocus();
}

QVariant SliceVisualizer::getNodePropertyCustom(QString key) {
  if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration") {
    return this->_filterChain2DWidget->getFilterChain()->toXMLString();
  } else if (key ==
             "de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation") {
    auto plane = dynamic_cast<PlaneNode*>(this->properties->plane());
    if (plane)
      return QVariant::fromValue<vx::TupleVector<double, 4>>(
          PropertyValueConvertRaw<vx::TupleVector<double, 4>,
                                  QQuaternion>::toRaw(plane->plane()
                                                          ->rotation));
    else
      return QVariant::fromValue<vx::TupleVector<double, 4>>(
          this->standaloneOrientation);
  } else if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin") {
    auto plane = dynamic_cast<PlaneNode*>(this->properties->plane());
    if (plane)
      return QVariant::fromValue(
          PropertyValueConvertRaw<vx::TupleVector<double, 3>, QVector3D>::toRaw(
              plane->plane()->origin));
    else
      return QVariant::fromValue<vx::TupleVector<double, 3>>(
          this->standaloneOrigin);
  } else {
    return VisualizerNode::getNodePropertyCustom(key);
  }
}
void SliceVisualizer::setNodePropertyCustom(QString key, QVariant value) {
  if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration") {
    this->_filterChain2DWidget->getFilterChain()->fromXMLString(
        Node::parseVariant<QString>(value));
  } else if (key ==
             "de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation") {
    // Note: If there is a plane, setting the origin / orientation is ignored
    // TODO: Should this be the case?
    if (!properties->plane())
      this->standaloneOrientation =
          Node::parseVariant<vx::TupleVector<double, 4>>(value);
  } else if (key == "de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin") {
    // Note: If there is a plane, setting the origin / orientation is ignored
    // TODO: Should this be the case?
    if (!properties->plane())
      this->standaloneOrigin =
          Node::parseVariant<vx::TupleVector<double, 3>>(value);
  } else {
    VisualizerNode::setNodePropertyCustom(key, value);
  }
}

QSharedPointer<QObject> SliceVisualizer::getPropertyUIData(
    QString propertyName) {
  if (propertyName ==
      "de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping") {
    return histogramProvider;
  } else {
    return Node::getPropertyUIData(propertyName);
  }
}

void SliceVisualizer::onCanvasResized(QResizeEvent* event) {
  Q_UNUSED(event);
  this->_resizeTimer.stop();
  this->_resizeTimer.start(RESIZE_TIMER_TIMEOUT);
}

void SliceVisualizer::initializeFinished(QVector<float> values) {
  // TODO
  (void)values;
  /*
  this->clearMappings();
  qDebug() << "initFinisched Color" + QString::number(values.at(0)) + " _ " +
                  QString::number(values.at(1));
  this->addColorMapping(values.at(0), qRgba(0, 0, 0, 255));
  this->addColorMapping(values.at(1), qRgba(255, 255, 255, 255));
  */
}

void SliceVisualizer::createPlaneNode() {
  if (this->properties->plane()) {
    qWarning() << "SliceVisualizer::createPlaneNode(): SliceVisualizer "
                  "already has a plane";
    return;
  }
  auto plane = this->getCuttingPlane();
  auto planeProperty = createNode<PlaneNode>({
      {Prop::Orientation, plane.rotation},
      {Prop::Origin, plane.origin},
  });
  this->properties->setPlane(planeProperty.data());
}

void SliceVisualizer::updateCreatePlaneNode() {
  createPlaneNodeAction->setEnabled(!this->properties->plane());
}

void SliceVisualizer::redraw() { _imageDisplayingWidget->update(); }

QSharedPointer<vx::VolumeData> SliceVisualizer::volumeData() {
  auto volume = dynamic_cast<vx::VolumeNode*>(this->properties->volume());
  if (!volume)
    return QSharedPointer<vx::VolumeData>();
  else
    return volume->volumeData();
}

QString SliceVisualizer::getLoadedDataType() {
  auto volume = dynamic_cast<vx::VolumeNode*>(this->properties->volume());
  if (volume) {
    auto data = volume->data();
    auto volumeData = qSharedPointerDynamicCast<vx::VolumeData>(data);
    if (volumeData) {
      // ToDo: replace with real non-multivariate data type name
      return "non-multivariate data";
    } else {
      return "";
    }
  }

  auto volumeSeries =
      dynamic_cast<VolumeSeriesNode*>(this->properties->volume());
  if (volumeSeries) {
    auto data = volumeSeries ? volumeSeries->data() : QSharedPointer<Data>();
    auto seriesData = qSharedPointerDynamicCast<vx::VolumeSeriesData>(data);
    if (seriesData) {
      return seriesData->dimensions()[0]->property()->name();
    } else {
      return "";
    }
  }
  return "No data loaded";
}

QList<rawMetaData> SliceVisualizer::getMultivariateDimensionData() {
  QList<rawMetaData> outputList = QList<rawMetaData>();

  auto node = dynamic_cast<DataNode*>(this->properties->volume());
  auto data = node ? node->data() : QSharedPointer<Data>();
  auto dataSeries = qSharedPointerDynamicCast<VolumeSeriesData>(data);

  if (!dataSeries || dataSeries->dimensionCount() != 1) {
    // qDebug() << "No multivariate data to return";
    return QList<rawMetaData>();
  }

  // ToDo: get additional info about channel and save
  // them in rawMetaData.infoText
  auto dimension = dataSeries->dimensions()[0];
  for (SeriesDimension::EntryKey i = 0;
       i < (unsigned int)dimension->entries().size(); i++) {
    QString description =
        dimension->property()->type()->valueToString(dimension->entries()[i]);
    QString info = dimension->property()->type()->valueGetDescription(
        dimension->entries()[i]);

    rawMetaData metadata;
    metadata.description = description;
    metadata.infoText = info;
    metadata.entryIndex = i;
    outputList.append(metadata);
  }
  return outputList;
}

vx::PlaneInfo SliceVisualizer::getCuttingPlane() {
  return vx::PlaneInfo(this->properties->origin(),
                       this->properties->orientation());
}

void SliceVisualizer::movePlaneOrigin(const vx::Vector<double, 2>& planePos) {
  auto oldPlane = this->getCuttingPlane();
  auto origin = oldPlane.get3DPoint(planePos[0], planePos[1]);

  // TODO: Allow for rounding errors here?
  if (origin == oldPlane.origin) return;

  auto newPlane = oldPlane;
  newPlane.origin = origin;
  setPlaneInfo(newPlane, true);
}

// TODO: Remove setRotation() and setOrigin()?
void SliceVisualizer::setRotation(QQuaternion rotation) {
  // TODO: What should be passed for adjustCenterPoint here? Was:
  // old.isOnPlane(origin)
  this->setPlaneInfo(vx::PlaneInfo(this->properties->origin(), rotation),
                     false);
}
void SliceVisualizer::setOrigin(QVector3D origin) {
  // TODO: What should be passed for adjustCenterPoint here? Was:
  // (old.normal() - this->cuttingPlane.normal()).lengthSquared() < 1e-8
  this->setPlaneInfo(vx::PlaneInfo(origin, this->properties->orientation()),
                     false);
}
void SliceVisualizer::setPlaneInfo(const vx::PlaneInfo& newPlane,
                                   bool adjustCenterPoint) {
  auto oldPlane = getCuttingPlane();

  if (adjustCenterPoint && oldPlane.origin != newPlane.origin) {
    QPointF difference = newPlane.get2DPlanePoint(oldPlane.origin);
    this->properties->setCenterPoint(this->properties->centerPoint() +
                                     difference);
  }

  auto plane = dynamic_cast<PlaneNode*>(this->properties->plane());
  if (plane) {
    plane->setRotation(newPlane.rotation);
    plane->setOrigin(newPlane.origin);
  } else {
    this->properties->setOrientation(newPlane.rotation);
    this->properties->setOrigin(newPlane.origin);
  }

  // TODO: Is this needed?
  Q_EMIT signalRequestSliceImageUpdate();
}

void SliceVisualizer::multivariateDataPropertiesChangedIn() {
  Q_EMIT multivariateDataPropertiesChangedOut();
}

void SliceVisualizer::onFilterMaskRequest(vx::filter::Filter2D* filter) {
  this->selectionTool->setMask(filter);
  this->switchToolTo(this->selectionTool);
}

QSize SliceVisualizer::canvasSize() {
  return this->_imageDisplayingWidget->size();
}

// TODO: Lookups should not be done by object name (and the object name for
// the layers probably should not be set in the constructor)
int SliceVisualizer::getLayerIndexByName(QString name) {
  // TODO: This is broken
  int layerIndex = 0;
  for (int i = 0; i < this->layers().size(); i++) {
    if (this->layers()[0]->objectName().compare(name)) layerIndex = i;
  }
  return layerIndex;
}

void SliceVisualizer::addSegmentationFunctionality(quint8 brushRadius) {
  // get image layer index
  int imageLayerIdx = getLayerIndexByName(this->imageLayerName);

  // selection Layer
  auto selectionLayer = SelectionLayer::create(this);
  this->layers_.insert(imageLayerIdx + 1, selectionLayer);

  // labelLayer
  auto labelLayer = LabelLayer::create(this);
  this->layers_.insert(imageLayerIdx + 2, labelLayer);

  // Brush Layer
  auto brushLayer = BrushSelectionLayer::create(this);
  brushLayer->setBrushRadius(brushRadius);
  brushLayer->setObjectName(this->brushLayerName);
  this->layers_.insert(imageLayerIdx + 3, brushLayer);
  this->brushSelectionLayer = brushLayer.data();

  // Brush Tool
  BrushSelectionTool* brush = new BrushSelectionTool(view, this);
  brush->setBrushRadius(brushRadius);
  brush->setObjectName(this->brushSelectionName);
  this->brushSelectionTool = brush;
  this->_tools.append(brush);

  // Lasso Layer
  auto lassoLayer = LassoSelectionLayer::create(this);
  lassoLayer->setObjectName(this->lassoLayerName);
  this->layers_.insert(imageLayerIdx + 4, lassoLayer);
  this->lassoSelectionLayer = lassoLayer.data();

  // Lasso Tool
  auto* lasso = new LassoSelectionTool(view, this);
  lasso->setObjectName(this->lassoSelectionName);
  this->lassoSelectionTool = lasso;
  this->_tools.append(lasso);
}

void SliceVisualizer::activateBrushSelectionTool() {
  if (this->brushSelectionTool) switchToolTo(this->brushSelectionTool);
}

void SliceVisualizer::deactivateBrushSelectionTool() {
  if (this->currentTool()->objectName() == this->brushSelectionName) {
    if (this->defaultTool) {
      switchToolTo(this->defaultTool);
    }
  }
}

void SliceVisualizer::activateLassoSelectionTool() {
  if (this->lassoSelectionTool) switchToolTo(this->lassoSelectionTool);
}

void SliceVisualizer::deactivateLassoSelectionTool() {
  if (this->currentTool()->objectName() == this->lassoSelectionName) {
    if (this->defaultTool) {
      switchToolTo(this->defaultTool);
    }
  }
}

void SliceVisualizer::setBrushRadius(quint8 radius) {
  if (this->brushSelectionLayer)
    this->brushSelectionLayer->setBrushRadius(radius);

  if (this->brushSelectionTool)
    this->brushSelectionTool->setBrushRadius(radius);
}

quint8 SliceVisualizer::getBrushRadius() {
  if (this->brushSelectionTool)
    return this->brushSelectionTool->getBrushRadius();

  return 0;
}

QSharedPointer<vx::HistogramProvider>
SliceVisualizer::getSliceHistogramProvider() {
  return sliceHistogramProvider;
}

void SliceVisualizer::setHistogramColorizer(
    QSharedPointer<vx::Colorizer> colorizer) {
  _histogramWidget->setColorizer(colorizer);

  try {
    auto value = colorizer->getEntriesAsQList();
    properties->setValueColorMapping(value);

  } catch (Exception& e) {
    qCritical() << "SliceVisualizer::getSliceHistogramProvider Error while "
                   "updating property value:"
                << e.what();
  }
}

void SliceVisualizer::setVolume(vx::Node* volume) {
  this->properties->setVolume(volume);
}

QSharedPointer<vx::Colorizer> SliceVisualizer::getHistogramColorizer() {
  return _histogramWidget->colorizer();
}

QRectF SliceVisualizer::adjustedAreaAspect(const QRectF& area,
                                           const QSize& oldSize,
                                           const QSize& newSize) {
  QRectF newArea;
  if (oldSize.width() < 1 || oldSize.height() < 1) {
    if (newSize.width() < 1 || newSize.height() < 1) {
      return area;
    } else {
      newArea = area;
    }
  } else {
    qreal deltaW =
        (newSize.width() - oldSize.width()) / (oldSize.width() * 1.0);
    qreal deltaH =
        (newSize.height() - oldSize.height()) / (oldSize.height() * 1.0);
    deltaW *= area.width();
    deltaH *= area.height();
    newArea = QRectF(area.left(), area.top(), area.width() + deltaW,
                     area.height() + deltaH);
  }
  // adjust aspect
  qreal areaAspect = newArea.width() / newArea.height();
  qreal imageAspect = (newSize.width() * 1.0) / newSize.height();
  qreal imageAspectInv = (newSize.height() * 1.0) / newSize.width();

  if (fabs(areaAspect - imageAspect) >
      1e-5) {  // same as areaAspect != imageAspect
    if (imageAspect > areaAspect) {
      // image wider than area, lets add to area's width
      qreal width = newArea.height() * imageAspect;
      // let area grow left and right, so adjust x position
      qreal x = newArea.x();

      return QRectF(x, newArea.y(), width, newArea.height());
    } else {
      // image taller than area, lets add to area's height
      qreal height = newArea.width() * imageAspectInv;
      // let area grow top and bottom, so adjust y position
      qreal y = newArea.y();

      return QRectF(newArea.x(), y, newArea.width(), height);
    }
  } else {
    return newArea;
  }
}

QRectF SliceVisualizer::zoomedArea(const QRectF& area, qreal zoom) {
  if (zoom == 0) {
    return area;
  }
  qreal width = area.width() * zoom;
  qreal height = area.height() * zoom;
  // zoom to center
  qreal x = area.x() - ((width - area.width()) / 2);
  qreal y = area.y() - ((height - area.height()) / 2);
  return QRectF(x, y, width, height);
}

vx::InterpolationMethod SliceVisualizer::getInterpolation(
    vx::SlicePropertiesBase* properties) {
  return getInterpolationFromString(properties->interpolation());
}

void SliceVisualizer::renderEverything(
    QImage& outputImage, const QSharedPointer<vx::ParameterCopy>& parameters,
    bool isMainImage) {
  imageLayer->render(outputImage, parameters, isMainImage);

  // TODO: when renderEverything() is called from a background thread, is it
  // okay to access layers()? => Make a copy of layers()
  for (const auto& tool : this->layers()) {
    tool->render(outputImage, parameters, isMainImage);
  }
}

vx::SharedFunPtr<VisualizerNode::RenderFunction>
SliceVisualizer::getRenderFunction() {
  return [this](const QSharedPointer<vx::ImageDataPixel>& outputImage,
                const vx::VectorSizeT2& outputRegionStart,
                const vx::VectorSizeT2& size,
                const QSharedPointer<vx::ParameterCopy>& parameters,
                const QSharedPointer<vx::VisualizerRenderOptions>& options) {
    // TODO: This lambda should not capture this, but currently this is
    // needed because tools is accessed (and the tools will be destroyed
    // when the SliceVisualizer is destroyed).

    Q_UNUSED(options);

    // The main drawing function does not call getRenderFunction()
    bool isMainImage = false;

    quint64 width = size.x;
    quint64 height = size.y;

    if (!outputImage)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Output image is nullptr");
    if (outputRegionStart.x > outputImage->width() ||
        (outputRegionStart.x + width) > outputImage->width() ||
        (outputRegionStart.x + width) < width ||
        outputRegionStart.y > outputImage->height() ||
        (outputRegionStart.y + height) > outputImage->height() ||
        (outputRegionStart.y + height) < height)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Region is outsize image");

    QImage qimage(width, height, QImage::Format_ARGB32);
    qimage.fill(qRgba(0, 0, 0, 0));  // Fill will transparent // TODO:
                                     // doesn't really work currently
    renderEverything(qimage, parameters, isMainImage);
    outputImage->fromQImage(qimage, outputRegionStart);
  };
}

void SliceVisualizer::addToDrawStack(Layer* self, const QImage& image) {
  int idx;
  for (idx = 0; idx < layers().size(); idx++) {
    if (layers().at(idx) == self) {
      break;
    }
  }
  if (idx == layers().size()) {
    qWarning() << "SliceVisualizer::addToDrawStack(): Cannot find layer"
               << self;
    qDebug() << "===";
    for (const auto& layer : this->layers()) qDebug() << "Layer:" << layer;
    qDebug() << "===";
    return;
  }
  _drawStack.remove(idx);
  _drawStack.insert(idx, image);
}

bool SliceVisualizer::isAllowedParent(vx::NodeKind object) {
  return object == NodeKind::Data || object == NodeKind::Property ||
         object == NodeKind::Filter;
}

void SliceVisualizer::updateBoundingBox() {
  BoundingBox3D bb = BoundingBox3D::empty();
  auto volume = dynamic_cast<MovableDataNode*>(this->properties->volume());
  if (volume) bb = volume->boundingBoxGlobal();
  if (bb.isEmpty())
    // Default BB
    bb = BoundingBox3D::point(vx::Vector<double, 3>(-0.2, -0.2, -0.2)) +
         BoundingBox3D::point(vx::Vector<double, 3>(0.2, 0.2, 0.2));

  this->view3d()->setBoundingBox(bb);
}
