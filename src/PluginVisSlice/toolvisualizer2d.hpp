#pragma once

#include <Voxie/data/slice.hpp>
#include <Voxie/data/sliceimage.hpp>

#include <QtCore/QObject>

#include <QtGui/QIcon>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include <QtWidgets/QWidget>

class SliceVisualizer;

/**
 * @brief The Visualizer2DTool class provides an interface for tools to be able to receive information about the user input on the displayed image.
 */

class Visualizer2DTool : public QWidget
{
    Q_OBJECT
public:
    Visualizer2DTool(QWidget *parent);
    ~Visualizer2DTool();
public:
    virtual QIcon getIcon() = 0;
    virtual QString getName() = 0;

public slots:
	/**
	 * @brief activateTool signals that this tool was switched in the slice visualizer.
	 */
    virtual void activateTool() = 0;
	/**
	 * @brief deactivateTool signals that this tool has been switched away from in the slice visualizer.
	 */
    virtual void deactivateTool() = 0;

	/**
	 * Used to forward events to the tool. Check with the callee as it might catching events already. (e.g. numbers for switching tools)
	 */
    virtual void toolMousePressEvent(QMouseEvent *) = 0;
	/**
	 * Used to forward events to the tool. Check with the callee as it might catching events already. (e.g. numbers for switching tools)
	 */
    virtual void toolMouseReleaseEvent(QMouseEvent *) = 0;
	/**
	 * Used to forward events to the tool. Check with the callee as it might catching events already. (e.g. numbers for switching tools)
	 */
    virtual void toolMouseMoveEvent(QMouseEvent *) = 0;
	/**
	 * Used to forward events to the tool. Check with the callee as it might catching events already. (e.g. numbers for switching tools)
	 */
    virtual void toolKeyPressEvent(QKeyEvent *) = 0;
	/**
	 * Used to forward events to the tool. Check with the callee as it might catching events already. (e.g. numbers for switching tools)
	 */
    virtual void toolKeyReleaseEvent(QKeyEvent *) = 0;
	/**
	 * Used to forward events to the tool. Check with the callee as it might catching events already. (e.g. numbers for switching tools)
	 */
    virtual void toolWheelEvent(QWheelEvent *) = 0;

	/**
	 * @brief sliceChanged signals that the slice associated with the associated slice visualizer has changed.
	 * @param oldPlane
	 * @param newPlane
	 * @param equivalent
	 */
    virtual void sliceChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent) = 0;
	/**
	 * @brief sliceImageChanged signals that the slice image associated with the associated slice visualizer has changed.
	 * @param si
	 */
    virtual void sliceImageChanged(voxie::data::SliceImage& si) = 0;
	/**
	 * @brief onResize signals that the canvas size the slice visualizer uses has been changed.
	 */
    virtual void onResize(){}
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
