#pragma once

#include <Voxie/visualization/visualizer.hpp>

#include <QtCore/QObject>

#include <QtGui/QPaintEvent>

class ExampleVisualizer : public voxie::visualization::Visualizer
{
	Q_OBJECT
private:
	QVector<QWidget*> sections;
	float (*f)(float);
public:
	explicit ExampleVisualizer(QWidget *parent = 0);
	~ExampleVisualizer();

protected:
	virtual void paintEvent(QPaintEvent *event) override;

signals:

public slots:
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
