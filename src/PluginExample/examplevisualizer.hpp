#pragma once

#include <Voxie/visualization/visualizer.hpp>

#include <QtCore/QObject>

#include <QtGui/QPaintEvent>

class ExampleView : public QWidget {
	Q_OBJECT
private:
	QVector<QWidget*> sections;
	float (*f)(float);
public:
	explicit ExampleView(QWidget *parent = 0);
	~ExampleView();

    QWidget *form;

protected:
	virtual void paintEvent(QPaintEvent *event) override;
};

class ExampleVisualizer : public voxie::visualization::Visualizer {
	Q_OBJECT

    ExampleView* view;

public:
    ExampleVisualizer();
	~ExampleVisualizer();

    voxie::plugin::MetaVisualizer* type() const override;

    QWidget* mainView() override {
        return view;
    }
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
