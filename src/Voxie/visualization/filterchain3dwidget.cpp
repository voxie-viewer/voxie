#include "filterchain3dwidget.hpp"

#include <Voxie/ivoxie.hpp>

#include <Voxie/data/floatimage.hpp>
#include <Voxie/data/sliceimage.hpp>

#include <Voxie/filter/filter3d.hpp>

#include <Voxie/plugin/metafilter3d.hpp>
#include <Voxie/plugin/voxieplugin.hpp>

#include <QtCore/QDebug>
#include <QtCore/QVector>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>

using namespace voxie::filter;
using namespace voxie::data;
using namespace voxie::visualization;


FilterChain3DWidget::FilterChain3DWidget(QWidget *parent) :
    QWidget(parent),
    filterchain(new FilterChain3D(this))
{
    this->setWindowTitle("3D Filters");
    QVBoxLayout *topLayout = new QVBoxLayout(this);

    //******Toolbar that`s shown over list****

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QToolBar *toolbar = new QToolBar();

    QAction *addFilterButton = toolbar->addAction(QIcon(":/icons/slide--plus.png"),"Add Filter");
    QAction *removeFilterButton = toolbar->addAction(QIcon(":/icons/slide--minus.png"),"Remove Filter");
    QAction *moveFilterDownButton = toolbar->addAction(QIcon(":/icons/arrow-270.png"),"Filter down");
    QAction *moveFilterUpButton = toolbar->addAction(QIcon(":/icons/arrow-090.png"),"Filter up");
    QAction *filterSettingsButton = toolbar->addAction(QIcon(":/icons/gear.png"),"Filter Settings");
    QAction *filtermaskButton = toolbar->addAction(QIcon(":/icons/mask.png"),"Filter Mask");
    QAction *exportButton = toolbar->addAction(QIcon(":/icons/disk.png"),"Export Filterchain");
    QAction *importButton = toolbar->addAction(QIcon(":/icons/folder-horizontal-open.png"),"Import Filterchain");
    _directFilterApply = toolbar->addAction(QIcon(":/icons/paper-bag-recycle.png"),"Apply filters manually?");
    _directFilterApply->setCheckable(true);
    QAction *triggerFilters = toolbar->addAction(QIcon(":/icons/tick-button.png"), "Apply filters");
    triggerFilters->setEnabled(_directFilterApply->isChecked());
    connect(_directFilterApply, &QAction::triggered, [=]() {
        triggerFilters->setEnabled(_directFilterApply->isChecked());
        this->filterchain->enableSignalOnChange(!triggerFilters->isEnabled());
    });
    connect(triggerFilters, &QAction::triggered, this, &FilterChain3DWidget::triggerChainChangeSignal);
    connect(addFilterButton,&QAction::triggered,this, &FilterChain3DWidget::addFilter);
    connect(removeFilterButton, &QAction::triggered, this, &FilterChain3DWidget::removeFilter);
    connect(moveFilterDownButton, &QAction::triggered, this, &FilterChain3DWidget::moveFilterDown);
    connect(moveFilterUpButton, &QAction::triggered, this, &FilterChain3DWidget::moveFilterUp);
    connect(filterSettingsButton, &QAction::triggered, this, &FilterChain3DWidget::openSettingsDialog);
    connect(filtermaskButton, &QAction::triggered, this, &FilterChain3DWidget::openFiltermaskEditor);
    connect(exportButton, &QAction::triggered, this, &FilterChain3DWidget::exportFilterChain);
    connect(importButton, &QAction::triggered, this, &FilterChain3DWidget::importFilterChain);

    buttonLayout->addWidget(toolbar);

    //**********List of Filters**************

    QHBoxLayout *filterLayout = new QHBoxLayout();
    list = new QListWidget();
    filterLayout->addWidget(list);


    //*******put all together**********

    topLayout->addLayout(buttonLayout);
    topLayout->addLayout(filterLayout);

    connect(filterchain, &FilterChain3D::filterListChanged, this, &FilterChain3DWidget::updateList);
    connect(this->list, &QListWidget::itemClicked, this, &FilterChain3DWidget::checkEnabled);
    //******* Dialog is shown when addFilter is clicked ***********
    this->addDialog = new QDialog(this);
    addDialog->setWindowTitle("Add Filter");

    QVBoxLayout *topLayoutDialog = new QVBoxLayout(addDialog);

    //list
    //QHBoxLayout *layout = new QHBoxLayout();
    filterToAdd = new QListWidget();
    topLayoutDialog->addWidget(filterToAdd);
    //topLayoutDialog->addLayout(layout);
    //Ok Buttoon
    QPushButton *okBtn = new QPushButton("Ok");
    connect(okBtn, &QPushButton::clicked, [=](){
        addDialog->accept();
        for(voxie::plugin::VoxiePlugin* plugin : ::voxie::voxieRoot().plugins())
        {
            if(plugin->filters3D().size() == 0) {
                continue;
            }
            for(voxie::plugin::MetaFilter3D* metafilter : plugin->filters3D()){
                if(metafilter->objectName().compare(this->filterToAdd->currentItem()->text()) == 0)
                {
                    Filter3D* filter = metafilter->createFilter();
                    this->filterchain->addFilter(filter);
                    if(!this->filterchain->signalOnChangeEnabled())
                        this->updateList();
                }
            }
        }
    });
    topLayoutDialog->addWidget(okBtn);
}

