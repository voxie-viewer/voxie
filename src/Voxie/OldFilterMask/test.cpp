/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "test.hpp"

#include <VoxieBackend/Data/FloatImage.hpp>

#include <Voxie/OldFilterMask/ImageComparator.hpp>
#include <Voxie/OldFilterMask/Selection2DMask.hpp>
#include <Voxie/OldFilterMask/rectangle.hpp>
#include <Voxie/OldFilterMask/shapes.hpp>

#include <iostream>

#include <math.h>

#include <QtCore/QDebug>

#include <QtGui/QPixmap>

#include <QtWidgets/QLabel>

#define PI 3.14159265
using namespace vx::filter;
using namespace vx;

test::test() {
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
      //FloatImage targett = comp.compareImageCPU(sourceImage, filteredImage,
    QRectF(0,0,200,200), mask);
     // targett.toQImage().save("test1.png");
      //mask->rotate(-66);
      //mask->translateOrigin(100, 0);
      FloatImage target = comp.compareImageCPU(sourceImage, filteredImage,
    QRectF(0,0,200,200), mask); target.setPixel(100, 100, 0);

      bla.append(QPointF(100, 0));
      bla.append((QPointF(200, 100)));
      bla.append(QPointF(100, 200));
      bla.append(QPointF(0, 100));
      bla.append((QPointF(100, 0)));
      //mask->addPolygon(bla);
      //mask->rotate(-90);
     // mask->translateOrigin(15, 15);
    // FloatImage target = comp.compareImageCPU(sourceImage, filteredImage,
    QRectF(0,0,200,200), mask);

      QLabel* l = new QLabel();
      l->setPixmap(QPixmap::fromImage(target.toQImage()));
      l->show();
  */
  // target.toQImage().save("test.png");
}
