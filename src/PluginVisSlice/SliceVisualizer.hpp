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

#include <VoxieBackend/Data/InterpolationMethod.hpp>

#include <QtWidgets/QComboBox>

#include <PluginVisSlice/BrushSelectionTool.hpp>
#include <PluginVisSlice/LassoSelectionTool.hpp>
#include <PluginVisSlice/MultivariateDataWidget/helpingStructures.hpp>
#include <PluginVisSlice/Prototypes.forward.hpp>
#include <PluginVisSlice/Prototypes.hpp>  // TODO: Should not be needed here, move stuff out of header
#include <PluginVisSlice/ToolSelection.hpp>
#include <PluginVisSlice/Visualizer2DTool.hpp>

#include <PluginVisSlice/Grid.hpp>
#include <PluginVisSlice/ImageLayer.hpp>
#include <PluginVisSlice/LabelLayer.hpp>
#include <PluginVisSlice/LegendLayer.hpp>
#include <PluginVisSlice/SelectionLayer.hpp>

#include <QRadioButton>
#include <Voxie/Data/InitializeColorizeWorker.hpp>

#include <Voxie/Data/VolumeSeriesNode.hpp>  //ToDo: Add to vx namespace
// #include <VoxieBackend/Data/VolumeSeriesData.hpp>
#include <Voxie/PropertyObjects/PlaneNode.hpp>
#include <VoxieBackend/Data/SliceImage.hpp>

#include <Voxie/Interfaces/SliceVisualizerI.hpp>

#include <Voxie/Vis/FilterChain2DWidget.hpp>
#include <Voxie/Vis/HistogramWidget.hpp>
#include <Voxie/Vis/VisualizerNode.hpp>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVarLengthArray>
#include <QtCore/QVector>

#include <QtGui/QPaintEvent>

#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

class ImagePaintWidget;

/**
 * @brief The SliceVisualizer class is a godclass mainly responsible for
 * initializing and handling the data flow between all slice view operations.
 * @author Hans Martin Berner, David Haegele
 */

namespace vx {
class ColorizerEntry;
class HistogramProvider;
class ParameterCopy;

namespace visualization {
class View3D;
}
}  // namespace vx

class InfoWidget;
class MultivariateDataWidget;
class ImageLayer;

class SliceVisualizer : public vx::VisualizerNode, public vx::SliceVisualizerI {
  Q_OBJECT
  VX_NODE_IMPLEMENTATION_PUB("de.uni_stuttgart.Voxie.Visualizer.Slice")

  friend class ImagePaintWidget;

 public:
  const QString imageLayerName = "ImageLayer";
  const QString lassoLayerName = "LassoLayer";
  const QString brushLayerName = "BrushLayer";
  const QString brushSelectionName = "BrushSelectionTool";
  const QString lassoSelectionName = "LassoSelectionTool";

  MultivariateDataWidget* getMultivariateDataWidget() {
    return this->multivariateDataWidget;
  };

 private:  // magic numbers
  int RESIZE_TIMER_TIMEOUT = 100;

  QWidget* view;
  QComboBox* box;

  QSharedPointer<vx::visualization::View3D> view3d_;

  // sidebar widgets
  ImagePaintWidget* _imageDisplayingWidget;
  vx::visualization::FilterChain2DWidget* _filterChain2DWidget;

  QWidget* histogramBox;
  QRadioButton* sliceRadioButton;
  QRadioButton* volumeRadioButton;
  vx::HistogramWidget* _histogramWidget;

  MultivariateDataWidget* multivariateDataWidget;
  InfoWidget* infoWidget;

  // Only used if no plane is connected
  vx::TupleVector<double, 4> standaloneOrientation =
      vx::TupleVector<double, 4>(1, 0, 0, 0);
  vx::TupleVector<double, 3> standaloneOrigin =
      vx::TupleVector<double, 3>(0, 0, 0);

  QSharedPointer<vx::HistogramProvider> sliceHistogramProvider;
  QSharedPointer<vx::HistogramProvider> histogramProvider;
  QMetaObject::Connection histogramConnection;