void FilterChain3DWidget::addFilter()
{
    this->filterToAdd->clear(); //clears list in the add dialog otherwise therre are duplicate entries
    for(voxie::plugin::VoxiePlugin* plugin : ::voxie::voxieRoot().plugins())
    {
        if(plugin->filters3D().size() == 0) {
            continue;
        }
        for(voxie::plugin::MetaFilter3D* metafilter : plugin->filters3D()){
            this->filterToAdd->addItem(metafilter->objectName());
        }
    }
    this->filterToAdd->setCurrentRow(0);
    int dialogResult = addDialog->exec();
    if(dialogResult == QDialog::Accepted){
        //qDebug() << "add filter";
    }
}

void FilterChain3DWidget::removeFilter()
{

    int currentRow = list->currentRow();
    if(currentRow != -1)//a item must be selected
    {
        this->filterchain->removeFilter(this->filterchain->getFilter(currentRow));
        if(!this->filterchain->signalOnChangeEnabled())
            this->updateList();
    }

}
void FilterChain3DWidget::moveFilterUp()
{
    int currentRow = list->currentRow();

    if(currentRow != -1)//a item must be selected
    {
        if(currentRow == 0)
        {
            //pos cant be <=1 so do nothing

        }else
        {
            Filter3D *filter = filterchain->getFilter(currentRow);
            filterchain->changePosition(filter, currentRow-1);
            list->setCurrentRow(currentRow-1); // keep focus on moved item
            if(!this->filterchain->signalOnChangeEnabled())
                this->updateList();
        }
    }

}
void FilterChain3DWidget::moveFilterDown()
{

    int currentRow = list->currentRow();

    if(currentRow != -1)//a item must be selected
    {
        if(currentRow+1 == list->count())
        {
            //pos cant be >= items in list. so do nothing

        }else
        {
            Filter3D *filter = filterchain->getFilter(currentRow);
            filterchain->changePosition(filter, currentRow+1);
            list->setCurrentRow(currentRow+1); // keep focus on moved item
            if(!this->filterchain->signalOnChangeEnabled())
                this->updateList();
        }
    }

}

void FilterChain3DWidget::applyFilter(DataSet* dataSet)
{
    this->filterchain->applyTo(dataSet);
}

FilterChain3D *FilterChain3DWidget::getFilterChain()
{
    return this->filterchain;
}

void FilterChain3DWidget::updateList()
{
    this->list->clear();
    for(int i = 0; i < this->filterchain->getFilters().size(); i++)
    {
        Filter3D* filter = this->filterchain->getFilter(i);
        //QString name = filter->getName();
        QListWidgetItem* item = new QListWidgetItem();
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        if(filter->isEnabled())
        {
            item->setCheckState(Qt::Checked);
        }else{
            item->setCheckState(Qt::Unchecked);
        }
        item->setText(filter->getMetaName());
        this->list->addItem(item);
        this->list->setCurrentItem(item);
    }
}

void FilterChain3DWidget::triggerChainChangeSignal()
{
    emit this->filterchain->filterListChanged();
}

void FilterChain3DWidget::checkEnabled(QListWidgetItem * item)
{
    //int currentRow = this->list->currentRow();
    int currentRow = this->list->row(item);
    if(item->checkState() == Qt::Unchecked)
    {
        this->filterchain->getFilter(currentRow)->setEnabled(false);
    }
    if(item->checkState() == Qt::Checked)
    {
        this->filterchain->getFilter(currentRow)->setEnabled(true);
    }
}

void FilterChain3DWidget::openSettingsDialog()
{
    int currentRow = this->list->currentRow();
    if(currentRow != -1)//a item must be selected
    {
        Filter3D *filter = filterchain->getFilter(currentRow);
        if(filter->hasSettingsDialog())
        {
            filter->getSettingsDialog()->exec();
        }
    }
}


