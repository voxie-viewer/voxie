#include "buttonlabel.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEngine>

#include <QtWidgets/QLabel>

using namespace voxie::gui;

ButtonLabel::ButtonLabel(QWidget *parent) :
	QLabel(parent)
{
	int size = 24;
	this->setMinimumSize(size, size);
	this->setMaximumSize(size, size);
	this->setAlignment(Qt::AlignCenter);
}

void ButtonLabel::mousePressEvent(QMouseEvent *event)
{
	if(event->button() == Qt::LeftButton)
	{
		emit this->clicked();
	}
}

void ButtonLabel::enterEvent(QEvent*)
{
	this->setStyleSheet("QLabel { background-color: silver; }");
}

void ButtonLabel::leaveEvent(QEvent*)
{
    //this->setStyleSheet("QLabel { }");
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
