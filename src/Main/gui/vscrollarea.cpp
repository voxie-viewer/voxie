#include "vscrollarea.hpp"

#include <QtCore/QEvent>

#include <QtWidgets/QScrollBar>

using namespace voxie::gui;

VScrollArea::VScrollArea(QWidget *parent) :
	QScrollArea(parent)
{
	this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	this->setWidgetResizable(true);

}

bool VScrollArea::eventFilter(QObject *o, QEvent *e)
{
	// This works because QScrollArea::setWidget installs an eventFilter on the widget
	if(o && o == widget() && e->type() == QEvent::Resize)
	{
		setMinimumWidth(
					std::min(
					maximumWidth(),
					widget()->minimumSizeHint().width() + verticalScrollBar()->width()));
	}
	return QScrollArea::eventFilter(o, e);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
