#pragma once

#include <QtCore/QStringList>

#include <QtWidgets/QLineEdit>

namespace voxie
{
namespace gui
{

class ScriptLineEdit : public QLineEdit
{
	Q_OBJECT
	QStringList log;
	int logOffset;
public:
	explicit ScriptLineEdit(QWidget *parent = 0);
	~ScriptLineEdit();

	virtual void keyPressEvent(QKeyEvent *) override;

signals:

public slots:
};


}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
