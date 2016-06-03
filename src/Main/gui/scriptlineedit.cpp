#include "scriptlineedit.hpp"

#include <QtCore/QDebug>

#include <QtGui/QKeyEvent>

using namespace voxie::gui;

ScriptLineEdit::ScriptLineEdit(QWidget *parent) :
	QLineEdit(parent),
	log(), logOffset(-1)
{

}

ScriptLineEdit::~ScriptLineEdit()
{

}

void ScriptLineEdit::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Up)
	{
		if(this->logOffset < (this->log.length() - 1))
		{
			if(this->logOffset <= this->log.length())
				this->logOffset++;
			this->setText(this->log.at(this->logOffset));
		}
	}
	else if(event->key() == Qt::Key_Down)
	{
		if(this->logOffset > 0)
		{
			this->logOffset--;
			this->setText(this->log.at(this->logOffset));
		}
	}
	else if(event->key() == Qt::Key_Return)
	{
		if(this->text().length() > 0)
		{
			if((this->log.size() == 0) || (this->log.at(0) != this->text()))
				this->log.insert(0, this->text());
			this->logOffset = -1;
		}
		QLineEdit::keyPressEvent(event);
	}
	else
	{
		QLineEdit::keyPressEvent(event);
		this->logOffset = -1;
	}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
