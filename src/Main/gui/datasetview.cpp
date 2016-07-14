#include "datasetview.hpp"

#include <Main/root.hpp>

#include <Voxie/data/slice.hpp>

#include <Voxie/io/voxelexporter.hpp>

#include <Voxie/plugin/voxieplugin.hpp>

#include <QtWidgets/QAction>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

using namespace voxie::gui;
using namespace voxie::data;
using namespace voxie::plugin;
using namespace voxie::io;
using namespace voxie::visualization;
using namespace voxie::filter;

DataSetView::DataSetView(voxie::data::DataSet *dataSet, QWidget *parent) :
	QWidget(parent),
	sequenceNumber(0),
	dataSet(dataSet)
{
	this->setMaximumHeight(400);
	QVBoxLayout *splitLayout = new QVBoxLayout();
	{
		QHBoxLayout *subContainer = new QHBoxLayout();
		{
			QToolBar *toolbar = new QToolBar();

			QAction *createSlice = toolbar->addAction(QIcon(":/icons/layer--plus.png"),"Create Slice");
            connect(createSlice, &QAction::triggered, dataSet, [=](){dataSet->createSlice();});

			QToolButton* voxelExport = new QToolButton();
			voxelExport->setIcon(QIcon(":/icons/disk.png"));
			voxelExport->setText("Export Dataset");
			voxelExport->setPopupMode(QToolButton::InstantPopup);

			QMenu* contextMenu = new QMenu(this);

			for(VoxiePlugin* plugin : ::voxie::Root::instance()->plugins()) {
				if(plugin->voxelExporters().size() == 0) {
					continue;
				}
				QMenu *pluginAction = contextMenu->addMenu(plugin->name());
				for(VoxelExporter *exporter : plugin->voxelExporters()) {
					QAction* action = pluginAction->addAction(exporter->objectName());
					connect(action, &QAction::triggered, [=]() -> void
					{
						exporter->exportGui(this->dataSet);
					});
				}
			}

			voxelExport->setMenu(contextMenu);

			toolbar->addWidget(voxelExport);

			subContainer->addWidget(toolbar);
		}
		splitLayout->addLayout(subContainer);
	}
	{
		QFormLayout *form = new QFormLayout();

		form->addRow("Dimension", new QLabel(
                        QString::number(this->dataSet->originalData()->getDimensions().x) + " x " +
						QString::number(this->dataSet->originalData()->getDimensions().y) + " x " +
						QString::number(this->dataSet->originalData()->getDimensions().z)));
		form->addRow("Size", new QLabel(
						QString::number(this->dataSet->size().x()) + " x " +
						QString::number(this->dataSet->size().y()) + " x " +
						QString::number(this->dataSet->size().z())));
		form->addRow("Spacing", new QLabel(
						QString::number(this->dataSet->originalData()->getSpacing().x()) + " x " +
						QString::number(this->dataSet->originalData()->getSpacing().y()) + " x " +
						QString::number(this->dataSet->originalData()->getSpacing().z())));
		form->addRow("Origin", new QLabel(
                        QString::number(this->dataSet->origin().x()) + " x " +
                        QString::number(this->dataSet->origin().y()) + " x " +
                        QString::number(this->dataSet->origin().z())));

		splitLayout->addLayout(form);
	}
	{
        /************ 3D Filter Chain **************/
        this->chainWidget = new FilterChain3DWidget();
        splitLayout->addWidget(this->chainWidget);
        connect(this->chainWidget->getFilterChain(), &FilterChain3D::filterListChanged, this, &DataSetView::applyFilters);
        connect(this->chainWidget->getFilterChain(), &FilterChain3D::filterChanged, this, &DataSetView::applyFilters);
        connect(this->dataSet->originalData().data(), &VoxelData::changed, this, &DataSetView::applyFilters);
	}
	this->setLayout(splitLayout);
    this->setWindowTitle(this->dataSet->displayName());

	QMetaObject::Connection conni = connect(this->dataSet, &QObject::destroyed, [this]() -> void
	{
		this->dataSet = nullptr;
		this->deleteLater();
	});
	connect(this, &QObject::destroyed, [=]() -> void
	{
		this->disconnect(conni);
	});
}

DataSetView::~DataSetView()
{
	if(this->dataSet != nullptr)
		this->dataSet->deleteLater();
}

void DataSetView::applyFilters() {
    this->chainWidget->applyFilter(this->dataSet);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
