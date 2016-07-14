#include "slicevisualizer.hpp"

#include <PluginVisSlice/imagepaintwidget.hpp>
#include <PluginVisSlice/toolexport.hpp>
#include <PluginVisSlice/toolselection.hpp>
#include <PluginVisSlice/toolsliceadjustment.hpp>
#include <PluginVisSlice/toolvalueviewer.hpp>
#include <PluginVisSlice/toolzoom.hpp>

#include <Voxie/ivoxie.hpp>

#include <Voxie/visualization/filterchain2dwidget.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QMetaType>
#include <QtCore/QThreadPool>

#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QRgb>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>

using namespace voxie::visualization;
using namespace voxie::data;

SliceVisualizer::SliceVisualizer(QVector<Slice*> slices, QWidget *parent) :
	SliceDataVisualizer(parent),
	_slice(slices.at(0))
{
	qRegisterMetaType<voxie::data::FloatImage>();
	qRegisterMetaType<voxie::data::SliceImage>();
	qRegisterMetaType<voxie::data::Plane>();
	qRegisterMetaType<QVector<int>>();

    resetPlaneArea();

    this->view = new QWidget();

	this->view->setWindowTitle("SliceVisualizer - " + this->slice()->displayName());
	this->setDisplayName("SliceVisualizer - " + this->slice()->displayName());

	this->view->setMinimumSize(300,200);

	this->_imageDisplayingWidget = new ImagePaintWidget(this); // has dependencies in tools
    this->view->setFocusProxy(this->_imageDisplayingWidget); // <- receives all keyboard events
	QSizePolicy pol(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	pol.setHorizontalStretch(0);
	pol.setVerticalStretch(0);
	this->_imageDisplayingWidget->setSizePolicy(pol);

	this->_tools.append(new SliceAdjustmentTool(view,this));
	this->_tools.append(new ToolZoom(view, this));
	this->_tools.append(new ValueViewerTool(view, this));
	this->_tools.append(this->selectionTool = new ToolSelection(view,this));
	this->_tools.append(new ToolExport(view, this));

	if(_tools.size() <= 0) {
		qDebug() << "NO TOOL FOUND IN SLICEVISUALIZER, WE DIE NOW";
	} else {
		toolBar = new QWidget(view);
		QHBoxLayout* l = new QHBoxLayout();
		toolBar->setLayout(l);
		l->setAlignment(Qt::AlignLeft);
		l->setContentsMargins(0,0,0,0);
		l->setSpacing(0);
		l->setMargin(0);

		Visualizer2DTool **data = _tools.data();
		for (int i = 0; i < _tools.size(); ++i) {
			this->toolBar->layout()->addWidget(data[i]);
			connect(this, &SliceVisualizer::sliceImageChanged, data[i], &Visualizer2DTool::sliceImageChanged);
		}
	}

	//*********FilterChain2D***********
	_filterChain2DWidget = new FilterChain2DWidget(view);
	QString name = _filterChain2DWidget->windowTitle();
	name.append(" - ");
    name.append(this->slice()->displayName());
    _filterChain2DWidget->setWindowTitle(name);

	this->dynamicSections().append(this->_filterChain2DWidget);
	connect(this->_filterChain2DWidget->getFilterChain(), &FilterChain2D::allFiltersApplied,[=](){
		this->onSliceImageFiltered(this->_filterChain2DWidget->getFilterChain()->getOutputSlice());
	});
	connect(this->_filterChain2DWidget->getFilterChain(), &FilterChain2D::allFiltersApplied,[=](){
		this->_filterWorkerRunning = false;
		runFilterWorker();
	});
	connect(this->_filterChain2DWidget->getFilterChain(), &FilterChain2D::filterListChanged, this, &SliceVisualizer::applyFilters);
	connect(this->_filterChain2DWidget->getFilterChain(), &FilterChain2D::filterChanged, this, &SliceVisualizer::applyFilters);
	connect(this->_filterChain2DWidget, &FilterChain2DWidget::requestFilterMaskEditor, this, &SliceVisualizer::onFilterMaskRequest);
	connect(this->_slice, &Slice::planeChanged, this->_filterChain2DWidget->getFilterChain(), &FilterChain2D::onPlaneChanged, Qt::DirectConnection);

	//*** COLORIZER ****
	this->_colorizerWidget = new SliceImageColorizerWidget(view);
	name = this->_colorizerWidget->windowTitle();
	name.append(" - ");
	name.append(this->slice()->displayName());
	_colorizerWidget->setWindowTitle(name);
	this->dynamicSections().append(_colorizerWidget);

	connect(_colorizerWidget, &SliceImageColorizerWidget::imageColorized, this, &SliceVisualizer::onImageColorized);
	connect(this, &SliceVisualizer::signalRequestColorization, _colorizerWidget, &SliceImageColorizerWidget::doColorizer);
	connect(_colorizerWidget, &SliceImageColorizerWidget::gradientChanged, this, [=](QMap<float, QRgb> mapping) {
		Q_UNUSED(mapping);
		emit signalRequestColorization(_filteredSliceImage);
	});


	//**** HISTOGRAMWIDGET ***
	_histogramWidget = new HistogramWidget(view);
	name = this->_histogramWidget->windowTitle();
	name.append(" - ");
    name.append(this->slice()->displayName());
    this->_histogramWidget->setWindowTitle(name);
	this->dynamicSections().append(this->_histogramWidget);

	connect(this, &SliceVisualizer::signalRequestHistogram, _histogramWidget, &HistogramWidget::calculateHistogram);
	connect(_histogramWidget, &HistogramWidget::histogramSettingsChanged, this, [=]() {
		emit signalRequestHistogram(_filteredSliceImage);
	});

	//*****   ****

	hobox = new QVBoxLayout(view);
	hobox->setSpacing(0);
	this->view->setLayout(hobox);

	hobox->addWidget(_imageDisplayingWidget);
	hobox->addWidget(toolBar);
	connect(this->_imageDisplayingWidget, &ImagePaintWidget::resized, this, &SliceVisualizer::onCanvasResized);

	_resizeTimer.setSingleShot(true);
	connect(&this->_resizeTimer, &QTimer::timeout, this, &SliceVisualizer::resized);
	connect(this, &SliceVisualizer::resized, this, &SliceVisualizer::signalRequestSliceImageUpdate);
	connect(this, &SliceVisualizer::signalRequestSliceImageUpdate, this, &SliceVisualizer::updateSliceImageFromSlice);
	//Slice update
	connect(this->_slice, &Slice::planeChanged, this, &SliceVisualizer::onSliceChanged);
	connect(this->_slice->getDataset(), &DataSet::changed, this, &SliceVisualizer::onDatasetChanged);

	//imageDisplayingWidget->setStyleSheet("QWidget {background: red}");
	//toolBar->setStyleSheet("QWidget { border-top: 1px }");
	Visualizer2DTool **data = _tools.data();
	for (int i = 0; i < _tools.size(); ++i){
		//!!!!connect(this, &SliceVisualizer::signalRequestColorization, data[i], &Visualizer2DTool::sliceImageChanged);
		connect(slices.at(0), &Slice::planeChanged, data[i], &Visualizer2DTool::sliceChanged);
		//connect(this, &SliceVisualizer::signalRequestSliceImageUpdate, data[i], &Visualizer2DTool::requestedImageUpdate);
		connect(this, &SliceVisualizer::resized, data[i], &Visualizer2DTool::onResize);
	}
	if(_tools.size() > 0){
		this->switchToolTo(this->currentTool());
	}

	this->view->show();

	_imageDisplayingWidget->setFocus();
	_imageDisplayingWidget->show();
	toolBar->show();
}

void SliceVisualizer::switchToolTo(Visualizer2DTool* tool) {
	if(tool == this->currentTool()){
	//	return;
	}

	Visualizer2DTool** data = _tools.data();
	int i;
	for (i = 0; i < _tools.size(); ++i) {
		if(data[i] == tool) {
			break;
		}
	}
	if(i == _tools.size()) {
		return;
	}
	data[_currentTool]->deactivateTool();
	this->_currentTool = i;
	data[_currentTool]->activateTool();
	this->_imageDisplayingWidget->setFocus();
}

void SliceVisualizer::updateSliceImageFromSlice() {
	int width = this->canvasWidth();
	int height = this->canvasHeight();
    //qDebug() << QString::number(width) + "|" + QString::number(height);
	if(currentPlaneArea().width() == 0 || currentPlaneArea().height() == 0){
		resetPlaneArea();
	}
	if(width > 0 && height > 0 && currentPlaneArea().height() > 0 && currentPlaneArea().width() > 0 ) {
		doGenerateSliceImage(this->_slice, currentPlaneArea(), QSize(width, height));
	}
}

void SliceVisualizer::doGenerateSliceImage(
		voxie::data::Slice* slice,
		 const QRectF &sliceArea,
		 const QSize &imageSize,
		 voxie::data::InterpolationMethod interpolation)
{
	if(_imageQueueWorker != nullptr) {
		delete _imageQueueWorker;
	}
	_imageQueueWorker = new ImageGeneratorWorker(slice, sliceArea, imageSize, interpolation);
	if(!_imageWorkerRunning) {
		runSliceImageGeneratorWorker();
	}
}

void SliceVisualizer::runSliceImageGeneratorWorker() {
	if(!_imageQueueWorker || _imageWorkerRunning) {
		return;
	}
	ImageGeneratorWorker* worker = _imageQueueWorker;
	_imageQueueWorker = nullptr;
	_imageWorkerRunning = true;
	connect(worker, &ImageGeneratorWorker::imageGenerated, this, &SliceVisualizer::onSliceImageGenerated);
	worker->setAutoDelete(true);
	connect(worker, &ImageGeneratorWorker::imageGenerated, [=]() -> void {
		_imageWorkerRunning = false;
		runSliceImageGeneratorWorker(); // run next item in queue if existant
	});
	QThreadPool::globalInstance()->start(worker);
}

void SliceVisualizer::onCanvasResized(QResizeEvent* event) {
    Q_UNUSED(event);
	//qDebug() << this->_currentPlaneArea << event->oldSize();
	//this->_currentPlaneArea = adjustedAreaAspect(this->_currentPlaneArea, event->oldSize(), event->size());
	//qDebug() << this->_currentPlaneArea;
	this->_resizeTimer.stop();
	this->_resizeTimer.start(RESIZE_TIMER_TIMEOUT);
}

void SliceVisualizer::updateBaseImage(QImage baseImage) {
	this->_displayedImage = baseImage;
	addToDrawStack(this, baseImage);
	redraw();
}

void SliceVisualizer::redraw() {
	_imageDisplayingWidget->update();
}

void SliceVisualizer::applyFilters()
{
	this->_filterQueueImage = this->_sliceImage.clone();
	this->_filterImageWorked = false;
	// apply to clone so sliceImage will not be modified
	if(!_filterWorkerRunning) {
		runFilterWorker();
	}
}

void SliceVisualizer::runFilterWorker()
{
	if(_filterWorkerRunning || _filterImageWorked) {
		return;
	}
	_filterWorkerRunning = true;
	this->_filterImageWorked = true;
	this->_filterChain2DWidget->applyFilter(this->_filterQueueImage);
}

void SliceVisualizer::onSliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent)
{
    //qDebug() << "slice change";
	if(equivalent && oldPlane.origin != newPlane.origin){
		QPointF difference = newPlane.get2DPlanePoint(oldPlane.origin);
		//this->_currentPlaneArea.translate(difference.x(), difference.y());
		center_ += QPointF(difference.x(), difference.y()); // TODO: ?
	}

	emit signalRequestSliceImageUpdate();
}

