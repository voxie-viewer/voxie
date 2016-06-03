#pragma once

#include <Voxie/visualization/visualizer.hpp>

#include <QtWidgets/QAction>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMdiSubWindow>
#include <QtWidgets/QWidget>

class VisualizerContainer : public QWidget
{
    Q_OBJECT
    QIcon icon;
public:
    voxie::visualization::Visualizer * const visualizer;
    QMdiArea * const container;
    QMdiSubWindow * window;
public:
    explicit VisualizerContainer(QMdiArea *container, voxie::visualization::Visualizer *visualizer);

    ~VisualizerContainer();

    virtual void changeEvent(QEvent *event) override;

    void activate();

    void closeWindow();

protected:
    virtual void closeEvent(QCloseEvent *ev) override;

private:
    void switchPopState();

    void moveToNewMdiChild();

    void subWindowChanged(Qt::WindowStates oldState, Qt::WindowStates newState);

    void destroyme(QObject*);

signals:
    void sidePanelVisiblityChanged(bool isVisible);


public slots:

};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
