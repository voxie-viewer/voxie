#pragma once

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/visualization/filterchain3dwidget.hpp>

#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

namespace voxie
{
namespace gui
{


class DataSetView : public QWidget
{
	Q_OBJECT
private:
	int sequenceNumber;
	voxie::data::DataSet *dataSet;
    voxie::visualization::FilterChain3DWidget* chainWidget;
    void applyFilters();
public:
	explicit DataSetView(voxie::data::DataSet *dataSet, QWidget *parent = 0);
	~DataSetView();

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