void SliceVisualizer::onDatasetChanged()
{
    //qDebug() << "dataset change" << this->slice()->getDataset();
	emit signalRequestSliceImageUpdate();
}


void SliceVisualizer::onSliceImageGenerated(SliceImage image)
{
    //qDebug() << "sliceimg generated";
	this->_sliceImage = image;
	this->applyFilters();
}

void SliceVisualizer::onSliceImageFiltered(SliceImage image)
{
    //qDebug() << "sliceimg filtered";
	this->_filteredSliceImage = image;
	emit this->signalRequestColorization(this->_filteredSliceImage);
	emit this->signalRequestHistogram(this->_filteredSliceImage);
	emit this->sliceImageChanged(this->_filteredSliceImage);
}

void SliceVisualizer::onImageColorized(QImage image)
{
    //qDebug() << "img colorized";
	updateBaseImage(image);
}


void SliceVisualizer::onFilterMaskRequest(Selection2DMask *mask)
{
    //qDebug() << "trigger selectionmask-tool" << mask;
	this->selectionTool->setMask(mask);
	this->switchToolTo(this->selectionTool);
}

QSize SliceVisualizer::canvasSize()
{
	return this->_imageDisplayingWidget->size();
}

QRectF
SliceVisualizer::adjustedAreaAspect(const QRectF &area, const QSize &oldSize, const QSize& newSize)
{
	QRectF newArea;
	if(oldSize.width() < 1 || oldSize.height() < 1){
		if(newSize.width() < 1 || newSize.height() < 1){
			return area;
		} else {
			newArea = area;
		}
	} else {
		qreal deltaW = (newSize.width() - oldSize.width())/(oldSize.width()*1.0);
		qreal deltaH = (newSize.height() - oldSize.height())/(oldSize.height()*1.0);
		//qDebug() << "deltas" << deltaW << deltaH;
		deltaW *= area.width();
		deltaH *= area.height();
		//qDebug() << "deltas" << deltaW << deltaH;

		newArea = QRectF(area.left(),area.top(),area.width()+deltaW, area.height()+deltaH);
		//qreal areaAspect = newArea.width()/newArea.height();
		//qreal imageAspect = (newSize.width()*1.0)/newSize.height();
		//qDebug() << areaAspect << imageAspect << (areaAspect - imageAspect) << newArea;
	}
	// adjust aspect
	qreal areaAspect = newArea.width()/newArea.height();
	qreal imageAspect = (newSize.width()*1.0)/newSize.height();
	qreal imageAspectInv = (newSize.height()*1.0)/newSize.width();

	if(fabs(areaAspect-imageAspect) > 1e-5){ // same as areaAspect != imageAspect
		//qDebug() << "aspect not fitting!!";
		if(imageAspect > areaAspect){
			// image wider than area, lets add to area's width
			qreal width = newArea.height() * imageAspect;
			// let area grow left and right, so adjust x position
			qreal x = newArea.x();// - ((width - newArea.width()) / 2);

			return QRectF(x, newArea.y(), width, newArea.height());
		} else {
			// image taller than area, lets add to area's height
			qreal height = newArea.width() *imageAspectInv;// / imageAspect;
			// let area grow top and bottom, so adjust y position
			qreal y = newArea.y() ;//- ((height - newArea.height()) / 2);

			return QRectF(newArea.x(), y, newArea.width(), height);
		}
	} else {
		return newArea;
	}
}

QRectF
SliceVisualizer::zoomedArea(const QRectF &area, qreal zoom)
{
	if(zoom == 0){
		return area;
	}
	qreal width = area.width()*zoom;
	qreal height = area.height()*zoom;
	// zoom to center
	qreal x = area.x() - ((width - area.width()) / 2);
	qreal y = area.y()- ((height - area.height()) / 2);
	return QRectF(x,y,width,height);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
