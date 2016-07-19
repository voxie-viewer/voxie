#include "sidepanel.hpp"

#include <Main/root.hpp>

#include <Main/gui/objecttree.hpp>
#include <Main/gui/buttonlabel.hpp>

#include <Voxie/io/operation.hpp>

#include <Voxie/plugin/voxieplugin.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QPointer>

#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>

using namespace voxie::data;
using namespace voxie::gui;
using namespace voxie::io;

SidePanel::SidePanel(voxie::Root* root, QMainWindow* mainWindow, QWidget *parent) : QWidget(parent) {
    this->setMinimumWidth(350);
    this->setMaximumWidth(800);

    auto layout = new QVBoxLayout();
    layout->setMargin(0);
    this->setLayout(layout);

    auto splitter = new QSplitter(Qt::Vertical);
    layout->addWidget(splitter);

    splitter->addWidget(objectTree = new ObjectTree(root));
    splitter->setCollapsible(0, false);
    objectTree->setMinimumHeight(100);
    //objectTree->setMaximumHeight(500);

    auto bottomWidget = new QWidget();
    splitter->addWidget(bottomWidget);
    splitter->setCollapsible(1, false);
    bottomLayout = new QVBoxLayout();
    bottomLayout->setMargin(0);
    bottomWidget->setLayout(bottomLayout);

    //auto scroll = new VScrollArea();
    auto scroll = new QScrollArea();
    bottomLayout->addWidget(scroll);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidgetResizable(true);
    {
        QWidget *boxC = new QWidget();
        QVBoxLayout *box = new QVBoxLayout(boxC);
        box->setMargin(0);
        box->setSpacing(2);
        {
            this->sections = new QVBoxLayout();
            this->sections->setMargin(3);
            //this->sections->addWidget(objectTree = new ObjectTree(root));
            box->addLayout(this->sections);
            box->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
        }
        scroll->setWidget(boxC);
    }

    splitter->setSizes({1000, 3000});

    connect(root, &voxie::Root::dataObjectAdded, this, &SidePanel::addDataObject);
    connect(objectTree, &ObjectTree::objectSelected, this, [this] (voxie::data::DataObject* obj) {
            //qDebug() << "Change sidepanel visibility to" << obj << visibleSections.size();

            for (auto section : visibleSections) {
                if (!section)
                    continue;
                if (section->property("dockWidget").value<QWidget*>())
                    section->property("dockWidget").value<QWidget*>()->setVisible(false);
            }
            visibleSections.clear();

            if (obj) {
                for (auto section : obj->propertySections()) {
                    visibleSections << section;
                    if (section->property("dockWidget").value<QWidget*>())
                        section->property("dockWidget").value<QWidget*>()->setVisible(true);
                }
            }
        });
    connect(objectTree, &ObjectTree::objectActivated, this, [this] (DataObject* obj) {
            auto vis = qobject_cast<voxie::visualization::Visualizer*>(obj);
            if (vis) {
                auto parent = qobject_cast<VisualizerContainer*>(vis->mainView()->parent());
                if (!parent)
                    qWarning() << "Visualizer main view does not have a VisualizerContainer parent" << vis->mainView();
                else
                    parent->activate();
            }
        });
    objectTree->setContextMenuPolicy(Qt::CustomContextMenu);
    auto currentObject = createQSharedPointer<QPointer<DataObject>>(nullptr);
    auto contextMenuDataset = new QMenu(mainWindow);
    connect (contextMenuDataset->addAction("Create slice"), &QAction::triggered, this, [this, currentObject] {
            auto dataSet = qobject_cast<DataSet*>(*currentObject);
            if (!dataSet) {
                qWarning() << "Failed to cast object to DataSet*";
                return;
            }
            dataSet->createSlice();
        });
    connect (contextMenuDataset->addAction("Create isosurface visualizer"), &QAction::triggered, this, [this, currentObject, mainWindow] {
            auto dataSet = qobject_cast<DataSet*>(*currentObject);
            if (!dataSet) {
                qWarning() << "Failed to cast object to DataSet*";
                return;
            }
            try {
                auto isoVisualizer = qobject_cast<voxie::plugin::MetaVisualizer*>(voxie::Root::instance()->getPluginByName("Voxie3D")->getMemberByName("de.uni_stuttgart.Voxie.VisualizerFactory", "IsosurfaceMetaVisualizer"));
                if (!isoVisualizer)
                    throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Failed to cast isosurface visualizer factory");
                QVector<data::DataSet*> dataSets;
                QVector<data::Slice*> slices;
                dataSets << dataSet;
                isoVisualizer->create(dataSets, slices);
            } catch (voxie::scripting::ScriptingException& e) {
                QMessageBox(QMessageBox::Critical, mainWindow->windowTitle(), QString("Failed to create isosurface visualizer: %1").arg(e.message()), QMessageBox::Ok, mainWindow).exec();
            }
        });
    contextMenuDataset->addSeparator();
    connect (contextMenuDataset->addAction("Close"), &QAction::triggered, this, [this, currentObject] {
            if (*currentObject)
                (*currentObject)->deleteLater();
        });
    auto contextMenuSlice = new QMenu(mainWindow);
    connect (contextMenuSlice->addAction("Create slice visualizer"), &QAction::triggered, this, [this, currentObject, mainWindow] {
            auto slice = qobject_cast<Slice*>(*currentObject);
            if (!slice) {
                qWarning() << "Failed to cast object to DataSet*";
                return;
            }
            try {
                auto isoVisualizer = qobject_cast<voxie::plugin::MetaVisualizer*>(voxie::Root::instance()->getPluginByName("SliceView")->getMemberByName("de.uni_stuttgart.Voxie.VisualizerFactory", "SliceMetaVisualizer"));
                if (!isoVisualizer)
                    throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.Error", "Failed to cast slice visualizer factory");
                QVector<data::DataSet*> dataSets;
                QVector<data::Slice*> slices;
                slices << slice;
                isoVisualizer->create(dataSets, slices);
            } catch (voxie::scripting::ScriptingException& e) {
                QMessageBox(QMessageBox::Critical, mainWindow->windowTitle(), QString("Failed to create slice visualizer: %1").arg(e.message()), QMessageBox::Ok, mainWindow).exec();
            }
        });
    contextMenuSlice->addSeparator();
    connect (contextMenuSlice->addAction("Close"), &QAction::triggered, this, [this, currentObject] {
            if (*currentObject)
                (*currentObject)->deleteLater();
        });
    auto contextMenuVisualizer = new QMenu(mainWindow);
    connect (contextMenuVisualizer->addAction("Raise"), &QAction::triggered, this, [this, currentObject] {
            auto visualizer = qobject_cast<voxie::visualization::Visualizer*>(*currentObject);
            if (!visualizer) {
                qWarning() << "Failed to cast object to DataSet*";
                return;
            }
            auto parent = qobject_cast<VisualizerContainer*>(visualizer->mainView()->parent());
            if (!parent)
                qWarning() << "Visualizer main view does not have a VisualizerContainer parent" << visualizer->mainView();
            else
                parent->activate();
        });
    contextMenuVisualizer->addSeparator();
    connect (contextMenuVisualizer->addAction("Close"), &QAction::triggered, this, [this, currentObject] {
            if (*currentObject)
                (*currentObject)->deleteLater();
        });
    connect(objectTree, &QWidget::customContextMenuRequested, this, [this, currentObject, contextMenuDataset, contextMenuSlice, contextMenuVisualizer] (const QPoint& pos) {
            auto obj = objectTree->getObjectForItem(objectTree->itemAt(pos));
            if (!obj)
                return;
            *currentObject = obj;
            auto globalPos = objectTree->viewport()->mapToGlobal(pos);
            if (qobject_cast<DataSet*>(obj))
                contextMenuDataset->popup(globalPos);
            if (qobject_cast<Slice*>(obj))
                contextMenuSlice->popup(globalPos);
            if (qobject_cast<voxie::visualization::Visualizer*>(obj))
                contextMenuVisualizer->popup(globalPos);
        });
}
SidePanel::~SidePanel() {
}

