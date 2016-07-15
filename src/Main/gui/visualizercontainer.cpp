#include "visualizercontainer.hpp"

#include <QtCore/QDebug>
#include <QtCore/QEvent>

#include <QtGui/QCloseEvent>

#include <QtWidgets/QMenuBar>
#include <QtWidgets/QVBoxLayout>

VisualizerContainer::VisualizerContainer(QMdiArea *container, voxie::visualization::Visualizer *visualizer) :
    QWidget(nullptr),
    //icon(":/icons/application-blue.png"),
    icon(visualizer->icon()),
    visualizer(visualizer),
    container(container),
    window(nullptr)
{
    visualizer->setParent(this);
    this->setWindowTitle(this->visualizer->mainView()->windowTitle());
    this->setWindowIcon(icon);
    {
        QVBoxLayout *layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(0);
        {
            QMenuBar *bar = new QMenuBar();
            bar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);

            QAction *popOutAction = bar->addAction("&Pop Out");
            connect(popOutAction, &QAction::triggered, this, &VisualizerContainer::switchPopState);

            layout->addWidget(bar);
            layout->addWidget(visualizer->mainView());
        }
        this->setLayout(layout);
    }

    this->moveToNewMdiChild();
}

VisualizerContainer::~VisualizerContainer()
{
    //qDebug() << "delete visContainer";
    if(this->window != nullptr)
    {
        delete this->window;
    }
}

void VisualizerContainer::changeEvent(QEvent *event)
{
    if(this->window != nullptr)
    {
        return;
    }
    if(
        (event->type() != QEvent::WindowStateChange) &&
        (event->type() != QEvent::ActivationChange))
    {
        return;
    }
    emit this->sidePanelVisiblityChanged(this->isActiveWindow());
}

void VisualizerContainer::subWindowChanged(Qt::WindowStates oldState, Qt::WindowStates newState)
{
    bool prev = (oldState & Qt::WindowActive) != 0;
    bool curr = (newState & Qt::WindowActive) != 0;

    if(prev != curr)
    {
        emit this->sidePanelVisiblityChanged(curr);
    }
}

static void deleteObj(QMdiSubWindow **ref)
{
    auto tmp = *ref;
    *ref = nullptr;
    delete tmp;
}

void VisualizerContainer::switchPopState()
{
    if(this->window != nullptr)
    {
        this->setParent(nullptr);
        deleteObj(&this->window);
        this->show();
    }
    else
    {
        this->moveToNewMdiChild();
    }
}

void VisualizerContainer::moveToNewMdiChild()
{
    if(this->window != nullptr)
    {
        this->setParent(nullptr);
        deleteObj(&this->window);
    }

    this->window = this->container->addSubWindow(this);
    this->window->move(10, 10);
    this->window->setWindowIcon(icon);
    this->window->show();

    connect(this->window, &QMdiSubWindow::destroyed, this, &VisualizerContainer::destroyme);

    this->container->setActiveSubWindow(this->window);

    connect(this->window, &QMdiSubWindow::windowStateChanged, this, &VisualizerContainer::subWindowChanged);

    this->sidePanelVisiblityChanged(true);
}

void VisualizerContainer::destroyme(QObject*)
{
    if(this->window != nullptr)
    {
        this->window = nullptr;
        this->deleteLater();
    }
}

void VisualizerContainer::activate()
{
    if(this->window != nullptr)
    {
        this->container->setActiveSubWindow(this->window);
    }
    else
    {
        this->activateWindow();
    }
}

void VisualizerContainer::closeWindow()
{
    if(this->window != nullptr)
    {
        this->window->close();
    } else {
        this->close();
    }
}

void VisualizerContainer::closeEvent(QCloseEvent *ev)
{
    ev->accept();
    if(this->parent() == nullptr)
    {
        // will not delete itself without parent
        this->deleteLater();
    }
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
