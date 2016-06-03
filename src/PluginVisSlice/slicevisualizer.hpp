#pragma once

#include <PluginVisSlice/histogramwidget.hpp>
#include <PluginVisSlice/imagegeneratorworker.hpp>
#include <PluginVisSlice/sliceimagecolorizerwidget.hpp>
#include <PluginVisSlice/toolselection.hpp>
#include <PluginVisSlice/toolvisualizer2d.hpp>

#include <Voxie/data/slice.hpp>
#include <Voxie/data/sliceimage.hpp>
#include <Voxie/data/sliceimage.hpp>

#include <Voxie/visualization/filterchain2dwidget.hpp>
#include <Voxie/visualization/visualizer.hpp>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QVarLengthArray>

#include <QtGui/QPaintEvent>

#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

class ImagePaintWidget;

/**
 * @brief The SliceVisualizer class is a godclass mainly responsible for initializing and handling the data flow between all slice view operations.
 * @author Hans Martin Berner, David Haegele
 */

class SliceVisualizer : public voxie::visualization::SliceDataVisualizer
{
	friend class ImagePaintWidget;
	Q_OBJECT
private: // magic numbers
	int RESIZE_TIMER_TIMEOUT = 100;
private:
	//sidebar widgets
	ImagePaintWidget* _imageDisplayingWidget;
	voxie::visualization::FilterChain2DWidget *_filterChain2DWidget;
	SliceImageColorizerWidget *_colorizerWidget;
	HistogramWidget *_histogramWidget;


	//data
	voxie::data::Slice* _slice;
	voxie::data::SliceImage _sliceImage;
	voxie::data::SliceImage _filteredSliceImage;
	QImage _displayedImage;
    QPointF center_;
    float sizeInM_; // currently is height of image

	QMap<int, QImage> _drawStack;

	//for window elements
	QVBoxLayout* hobox;
	QWidget* toolBar;
	QVarLengthArray<Visualizer2DTool*> _tools;
	int _currentTool = 0; // init value is tool to be activated when window is created

	/**
	 * Handles resizes. We don't want to regenerate an image on each resize event as this would need unnecessary amount of resources
	 */
	QTimer _resizeTimer;

	/**
	 * Makes sure that only one worker thread is running at a time.
	 */
	bool _imageWorkerRunning = false;

	/**
	 * The current worker that was queued up should another worker be running already. This makes sure that we have always the latest request in the queued up.
	 */
	ImageGeneratorWorker* _imageQueueWorker = nullptr;

	bool _filterWorkerRunning = false;
	bool _filterImageWorked = false;

	voxie::data::SliceImage _filterQueueImage;

	/**
	 * selection tool
	 */
	ToolSelection* selectionTool;

private:
	void doGenerateSliceImage(voxie::data::Slice* slice,
							 const QRectF &sliceArea,
							 const QSize &imageSize,
							 voxie::data::InterpolationMethod interpolation = voxie::data::linear);
	void runSliceImageGeneratorWorker();
	void runFilterWorker();
public:
	explicit SliceVisualizer(QVector<voxie::data::Slice*> slices, QWidget *parent = 0);
	//~SliceVisualizer();

	/**
	 * Notifies the widget that draws the canvas that it needs to redraw the draw stack.
	 */
	void redraw();

	/**
	 * @return the current unfiltered slice image
	 */
	voxie::data::SliceImage& sliceImage() {
		return this->_sliceImage;
	}

	/**
	 * @return the current filtered slice image
	 */
	voxie::data::SliceImage& filteredSliceImage() {
		return this->_filteredSliceImage;
	}

	/**
	 * @return the currently displayed image
	 */
	QImage& displayedImage() {
		return this->_displayedImage;
	}

	/**
	 * Notifies the tool that it has been activated. Notifies the previous tool that it has been deactivated. This tool now gets forwarded all key and mouse events.
	 * @param tool pointer to the tool.
	 */
	void switchToolTo(Visualizer2DTool* tool);

	/**
	 * Returns the currently selected tool.
	 * @see switchToolTo(Visualizer2DTool *tool);
	 */
	Visualizer2DTool* currentTool() {
		return _tools.at(_currentTool);
	}

	/**
	 * adds an image to the draw stack. The image at -1 is the one generated from a slice, everything >=0 has a tool as source. Images are mapped to the pointer of the tool that added them.
	 * @param self pointer to the object the image is associated with
	 * @param image to be added or replaced
	 */
	void addToDrawStack(QObject* self, QImage image) {
		int idx;
		if((SliceVisualizer*)self == this) {
			idx = -1;
		} else {
			for(idx = 0; idx < _tools.size(); idx++) {
				if(_tools.at(idx) == (Visualizer2DTool*)self) {
					break;
				}
			}
			if(idx == _tools.size()) {
				return;
			}
		}
		_drawStack.remove(idx);
		_drawStack.insert(idx, image);
	}

	/**
	 * @return the current drawStack consisting of a mapping of index to image. Smallest index is at the bottom. Begins at -1 (as >= 0 are tools).
	 */
	QMap<int, QImage> drawStack() {
		return _drawStack;
	}