void SidePanel::addDataObject(voxie::data::DataObject* obj) {
    auto hasDataSection = createQSharedPointer<bool>(false);

    connect(obj, &DataObject::propertySectionAdded, this, [this, hasDataSection, obj] (QWidget* section) {
            addSection(section, !*hasDataSection, obj);
            *hasDataSection = true;
        });
    for (auto section : obj->propertySections()) {
        addSection(section, !*hasDataSection, obj);
        *hasDataSection = true;
    }
}

QWidget* SidePanel::addSection(QWidget *section, bool closeable, voxie::data::DataObject* obj) {
    if(section == nullptr)
    {
        return nullptr;
    }

    QString title = section->windowTitle();
    if(title.length() == 0)
    {
        title = section->objectName();
    }
    if(title.length() == 0)
    {
        title = section->metaObject()->className();
    }

    QWidget *dockWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    {
        QWidget *headerContainer = new QWidget();
        headerContainer->setStyleSheet("QWidget { background-color: gray; color: white }");
        {
            QHBoxLayout *headerBox = new QHBoxLayout();
            headerBox->setMargin(0);
            headerBox->setSpacing(0);
            {
                QSpacerItem *spacer = new QSpacerItem(
                    24 + (closeable ? 24 : 0),
                    24,
                    QSizePolicy::Minimum,
                    QSizePolicy::Minimum);
                headerBox->addSpacerItem(spacer);

                QLabel *header = new QLabel("<b>" + title + "</b>");
                connect(section, &QWidget::windowTitleChanged, header, [=]()
                    {header->setText("<b>" + section->windowTitle() + "</b>");});
                header->setAlignment(Qt::AlignCenter);
                headerBox->addWidget(header);

                if(closeable)
                {
                    ButtonLabel *closeButton = new ButtonLabel();
                    //closeButton->setText("X");
                    closeButton->setPixmap(QPixmap(":/icons/cross-button.png"));
                    connect(closeButton, &ButtonLabel::clicked, dockWidget, &QObject::deleteLater);
                    headerBox->addWidget(closeButton);
                }

                ButtonLabel *hideButton = new ButtonLabel();
                //hideButton->setText("_");
                hideButton->setPixmap(QPixmap(":/icons/chevron.png"));
                connect(hideButton, &ButtonLabel::clicked, [hideButton, section]() -> void
                {
                    section->setVisible(!section->isVisible());
                    if(section->isVisible())
                    {
                        hideButton->setPixmap(QPixmap(":/icons/chevron.png"));
                    }
                    else
                    {
                        hideButton->setPixmap(QPixmap(":/icons/chevron-expand.png"));
                    }
                });
                headerBox->addWidget(hideButton);
            }
            headerContainer->setLayout(headerBox);
        }
        layout->addWidget(headerContainer);
        layout->addWidget(section);
    }
    dockWidget->setLayout(layout);
    dockWidget->setWindowTitle(title);
    connect(section, &QObject::destroyed, dockWidget, &QAction::deleteLater);
    if (obj)
        connect(section, &QObject::destroyed, obj, &QAction::deleteLater);

    /*
    QFlags<QDockWidget::DockWidgetFeature> features =
            QDockWidget::DockWidgetFloatable |
            QDockWidget::DockWidgetMovable;
    if(closeable)
    {
        features |= QDockWidget::DockWidgetClosable;
    }
    dockWidget->setFeatures(features);
    dockWidget->setAttribute(Qt::WA_DeleteOnClose);
    */


    /*
    QAction *sectionEntry = this->sectionMenu->addAction(dockWidget->windowTitle());
    sectionEntry->setCheckable(true);
    sectionEntry->setChecked(true);
    connect(sectionEntry, &QAction::toggled, this, [=]() -> void
    {
        dockWidget->setVisible(sectionEntry->isChecked());
    });
    dockWidget->setProperty("menuEntry", QVariant::fromValue(sectionEntry));
    connect(dockWidget, &QObject::destroyed, sectionEntry, &QAction::deleteLater);
    */

    dockWidget->setProperty("section", QVariant::fromValue(section));

    section->setProperty("dockWidget", QVariant::fromValue(dockWidget));

    this->sections->addWidget(dockWidget);

    bool visible = !obj || this->objectTree->selectedObject() == obj;
    dockWidget->setVisible(visible);
    if (visible)
        visibleSections << section;

    return dockWidget;
}

