#include "imagepaintwidget.hpp"

#include <PluginVisSlice/slicevisualizer.hpp>

#include <QtCore/QDebug>
#include <QtCore/QList>

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include <QtWidgets/QProgressBar>

ImagePaintWidget::ImagePaintWidget(SliceVisualizer* sv, QWidget* parent)
        : QLabel(parent),
          sv(sv)
{
    QImage image(":/icons/transparency_.png");
    b.setTextureImage(image);
    b.setStyle(Qt::TexturePattern);
    this->setFocusPolicy( Qt::WheelFocus );
    this->setMouseTracking(true);
	this->setFocus();
}

ImagePaintWidget::~ImagePaintWidget() {

}

void ImagePaintWidget::wheelEvent(QWheelEvent * e) {
    sv->currentTool()->toolWheelEvent(e);
}

void ImagePaintWidget::mousePressEvent(QMouseEvent * e) {
	//qDebug() << "click";
    sv->currentTool()->toolMousePressEvent(e);
}

void ImagePaintWidget::mouseReleaseEvent(QMouseEvent * e) {
    sv->currentTool()->toolMouseReleaseEvent(e);
}

void ImagePaintWidget::keyPressEvent(QKeyEvent * e) {
	// quick tool switching with 1 ~ 9 & 0
	int tool;
    if(e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9) { // might want to exclude modifiers here
        tool = (e->key() - 0x30 - 1) % 10;
		if(tool >= 0 && tool < sv->tools().size()) {
			sv->switchToolTo(sv->tools().at(tool));
		}
	} else {
		sv->currentTool()->toolKeyPressEvent(e);
	}
}

void ImagePaintWidget::keyReleaseEvent(QKeyEvent * e) {
    sv->currentTool()->toolKeyReleaseEvent(e);
}

void ImagePaintWidget::mouseMoveEvent(QMouseEvent *e) {
    sv->currentTool()->toolMouseMoveEvent(e);
}

void ImagePaintWidget::paintEvent(QPaintEvent * pe) {
	Q_UNUSED(pe);
    QPainter painter(this);
    painter.setBrush(b);
    painter.drawRect(0,0,this->size().width(), this->size().height());
    QMap<int, QImage> map = sv->_drawStack;
    int i;
    for (i = -1; i < sv->_tools.size(); ++i) {
        if(map.contains(i)) {
            QImage im = map.value(i);
            if(im.width() > 0 && im.height() > 0) {
               // qDebug() << "drawing stack at " << i << " of range -1 to " << sv->tools.size()-1;
                painter.drawImage(0, 0, im);
            }/* else {
                qDebug() << "stack at " << i << " is null";
            }*/
        }
    }
    //this->setPixmap(pixmap);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
