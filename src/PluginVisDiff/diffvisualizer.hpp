#pragma once

#include <PluginVisDiff/diffimagecolorizerwidget.hpp>
#include <PluginVisDiff/histogramwidget.hpp>
#include <PluginVisDiff/imagegeneratorworker.hpp>
#include <PluginVisDiff/toolselection.hpp>
#include <PluginVisDiff/toolvisualizer2d.hpp>

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
 * @brief The DiffVisualizer class is a godclass mainly responsible for initializing and handling the data flow between all diff view operations.
 * @author Hans Martin Berner, David Haegele, Tim Borner
 */
class DiffVisualizer : public voxie::visualization::Visualizer
{
    friend class ImagePaintWidget;
    Q_OBJECT
private: // magic numbers
    int RESIZE_TIMER_TIMEOUT = 100;
private:
    QWidget* view;

    //sidebar widgets
    ImagePaintWidget* _imageDisplayingWidget;
    voxie::visualization::FilterChain2DWidget *_filterChain2DWidget;
    DiffImageColorizerWidget *_colorizerWidget;
    HistogramWidget *_histogramWidget;

    //data
    QVector<voxie::data::Slice*> _slices;
    voxie::data::FloatImage _floatImage;
    voxie::data::SliceImage _sliceImageFirst;
    voxie::data::SliceImage _sliceImageSecond;
    voxie::data::SliceImage _filteredSliceImage;
    QImage _displayedImage;
    QRectF _currentPlaneAreaFirstSlice;
    QRectF _currentPlaneAreaSecondSlice;

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
    void doGenerateSliceImage(QVector<voxie::data::Slice*> slices,
                              const QRectF &sliceAreaFirst,
                              const QRectF &sliceAreaSecond,
                              const QSize &imageSize,
                              voxie::data::InterpolationMethod interpolation = voxie::data::linear);
    void runSliceImageGeneratorWorker();
    void runFilterWorker();


public:
    explicit DiffVisualizer(QVector<voxie::data::Slice*> slices, QWidget *parent = 0);
    //~DiffVisualizer();

    QWidget* mainView() override {
        return view;
    }

    /**
     * Notifies the widget that draws the canvas that it neds to redraw the draw stack.
     */
    void redraw();

    /**
     * @return the current unfiltered float image
     */
    voxie::data::FloatImage& floatImage(){return this->_floatImage;}

    /**
     * @return the current unfiltered slice image from the first slice
     */
    voxie::data::SliceImage& sliceImageFirst(){return this->_sliceImageFirst;}

    /**
     * @return the current unfiltered slice image from the second slice
     */
    voxie::data::SliceImage& sliceImageSecond(){return this->_sliceImageSecond;}

    /**
     * @return the current filtered image
     */
    voxie::data::SliceImage& filteredSliceImage(){return this->_filteredSliceImage;}

    /**
     * @return the currently displayed image
     */
    QImage& displayedImage(){return this->_displayedImage;}

    /**
     * Notifies the tool that it has been activated. Notifies the previous tool that it has been deactivated. This tool now gets forwarded all key and mouse events.
     * @param tool pointer to the tool.
     */
    void switchToolTo(Visualizer2DTool* tool);

    /**
     * Returns the currently selected tool.
     * @see switchToolTo(Visualizer2DTool* tool);
     */
    Visualizer2DTool* currentTool() {
        return _tools.at(_currentTool);
    }

