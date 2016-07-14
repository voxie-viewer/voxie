#include "dataselectiondialog.hpp"

#include <Main/root.hpp>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QVBoxLayout>

using namespace voxie;
using namespace voxie::gui;
using namespace voxie::data;

DataSelectionDialog::DataSelectionDialog(
		Range sliceCount,
		Range dataSetCount,
		QWidget *parent) :
	QDialog(parent),
	sliceCount(sliceCount),
	dataSetCount(dataSetCount)
{
	QVBoxLayout *layout = new QVBoxLayout();

	QLabel *instruction = new QLabel();

	QString instructionText = "";

	bool singularSelection = false;
	if(this->dataSetCount.isNull() == false)
	{
		if(instructionText.length() > 0) instructionText += "\n";
		if(this->dataSetCount.isSingularity())
		{
			if(this->dataSetCount.min == 1)
			{
				instructionText += QString("Select a data set.");
				singularSelection ^= true;
			}
			else
			{
				instructionText += QString("Select %1 data sets.")
						.arg(this->dataSetCount.min);
			}
		}
		else
		{
			instructionText += QString("Select from %1 to %2 data sets.")
					.arg(this->dataSetCount.min)
					.arg(this->dataSetCount.max);
		}
	}
	if(this->sliceCount.isNull() == false)
	{
		if(instructionText.length() > 0) instructionText += "\n";
		if(this->sliceCount.isSingularity())
		{
			if(this->sliceCount.min == 1)
			{
				instructionText += QString("Select a slice.");
				singularSelection ^= true;
			}
			else
			{
				instructionText += QString("Select %1 slices.")
						.arg(this->sliceCount.min);
			}
		}
		else
		{
			instructionText += QString("Select from %1 to %2 slices.")
					.arg(this->sliceCount.min)
					.arg(this->sliceCount.max);
		}
	}
	instruction->setText(instructionText);

	layout->addWidget(instruction);

	this->treeView = new QTreeWidget();
	if(singularSelection)
	{
		this->treeView->setSelectionMode(QAbstractItemView::SingleSelection);
	}
	else
	{
		this->treeView->setSelectionMode(QAbstractItemView::MultiSelection);
	}
	this->treeView->setHeaderLabel("Data Set Overview");
	connect(this->treeView, &QTreeWidget::itemSelectionChanged, this, &DataSelectionDialog::itemSelectionChanged);
	for(DataSet *dataSet : ::voxie::Root::instance()->dataSets())
	{
		QTreeWidgetItem *item = new QTreeWidgetItem(this->treeView);
		item->setText(0, dataSet->displayName());
		item->setData(0, Qt::UserRole, QVariant::fromValue(dataSet));

		// Include slices only when necessary
		if(this->sliceCount.isNull() == false)
		{
			for(Slice *slice : dataSet->getSlices())
			{

				QTreeWidgetItem *sliceItem = new QTreeWidgetItem(item);
				sliceItem->setText(0, slice->displayName());
				sliceItem->setData(0, Qt::UserRole, QVariant::fromValue(slice));
			}
		}
	}
	this->treeView->expandAll();

	layout->addWidget(treeView);

	QHBoxLayout *buttons = new QHBoxLayout();

	buttons->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

	QPushButton *rejectButton = new QPushButton("Cancel");
	connect(rejectButton, &QPushButton::clicked, this, &QDialog::reject);
	buttons->addWidget(rejectButton);

	this->okButton = new QPushButton("Ok");
	this->okButton->setEnabled(false);
	connect(this->okButton, &QPushButton::clicked, this, &QDialog::accept);
	buttons->addWidget(this->okButton);

	layout->addLayout(buttons);

	this->setLayout(layout);
}

void DataSelectionDialog::itemSelectionChanged()
{
	this->sliceSelection.clear();
	this->dataSetSelection.clear();
	for(QTreeWidgetItem *item : this->treeView->selectedItems())
	{
		DataSet *voxelData = item->data(0, Qt::UserRole).value<DataSet*>();
		Slice *slice = item->data(0, Qt::UserRole).value<Slice*>();

		if(voxelData != nullptr) this->dataSetSelection.append(voxelData);
		if(slice != nullptr) this->sliceSelection.append(slice);
	}

	this->okButton->setEnabled(
				this->sliceCount.contains(this->sliceSelection.size()) &&
				this->dataSetCount.contains(this->dataSetSelection.size()));
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