void SidePanel::addProgressBar(Operation* operation) {
    if (QThread::currentThread() != qApp->thread()) {
        qCritical() << "SidePanel::addProgressBar called from outside main thread";
        return;
    }
    if (QThread::currentThread() != operation->thread()) {
        qCritical() << "Operation does not belong to main thread";
        return;
    }

    auto widget = new QWidget();
    auto layout = new QVBoxLayout();
    layout->setMargin(0);
    widget->setLayout(layout);

    //connect(operation, &QObject::destroyed, widget, &QObject::deleteLater);
    connect(operation, &QObject::destroyed, widget, [widget] { delete widget; });

    bottomLayout->addWidget(widget);

    auto hbox = new QWidget();
    auto hboxLayout = new QHBoxLayout();
    hboxLayout->setMargin(0);
    hbox->setLayout(hboxLayout);
    layout->addWidget(hbox);

    auto label = new QLabel(operation->description());
    hboxLayout->addWidget(label);
    connect(operation, &Operation::descriptionChanged, label, &QLabel::setText);

    auto cancelButton = new ButtonLabel();
    //auto cancelButton = new QPushButton();
    //cancelButton->setText("X");
    cancelButton->setPixmap(QPixmap(":/icons/cross-button.png"));
    //button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hboxLayout->addWidget(cancelButton);
    connect(cancelButton, &ButtonLabel::clicked, operation, &Operation::cancel);

    auto bar = new QProgressBar();
    layout->addWidget(bar);

    bar->setMinimum(0);
    bar->setMaximum(1000000);
    connect(operation, &Operation::progressChanged, bar, [bar] (float value) {
            //qDebug() << value;
            bar->setValue((int) (value * 1000000 + 0.5));
        });
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
