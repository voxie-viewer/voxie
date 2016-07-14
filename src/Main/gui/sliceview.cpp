#include "sliceview.hpp"

#include <Main/root.hpp>

#include <Main/gui/planeview.hpp>

#include <Voxie/io/sliceexporter.hpp>

#include <Voxie/plugin/voxieplugin.hpp>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

#include <QtOpenGL/QGLWidget>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

using namespace voxie;
using namespace voxie::gui;
using namespace voxie::data;
using namespace voxie::plugin;
using namespace voxie::io;

SliceView::SliceView(Slice *slice, QWidget *parent) :
    QWidget(parent),
    slice(slice)
{
    connect(this->slice, &Slice::planeChanged, this, &SliceView::sliceChanged);
    connect(slice, &DataObject::displayNameChanged, this, [=]()
        {this->setWindowTitle(this->slice->getDataset()->displayName() + " - " + this->slice->displayName());});
    connect(slice->getDataset(), &DataObject::displayNameChanged, this, [=]()
        {this->setWindowTitle(this->slice->getDataset()->displayName() + " - " + this->slice->displayName());});
    this->setWindowTitle(this->slice->getDataset()->displayName() + " - " + this->slice->displayName());

    QMetaObject::Connection conni = connect(this->slice, &QObject::destroyed, [this]() -> void
    {
        this->slice = nullptr;
        this->deleteLater();
    });
    connect(this, &QObject::destroyed, [=]() -> void
    {
        this->disconnect(conni);
    });

    QVBoxLayout *rootLayout = new QVBoxLayout();
    {
        QFormLayout *form = new QFormLayout();
        {
            QToolBar *toolbar = new QToolBar();

            QToolButton* sliceExport = new QToolButton();
            sliceExport->setIcon(QIcon(":/icons/disk.png"));
            sliceExport->setText("Export Slice");
            sliceExport->setPopupMode(QToolButton::InstantPopup);

            QMenu* contextMenu = new QMenu(this);

            for(VoxiePlugin* plugin : ::voxie::Root::instance()->plugins()) {
                if(plugin->sliceExporters().size() == 0) {
                    continue;
                }
                QMenu *pluginAction = contextMenu->addMenu(plugin->name());
                for(SliceExporter *exporter : plugin->sliceExporters()) {
                    QAction* action = pluginAction->addAction(exporter->objectName());
                    connect(action, &QAction::triggered, [=]() -> void
                    {
                        exporter->exportGui(this->slice);
                    });
                }
            }
            sliceExport->setMenu(contextMenu);
            toolbar->addWidget(sliceExport);
            form->addWidget(toolbar);

            this->positionEdit = new voxie::visualization::QVecLineEdit();
            this->positionEdit->setVector(this->slice->origin());//setText(toString(this->slice->origin()));
            connect(this->positionEdit, &QLineEdit::returnPressed, this, &SliceView::positionEdited);
            form->addRow("Position", this->positionEdit);

            this->rotationEdit = new voxie::visualization::QVecLineEdit();
            this->rotationEdit->setQuaternion(this->slice->rotation());//setText(toString(this->slice->rotation()));
            connect(this->rotationEdit, &QLineEdit::returnPressed, this, &SliceView::rotationEdited);
            form->addRow("Rotation", this->rotationEdit);
        }
        rootLayout->addLayout(form);

        QHBoxLayout *glView = new QHBoxLayout();
        {
            this->planeView = new PlaneView(this->slice);

            this->planeView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            glView->addWidget(this->planeView);
        }
        rootLayout->addLayout(glView);
    }
    this->setLayout(rootLayout);
}

SliceView::~SliceView()
{
    if(this->slice != nullptr)
        this->slice->deleteLater();
}

void SliceView::sliceChanged()
{
    this->positionEdit->setVector(this->slice->origin());//->setText(toString(this->slice->origin()));
    this->rotationEdit->setQuaternion(this->slice->rotation());//setText(toString(this->slice->rotation()));
    this->planeView->update();
}

void SliceView::positionEdited()
{
//    if(::parseVector.exactMatch(this->positionEdit->text()) == false)
//    {
//        this->positionEdit->setText(::toString(this->slice->origin()));
//        return;
//    }

//    qreal x, y, z;
//    x = ::parseVector.cap(1).toFloat();
//    y = ::parseVector.cap(2).toFloat();
//    z = ::parseVector.cap(3).toFloat();
    bool valid = false;
    QVector3D orig = this->positionEdit->getVector(&valid);
    if(valid){
        this->slice->setOrigin(orig);
        this->planeView->update();
    } else {
        this->positionEdit->setVector(this->slice->origin());
    }
}

void SliceView::rotationEdited()
{
//    if(::parseQuaternion.exactMatch(this->rotationEdit->text()) == false)
//    {
//        this->rotationEdit->setText(::toString(this->slice->origin()));
//        return;
//    }

//    qreal x, y, z, w;
//    w = ::parseQuaternion.cap(1).toFloat();
//    x = ::parseQuaternion.cap(2).toFloat();
//    y = ::parseQuaternion.cap(3).toFloat();
//    z = ::parseQuaternion.cap(4).toFloat();
    bool valid = false;
    QQuaternion quat = this->rotationEdit->getQuaternion(&valid);
    if(valid){
        this->slice->setRotation(quat.normalized());
        this->planeView->update();
    } else {
        this->rotationEdit->setQuaternion(this->slice->rotation());
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