    /**
     * adds an image to the draw stack. The image at -1 is the one generated from a slice, everything >=0 has a tool as source. Images are mapped to the pointer of the tool that added them.
     * @param self pointer to the object the image is associated with
     * @param image to be added or replaced
     */
    void addToDrawStack(QObject* self, QImage im) {
        int idx;
        if((DiffVisualizer*)self == this) {
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
        //qDebug() << idx;
        _drawStack.remove(idx);
        _drawStack.insert(idx, im);
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
     * @return the current plane area for the first slice describing the current position and size of the sliceimage inside the visualizer
     */
    QRectF currentPlaneAreaFirstSlice() {
        return _currentPlaneAreaFirstSlice;
    }

    /**
     * @return the current plane area for the second slice describing the current position and size of the sliceimage inside the visualizer
     */
    QRectF currentPlaneAreaSecondSlice() {
        return _currentPlaneAreaSecondSlice;
    }

    /**
     * Resets the current plane areas to the bounding rectangle putting it into the upper left corner.
     */
    void resetPlaneArea() {
        _currentPlaneAreaFirstSlice = slices().at(0)->getBoundingRectangle();
        _currentPlaneAreaSecondSlice = slices().at(1)->getBoundingRectangle();
    }


    /**
     * @param multiplier zooms the plane areas by a given delta. Multiplier is multiplied with current zoom. e.g.
     */
    void zoomPlaneArea(qreal delta)
    {
        _currentPlaneAreaFirstSlice = zoomedArea(_currentPlaneAreaFirstSlice, delta);
        _currentPlaneAreaSecondSlice = zoomedArea(_currentPlaneAreaSecondSlice, delta);
    }

    /**
     * Moves the current plane (representing the position of the image inside the canvas) by the given values
     * @param pixelDeltaX delta x
     * @param pixelDeltaX delta y
     */
    void moveArea(qreal pixelDeltaX, qreal pixelDeltaY){
        qreal relx = pixelDeltaX / this->canvasWidth();
        qreal rely = pixelDeltaY / this->canvasHeight();
        qreal relx1 = relx * _currentPlaneAreaFirstSlice.width();
        qreal rely1 = rely * _currentPlaneAreaFirstSlice.height();
        _currentPlaneAreaFirstSlice.translate(-relx1, -rely1);

        qreal relx2 = relx * _currentPlaneAreaSecondSlice.width();
        qreal rely2 = rely * _currentPlaneAreaSecondSlice.height();
        _currentPlaneAreaSecondSlice.translate(-relx2, -rely2);
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

    /**
     * @return the slices associated with this visualizer. These pointers originate from outside the plugin.
     */
    QVector<voxie::data::Slice*> slices() {return this->_slices;}

    static QRectF adjustedAreaAspect(const QRectF &area, const QSize &oldSize, const QSize& newSize);

    static QRectF zoomedArea(const QRectF& area, qreal zoom);


public slots:
    /**
     * @brief onDiffImageGenerated is called when a diff image has been generated from a slice (step 1)
     * @param image
     */
    void onDiffImageGenerated(DiffSliceImage *dsi);
    /**
     * @brief onSliceImageFiltered is called when an diff image has been filtered (step 2)
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
     * @brief onFirstSliceChanged is called when the first slice changes
     * @param oldPlane
     * @param newPlane
     * @param equivalent
     */
    void onFirstSliceChanged(const voxie::data::Plane& oldPlane, const voxie::data::Plane& newPlane, bool equivalent);
    /**
     * @brief onSecondSliceChanged is called when the second slice changes
     * @param oldPlane
     * @param newPlane
     * @param equivalent
     */
    void onSecondSliceChanged(const voxie::data::Plane& oldPlane, const voxie::data::Plane& newPlane, bool equivalent);
    /**
     * @brief onDatasetChangedFirstSlice is called when the dataset associated with the first slice changes
     */
    void onDatasetChangedFirstSlice();
    /**
     * @brief onDatasetChangedSecondSlice is called when the dataset associated with the second slice changes
     */
    void onDatasetChangedSecondSlice();
    /**
     * @brief applyFilters is called when the threaded filter call should be invoked. It clones the currently generated sliceimage.
     */
    void applyFilters();
    /**
     * @brief updateDiffImageFromSlices is called when the diff image should be generated (for step 1)
     */
    void updateDiffImageFromSlices();
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
