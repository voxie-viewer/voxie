#pragma once

#include <QtGui/QResizeEvent>

#include <QtWidgets/QLabel>

class SliceVisualizer;

/**
 * Provides a widget that can draw a stack of qimages on top of each other and forward user input associated with the widget to tools.
 * @author Hans Martin Berner
 */

class ImagePaintWidget : public QLabel
{
    Q_OBJECT
public:
	/**
	 * @brief ImagePaintWidget is a widget that can paint a stack of qimages and forward its input events to slice visualizer tools.
	 * @param sv the corresponding slice visualizer
	 * @param parent
	 */
    ImagePaintWidget(SliceVisualizer* sv, QWidget* parent = 0);
    ~ImagePaintWidget();
	/**
	 * event forwarding methods
	 */
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void keyReleaseEvent(QKeyEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *ev){emit this->resized(ev);}
signals:
    void resized(QResizeEvent *ev);

protected:
    void paintEvent(QPaintEvent*) ;

private:
    QBrush b;
    SliceVisualizer* sv;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