  // TODO: clean up
  QMap<int, QImage> _drawStack;

  // for window elements
  QVBoxLayout* hobox;
  QWidget* toolBar;
  QVarLengthArray<Visualizer2DTool*> _tools;
  QList<QSharedPointer<Layer>> layers_;

  int _currentTool =
      0;  // init value is tool to be activated when window is created

  vx::BrushSelectionTool* brushSelectionTool = nullptr;
  Visualizer2DTool* defaultTool = nullptr;
  vx::BrushSelectionLayer* brushSelectionLayer = nullptr;

  vx::LassoSelectionTool* lassoSelectionTool = nullptr;
  vx::LassoSelectionLayer* lassoSelectionLayer = nullptr;

  vx::InitializeColorizeWorker* initializeWorker;

  /**
   * When a change of the Plane is based by a PlaneNode change, don't
   * propagate it back to the PlaneNode
   */
  bool propagateToPlaneNode = true;

  /**
   * Handles resizes. We don't want to regenerate an image on each resize event
   * as this would need unnecessary amount of resources
   */
  QTimer _resizeTimer;

  /**
   * selection tool
   */
  ToolSelection* selectionTool;

  QAction* createPlaneNodeAction;

  // Override needed for SegmentationFilter
  bool isAllowedParent(vx::NodeKind object) override;

 protected:
  QWidget* getCustomPropertySectionContent(const QString& name) override;

 public:
  explicit SliceVisualizer();

  QWidget* mainView() override { return view; }

  const QSharedPointer<vx::visualization::View3D>& view3d() { return view3d_; }

  /**
   * @brief Appends all relevant segmentation functionality to the
   * SliceVisualizer: SelectionLayer, LabelLayer, BrushSelectionLayer &
   * BrushSelectionTool
   * @param brushRadius Radius of the brush in pixels
   */
  void addSegmentationFunctionality(quint8 brushRadius);

  /**
   * @brief Sets the brush selection tool as currently active tool
   */
  void activateBrushSelectionTool() override;

  /**
   * @brief Sets the DefaultTool as currently active tool
   */
  void deactivateBrushSelectionTool() override;

  /**
   * @brief Sets the lasso selection tool as currently active tool
   */
  void activateLassoSelectionTool() override;

  /**
   * @brief Set the current tool to DefaultTool if LassoSelection is active
   */
  void deactivateLassoSelectionTool() override;

  /**
   * @brief Sets the brush Radius in pixels
   */
  void setBrushRadius(quint8 radius) override;

  /**
   * @brief returns the current Brush radius in pixels
   */
  quint8 getBrushRadius() override;

  /**
   * @brief Returns the histogram provider of slice visualizer histogram widget
   */
  virtual QSharedPointer<vx::HistogramProvider> getSliceHistogramProvider()
      override;

  /**
   * @brief Sets the histogram provider of slice visualizer histogram widget
   */
  virtual void setHistogramColorizer(
      QSharedPointer<vx::Colorizer> colorizer) override;

  /**
   * @brief Get the colorizer of slice visualizer histogram widget
   */
  virtual QSharedPointer<vx::Colorizer> getHistogramColorizer() override;

  /**
   * @brief Set the volume property
   */
  void setVolume(vx::Node* volume) override;

 private:
  /**
   * Notifies the widget that draws the canvas that it needs to redraw the draw
   * stack.
   */
  void redraw();

 public:
  QSharedPointer<vx::VolumeData> volumeData();

  /** This getter return the current data type of the loaded data set.
   *  If no data is loaded it returns a empty QString.
   */
  QString getLoadedDataType();

  /**
   * This Getter returns a list of meta infomation about all multivariate data
   * channels.
   */
  QList<rawMetaData> getMultivariateDimensionData();

  /**
   * Notifies the tool that it has been activated. Notifies the previous tool
   * that it has been deactivated. This tool now gets forwarded all key and
   * mouse events.
   * @param tool pointer to the tool.
   */
  void switchToolTo(Visualizer2DTool* tool);

  /**
   * Returns the currently selected tool.
   * @see switchToolTo(Visualizer2DTool *tool);
   */
  Visualizer2DTool* currentTool() { return _tools.at(_currentTool); }

