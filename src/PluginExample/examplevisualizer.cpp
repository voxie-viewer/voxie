#include "examplevisualizer.hpp"

#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>

using namespace voxie::visualization;

ExampleVisualizer::ExampleVisualizer() {
    this->setDisplayName("Sweet Curves");
    view = new ExampleView();
    this->dynamicSections().append(view->form);
}
ExampleVisualizer::~ExampleVisualizer() {
}

ExampleView::ExampleView(QWidget *parent) :
    QWidget(parent),
    f()
{
    this->setWindowTitle("Sweet Curves");
    this->setMinimumSize(300, 200);

    // Init function + section
    {
        QFormLayout *layout = new QFormLayout();

        QLabel *name = new QLabel("f(x) = ");
        QLabel *func = new QLabel("1");

        static int i = 0;
        switch((i++) % 4)
        {
        case 0:
            func->setText("x");
            this->f = [](float x) -> float { return x; };
            break;
        case 1:
            func->setText("x²");
            this->f = [](float x) -> float { return x * x; };
            break;
        case 2:
            func->setText("tan(0.5 * x)");
            this->f = [](float x) -> float { return std::tan(0.5 * x); };
            break;
        case 3:
            func->setText("sin(x)");
            this->f = [](float x) -> float { return std::sin(x); };
            break;
        default:
            this->f = [](float x) -> float { (void)x; return 1; };
            break;
        }
        layout->addRow(name, func);

        form = new QWidget();
        form->setLayout(layout);
        form->setWindowTitle("Function Information");
    }
}

ExampleView::~ExampleView()
{

}

void ExampleView::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);

	painter.fillRect(0, 0, this->width(), this->height(), QColor(255,255,255));

	float zoom = 15;
	float centerX = 0.5f * this->width();
	float centerY = 0.5f * this->height();

	painter.drawLine(centerX, 0, centerX, this->height());
	painter.drawLine(0, centerY, this->width(), centerY);

	int ticksX = this->width() / zoom + 1;
	int ticksY = this->height() / zoom + 1;
	for(int t = -ticksX; t <= ticksX; t++)
	{
		painter.drawLine(centerX + t * zoom, centerY - 2, centerX + t * zoom, centerY + 2);
	}
	for(int t = -ticksY; t <= ticksY; t++)
	{
		painter.drawLine(centerX - 2, centerY + t * zoom, centerX + 2, centerY + t * zoom);
	}

	float delta = 1.0f / zoom;
	for(float x = -ticksX; x < ticksX; x += delta)
	{
		float y0 = this->f(x);
		float y1 = this->f(x + delta);

		painter.drawLine(centerX + x * zoom, centerY - zoom * y0, centerX + (x+delta) * zoom, centerY - zoom * y1);
	}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
