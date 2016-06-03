#include "test.hpp"

#include <Voxie/data/floatimage.hpp>

#include <Voxie/filtermask/imagecomparator.hpp>
#include <Voxie/filtermask/rectangle.hpp>
#include <Voxie/filtermask/selection2dmask.hpp>
#include <Voxie/filtermask/shapes.hpp>

#include <iostream>

#include <math.h>

#include <QtCore/QDebug>

#include <QtGui/QPixmap>

#include <QtWidgets/QLabel>

#define PI 3.14159265
using namespace voxie::filter;
using namespace voxie::data;

test::test()
{
    /*std::cout << "halo" << std::endl;
	rectangle b(5, 5, 6, 6);
	b.translateOrigin(-1,+1);
	//b.isPointIn(QPointF(5,6));

	selection2DMask mask;
	mask.addRectangle(5,5,10,10);
   // mask.addEllipse(5, 5, 2, 5);
	if(mask.isPointIn(QPointF(7,7))){
		qDebug()<<"drin";
	} else {
		qDebug()<< "ürin";
	}
*/
/*
    FloatImage sourceImage(200, 200);
    FloatImage filteredImage(200, 200);


    for (int x = 0; x < 200; x++)
    {
        for (int y = 0; y < 200; y++)
        {
            sourceImage.setPixel(x, y, 0);
            filteredImage.setPixel(x, y, 255);
        }
    }
    ImageComparator comp;
    Selection2DMask* mask = new Selection2DMask();
    mask->addRectangle(100, 100, 110, 150);
    //mask->addEllipse(10, 100, 10, 20);
    //mask->addEllipse(100, 100, 100, 40);
    QVector<QPointF> bla;

    bla.append(QPointF(0, 0));
    bla.append((QPointF(100, 100)));
    bla.append(QPointF(0, 100));
    bla.append(QPointF(0, 0));
   // mask->addPolygon(bla);
    //FloatImage targett = comp.compareImageCPU(sourceImage, filteredImage, QRectF(0,0,200,200), mask);
   // targett.toQImage().save("test1.png");
    //mask->rotate(-66);
    //mask->translateOrigin(100, 0);
    FloatImage target = comp.compareImageCPU(sourceImage, filteredImage, QRectF(0,0,200,200), mask);
    target.setPixel(100, 100, 0);

    bla.append(QPointF(100, 0));
    bla.append((QPointF(200, 100)));
    bla.append(QPointF(100, 200));
    bla.append(QPointF(0, 100));
    bla.append((QPointF(100, 0)));
    //mask->addPolygon(bla);
    //mask->rotate(-90);
   // mask->translateOrigin(15, 15);
  // FloatImage target = comp.compareImageCPU(sourceImage, filteredImage, QRectF(0,0,200,200), mask);

    QLabel* l = new QLabel();
    l->setPixmap(QPixmap::fromImage(target.toQImage()));
    l->show();
*/
    //target.toQImage().save("test.png");

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