 private:
  /**
   * Add an image to the draw stack.
   * @param self pointer to the object the image is associated with
   * @param image to be added or replaced
   */
  void addToDrawStack(Layer* self, const QImage& image);

 public:
  /**
   * @return the current drawStack consisting of a mapping of index to image.
   * Smallest index is at the bottom. Begins at -1 (as >= 0 are tools).
   */
  QMap<int, QImage> drawStack() { return _drawStack; }

  int getLayerIndexByName(QString name);

  /**
   * @return array of tools added to the visualizer
   */
  QVarLengthArray<Visualizer2DTool*> tools() { return _tools; }

  QSharedPointer<Layer> imageLayer;

  const QList<QSharedPointer<Layer>>& layers() { return layers_; }

  // Get the properties and size for the currently shown image.
  // The returned value can only be used on the main thread and will be
  // invalidated by returning the the main loop or by changing anything about
  // the slice visualizer.
  // TODO: This does not really work currently, because "currently shown image"
  // is unclear because there are multiple layers which are all shown as soon as
  // the they are rendered
  std::tuple<vx::SlicePropertiesBase*, QSize> currentImageProperties();

  static double getCurrentPixelSize(vx::SlicePropertiesBase* properties,
                                    const QSize& canvasSize);

  /**
   * @return the current plane area describing the current position and size of
   * the sliceimage inside the visualizer
   */
  static QRectF getCurrentPlaneArea(vx::SlicePropertiesBase* properties,
                                    const QSize& canvasSize);
  QRectF currentPlaneArea();

  // Convert a pixel position (where (0,0) is lower left and (width,height) is
  // upper right) to a position on the plane.
  static vx::Vector<double, 2> pixelPosToPlanePos(
      vx::SlicePropertiesBase* properties, const QSize& canvasSize,
      const vx::Vector<double, 2>& pixelPos);
  // Same as pixelPosToPlanePos for the currently shown image
  vx::Vector<double, 2> pixelPosToPlanePosCurrentImage(
      const vx::Vector<double, 2>& pixelPos);

  // Convert a plane position to a 3D position.
  static vx::Vector<double, 3> planePosTo3DPos(
      vx::SlicePropertiesBase* properties, const QSize& canvasSize,
      const vx::Vector<double, 2>& planePos);
  // Same as planePosTo3DPos for the currently shown image
  vx::Vector<double, 3> planePosTo3DPosCurrentImage(
      const vx::Vector<double, 2>& planePos);

  // Convert a pixel position (where (0,0) is lower left and (width,height) is
  // upper right) to a 3D position.
  static vx::Vector<double, 3> pixelPosTo3DPos(
      vx::SlicePropertiesBase* properties, const QSize& canvasSize,
      const vx::Vector<double, 2>& pixelPos);
  // Same as pixelPosTo3DPos for the currently shown image
  vx::Vector<double, 3> pixelPosTo3DPosCurrentImage(
      const vx::Vector<double, 2>& pixelPos);

  void updateBoundingBox();

  /**
   * @param multiplier zooms the plane area by a given delta. Multiplier is
   * multiplied with current zoom. e.g.
   */
  void zoomPlaneArea(qreal multiplier) {
    //_currentPlaneArea = zoomedArea(_currentPlaneArea, multiplier);
    this->properties->setVerticalSize(this->properties->verticalSize() /
                                      multiplier);
  }

  /**
   * Moves the current plane (representing the position of the image inside the
   * canvas) by the given values
   * @param pixelDeltaX delta x
   * @param pixelDeltaX delta y
   */
  void moveArea(qreal pixelDeltaX, qreal pixelDeltaY) {
    qreal relx = pixelDeltaX / this->canvasWidth();
    qreal rely = pixelDeltaY / this->canvasHeight();
    relx *= currentPlaneArea().width();
    rely *= currentPlaneArea().height();
    //_currentPlaneArea.translate(-relx, -rely);
    this->properties->setCenterPoint(this->properties->centerPoint() -
                                     QPointF(relx, rely));
  }