	/**
	 * @return array of tools added to the visualizer
	 */
	QVarLengthArray<Visualizer2DTool*> tools() {
		return _tools;
	}

	/**
	 * @return the current plane area describing the current position and size of the sliceimage inside the visualizer
	 */
	QRectF currentPlaneArea() {
		//return _currentPlaneArea;
        int widthC = this->canvasWidth();
        int heightC = this->canvasHeight();
        float aspectRatio = (float) widthC / heightC;
        float height = sizeInM_;
        float width = height * aspectRatio;
		return QRectF (center_.x() - width/2, center_.y() - height/2, width, height);
	}

	/**
	 * @return the slice associated with this visualizer. This pointer originates from outside the plugin.
	 */
	virtual voxie::data::Slice* slice() final {
		return this->_slice;
	}

	/**
	 * Resets the current plane area to the bounding rectangle putting it into the upper left corner.
	 */
	void resetPlaneArea() {
		//_currentPlaneArea = slice()->getBoundingRectangle();
        QRectF bbox = slice()->getBoundingRectangle();
		center_ = bbox.center();
        sizeInM_ = bbox.height() * 1.1; // TODO: Use diagonal though entire volume instead?
	}

	/**
	 * @param multiplier zooms the plane area by a given delta. Multiplier is multiplied with current zoom. e.g.
	 */
	void zoomPlaneArea(qreal multiplier)
	{
		//_currentPlaneArea = zoomedArea(_currentPlaneArea, multiplier);
        sizeInM_ /= multiplier;
	}

	/**
	 * Moves the current plane (representing the position of the image inside the canvas) by the given values
	 * @param pixelDeltaX delta x
	 * @param pixelDeltaX delta y
	 */
	void moveArea(qreal pixelDeltaX, qreal pixelDeltaY){
		qreal relx = pixelDeltaX / this->canvasWidth();
		qreal rely = pixelDeltaY / this->canvasHeight();
		relx *= currentPlaneArea().width();
		rely *= currentPlaneArea().height();
		//_currentPlaneArea.translate(-relx, -rely);
        center_ -= QPointF(relx, rely);
	}

	/**
	 * @return the canvas height representing the whole drawing area reserved for the slice. This does not include borders or tools.
	 */
	int canvasHeight() {
		return canvasSize().height();
	}

	/**
	 * @return the canvas width representing the whole drawing area reserved for the slice. This does not include borders or tools.
	 */
	int canvasWidth() {
		return canvasSize().width();
	}

	/**
	 * @return The size of the area reserved for displaying the rendered image (not the window, not the toolbar)
	 */
	QSize canvasSize();

	static QRectF adjustedAreaAspect(const QRectF &area, const QSize &oldSize, const QSize& newSize);

	static QRectF zoomedArea(const QRectF& area, qreal zoom);

public slots:
	/**
	 * @brief onSliceImageGenerated is called when a slice image has been generated from a slice (step 1)
	 * @param image
	 */
	void onSliceImageGenerated(voxie::data::SliceImage image);
	/**
	 * @brief onSliceImageFiltered is called when an image has been filtered (step 2)
	 * @param image
	 */
	void onSliceImageFiltered(voxie::data::SliceImage image);
	/**
	 * @brief onImageColorized is called when an image has been colorized (step 3)
	 * @param image
	 */
	void onImageColorized(QImage image);
	/**
	 * @brief updateBaseImage is called when a generated, filtered and colorized image is ready to be displayed (step 4)
	 * @param baseImage
	 */
	void updateBaseImage(QImage baseImage);
	/**
	 * @brief onSliceChanged is called when the slice changes
	 * @param oldPlane
	 * @param newPlane
	 * @param equivalent
	 */
	void onSliceChanged(const voxie::data::Plane& oldPlane, const voxie::data::Plane& newPlane, bool equivalent);
	/**
	 * @brief onDatasetChanged is called when the dataset associated with the slice changes
	 */
	void onDatasetChanged();
	/**
	 * @brief applyFilters is called when the threaded filter call should be invoked. It clones the currently generated sliceimage.
	 */
	void applyFilters();
	/**
	 * @brief updateSliceImageFromSlice is called when the sliceimage should be generated (for step 1)
	 */
	void updateSliceImageFromSlice();
	/**
	 * @brief onFilterMaskRequest is called when the filterchain2d widget (section) requests for the filter mask to be initialized. This is necessary because the pointer is normally not known outside of this plugin.
	 * @param mask pointer to the selection mask
	 */
	void onFilterMaskRequest(voxie::filter::Selection2DMask* mask);
	/**
	 * @brief onCanvasResized is called when the ImagePaintWidget is resized
	 */
	void onCanvasResized(QResizeEvent *);

signals:
	// for tools
	void sliceImageChanged(voxie::data::SliceImage& image);

	// for data flow
	void signalBaseImageChanged(QImage baseImage);
	void signalRequestColorization(voxie::data::SliceImage& image);
	void signalRequestSliceImageUpdate();
	void resized();
	void signalRequestHistogram(voxie::data::SliceImage& image);
public slots:
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
