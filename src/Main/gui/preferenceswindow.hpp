#pragma once

#include <QtWidgets/QDialog>

namespace voxie
{
namespace gui
{

class PreferencesWindow :
		public QDialog
{
	Q_OBJECT
public:
	explicit PreferencesWindow(QWidget *parent = 0);
	~PreferencesWindow();

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