void FilterChain3DWidget::openFiltermaskEditor()
{
    int currentRow = this->list->currentRow();
    if(currentRow != -1)//a item must be selected
    {
        Selection3DMask *mask = filterchain->getFilter(currentRow)->getMask();
        if(mask != nullptr)
        {
            Filter3DMaskEditor* maskEditor = new Filter3DMaskEditor();
            maskEditor->setMask(mask);
            QVBoxLayout* layout = new QVBoxLayout();
            layout->addWidget(maskEditor);
            QDialog dialog;
            dialog.setWindowTitle("3D Filter Mask");
            dialog.setLayout(layout);
            dialog.exec();
        }
    }
}

// --- Filter3DMaskEditor ---
Filter3DMaskEditor::Filter3DMaskEditor(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QHBoxLayout* toolBarLayout = new QHBoxLayout();
    this->shapeList = new QListWidget();
    // --
    QPushButton* btnAdd = new QPushButton("add");
    QPushButton* btnRemove = new QPushButton("remove");
    QPushButton* btnEdit = new QPushButton("edit");
    this->cmbShape = new QComboBox();
    toolBarLayout->addWidget(cmbShape);
    toolBarLayout->addWidget(btnAdd);
    toolBarLayout->addWidget(btnRemove);
    toolBarLayout->addWidget(btnEdit);
    // --
    for(Shape3D* shape: Shape3D::getShapeInstances()){
        cmbShape->addItem(shape->name(), QVariant::fromValue(shape->copy()) );
    }
    // --
    connect(btnAdd, &QPushButton::clicked, this, &Filter3DMaskEditor::addShape);
    connect(btnRemove, &QPushButton::clicked, this, &Filter3DMaskEditor::removeShape);
    connect(btnEdit, &QPushButton::clicked, this, &Filter3DMaskEditor::editShape);
    // --
    layout->addLayout(toolBarLayout);
    layout->addWidget(shapeList);
}


void
Filter3DMaskEditor::setMask(voxie::filter::Selection3DMask* mask)
{
    Q_ASSERT(mask != nullptr);
    if(mask != this->mask){
        if(this->mask != nullptr)
            disconnect(this->mask, &voxie::filter::Selection3DMask::changed, this, &Filter3DMaskEditor::onMaskChange);
        connect(mask, &voxie::filter::Selection3DMask::changed, this, &Filter3DMaskEditor::onMaskChange);
    }
    this->mask = mask;
    setupList();
}

QListWidgetItem* itemFromShape(Shape3D* shape, QListWidget* list)
{
    QListWidgetItem* item = new QListWidgetItem(shape->toString(), list);
    item->setData(42, QVariant::fromValue(shape));
    return item;
}

bool openEditDialog(Shape3D* shape){
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(shape->editorWidget());
    QPushButton* accept = new QPushButton("ok");
    layout->addWidget(accept);
    QDialog dialog;
    QObject::connect(accept, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.setLayout(layout);
    return dialog.exec() == QDialog::Accepted;
}

void Filter3DMaskEditor::setupList()
{
    this->shapeList->clear();
    if(this->mask == nullptr)
        return;
    for(Shape3D* shape: this->mask->allShapes()){
        itemFromShape(shape, this->shapeList);
    }
}

void
Filter3DMaskEditor::addShape()
{
    QVariant var = this->cmbShape->currentData();
    Shape3D* shape = var.value<Shape3D*>();
    shape = shape->copy();
    if(openEditDialog(shape)){
        this->mask->addShape(shape);
    } else {
        delete shape;
    }
}

void
Filter3DMaskEditor::removeShape()
{
    int row = this->shapeList->currentRow();
    if(row >= 0){
        QListWidgetItem* item = this->shapeList->takeItem(row);
        this->mask->removeShape(item->data(42).value<Shape3D*>());
        delete item;
    }
}

void
Filter3DMaskEditor::editShape()
{
    int row = this->shapeList->currentRow();
    if(row >= 0){
        QListWidgetItem* item = this->shapeList->item(row);
        openEditDialog(item->data(42).value<Shape3D*>());
    }
}


void
Filter3DMaskEditor::onMaskChange()
{
    setupList();
}

void FilterChain3DWidget::exportFilterChain()
{
    QString fileName = QFileDialog::getSaveFileName(NULL,
            tr("Export Filterchain"),
            QDir::currentPath(),
            "XML file (*.xml)" );
    if (fileName.isNull()) {
        return;
    }
    this->filterchain->toXML(fileName);
}

void FilterChain3DWidget::importFilterChain()
{
    QString fileName = QFileDialog::getOpenFileName(NULL,
            tr("Import Filterchain"),
            QDir::currentPath(),
            "XML file (*.xml)");
    if (fileName.isNull()) {
        return;
    }
    this->filterchain->fromXML(fileName);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