  /**
   * @return the canvas height representing the whole drawing area reserved for
   * the slice. This does not include borders or tools.
   */
  int canvasHeight() { return canvasSize().height(); }

  /**
   * @return the canvas width representing the whole drawing area reserved for
   * the slice. This does not include borders or tools.
   */
  int canvasWidth() { return canvasSize().width(); }

  /**
   * @return The size of the area reserved for displaying the rendered image
   * (not the window, not the toolbar)
   */
  QSize canvasSize();

  static QRectF adjustedAreaAspect(const QRectF& area, const QSize& oldSize,
                                   const QSize& newSize);

  static QRectF zoomedArea(const QRectF& area, qreal zoom);

  // These methods will update either the internal plane settings or the
  // settings of the connected plane
  void setRotation(QQuaternion rotation) override;
  void setOrigin(QVector3D origin);
  // Set both rotation and origin
  void setPlaneInfo(const vx::PlaneInfo& plane, bool adjustCenterPoint);

  QVariant getNodePropertyCustom(QString key) override;
  void setNodePropertyCustom(QString key, QVariant value) override;

  QSharedPointer<QObject> getPropertyUIData(QString propertyName) override;

  static vx::InterpolationMethod getInterpolation(
      vx::SlicePropertiesBase* properties);

  // Should be able to run in a background thread
  void renderEverything(QImage& outputImage,
                        const QSharedPointer<vx::ParameterCopy>& parameters,
                        bool isMainImage);

  vx::SharedFunPtr<RenderFunction> getRenderFunction() override;

  // TODO: Should probably be removed
  vx::PlaneInfo getCuttingPlane();

  /**
   * @brief shifts slice plane's origin to another point on the plane.
   * @param planePoint point on this slice's plane.
   */
  void movePlaneOrigin(const vx::Vector<double, 2>& planePos);

 public Q_SLOTS:

  /**
   * This Slot bridges from multivariate widget to slice visualitzer to image
   * layer. This Slots emits multivariateDataPropertiesChangedOut.
   */
  void multivariateDataPropertiesChangedIn();
  /**
   * @brief onFilterMaskRequest is called when the filterchain2d widget
   * (section) requests for the filter mask to be initialized. This is necessary
   * because the pointer is normally not known outside of this plugin.
   * @param mask pointer to the selection mask
   */
  void onFilterMaskRequest(vx::filter::Filter2D* mask);
  /**
   * @brief onCanvasResized is called when the ImagePaintWidget is resized
   */
  void onCanvasResized(QResizeEvent*);

  void initializeFinished(QVector<float> values);

  /**
   * Create a separate plane property node.
   */
  void createPlaneNode();
  void updateCreatePlaneNode();

 public:
  static QPointF planePointToPixel(const QSize& imageSize,
                                   const QRectF& planeArea,
                                   const QPointF& planePoint);

 Q_SIGNALS:
  // for data flow
  void signalBaseImageChanged(QImage baseImage);
  void signalRequestSliceImageUpdate();
  void resized();
  void signalRequestHistogram(vx::SliceImage& image);

  /**
   * This signal is a part of the bridge from multivariate widget to
   * SliceVisualizer to ImageLayer. Signals ImageLayer to redraw cause of
   * multivariate propeties changed.
   */
  void multivariateDataPropertiesChangedOut();

  void imageMouseMove(QMouseEvent* e, const vx::Vector<double, 2>& planePos,
                      const vx::Vector<double, 3>& pos3D,
                      const vx::Vector<double, 3>* posVoxelPtr,
                      double valNearest, double valLinear);

  // Signals forwarded from GeometricPrimitiveNode
  void newMeasurement();
  void currentPointChanged();
  void gpoDataChangedFinished();

  // Signals forwarded from PlaneNode
  void rotationChangedForward();
  void originChangedForward();

  // Signals forwarded from VolumeData / VolumeNode
  void volumeDataChangedFinished();
  void volumeDataRotationChanged();
  void volumeDataTranslationChanged();

  // Signals forwarded from LabelContainer
  void labelContainerChangedFinished();

  void volumeDisplayNameChanged();
};
