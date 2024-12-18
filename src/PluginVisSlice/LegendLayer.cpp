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

#include "LegendLayer.hpp"

#include <PluginVisSlice/SliceVisualizer.hpp>

#include <Voxie/Node/ParameterCopy.hpp>

using namespace vx;

LegendLayer::LegendLayer(SliceVisualizer* sv) : sv(sv) {
  // Redraw when the image legend is shown / hidden
  connect(sv->properties, &SliceProperties::showLegendChanged, this,
          &Layer::triggerRedraw);
  // Bridge from MultivariateDataWidget to SliceVisualizer to Layer
  connect(this->sv, &SliceVisualizer::multivariateDataPropertiesChangedOut,
          this, &Layer::triggerRedraw);

  this->buildHsvComponentGradient();
}

void LegendLayer::render(QImage& outputImage,
                         const QSharedPointer<vx::ParameterCopy>& parameters,
                         bool isMainImage) {
  Q_UNUSED(isMainImage);

  SlicePropertiesCopy properties(
      parameters->properties()[parameters->mainNodePath()]);

  if (!properties.showLegend()) return;
  const int colorbarHeight = 200;
  const int colorbarWidth = 30;
  this->drawLegend(&outputImage, sv,
                   QPoint(6, outputImage.height() - colorbarHeight - 40),
                   colorbarWidth, colorbarHeight);
}

void LegendLayer::buildHsvComponentGradient() {
  // build Gradient for Hue mapping
  QLinearGradient hueGradient;
  for (qreal i = 0; i < (QColorHueMax); i += 10) {
    hueGradient.setColorAt(i / qreal(QColorHueMax),
                           QColor::fromHsv(i, QColorSatMax, QColorValueMax));
  }
  hueGradient.setSpread(QGradient::RepeatSpread);
  this->hueGradient = hueGradient;

  // build Gradient for Saturation mapping
  QLinearGradient satGradient;
  satGradient.setColorAt(0.0, Qt::white);
  satGradient.setColorAt(1.0, Qt::blue);
  satGradient.setSpread(QGradient::RepeatSpread);
  this->satGradient = satGradient;

  // build Gradient for Value mapping
  QLinearGradient valGradient;
  valGradient.setColorAt(0, Qt::black);
  valGradient.setColorAt(1, Qt::green);
  valGradient.setSpread(QGradient::RepeatSpread);
  this->valGradient = valGradient;
};

QPair<int, int> LegendLayer::getTextWidthHeight(QFont font, QString text) {
  QRect br = QFontMetrics(font).boundingRect(text);
  return {br.width(), br.height()};
}

void LegendLayer::drawLegend(QImage* outputImage, SliceVisualizer* sv,
                             QPoint upperLeft, qreal colorbarWidth,
                             qreal colorbarHeight) {
  MultivariateDataWidget* multiWid = sv->getMultivariateDataWidget();
  ActiveMode act = multiWid->getActiveMode();

  if (act != ActiveMode::overview && act != ActiveMode::effZ) {
    return;
  }
  MappingStrategy mappingStrat;
  if (act == ActiveMode::overview) {
    mappingStrat = getOverviewStrategyDetails(multiWid->getOverviewStrategy());
  }
  if (act == ActiveMode::effZ) {
    mappingStrat = getEffZStrategyDetails(multiWid->getEffZStrategy());
  }
  int spacing = 8;
  int fontSize = spacing * 2;
  int penWidth = 2;
  int currentX = upperLeft.x() + spacing;
  int currentY = upperLeft.y() + spacing;

  QPainter painter(outputImage);
  QFont font;
  font.setPixelSize(fontSize);
  painter.setFont(font);

  QPen pen;
  pen.setWidth(penWidth);
  painter.setPen(pen);

  // get mappings
  for (int i = 0; i < mappingStrat.targetScaling.size(); i++) {
    QString targetScaling = mappingStrat.targetScaling.at(i);
    QString sourceScaling = mappingStrat.sourceScaling.at(i);

    // draw source scale above color bar
    // TODO: What happens if sourceMin / sourceMax are not set?
    float sourceMin = 0.0;
    float sourceMax = 1.0;

    if (act == ActiveMode::overview) {
      if (sourceScaling == "Average") {
        sourceMin = multiWid->getOverviewAvgMin();
        sourceMax = multiWid->getOverviewAvgMax();
      } else if (sourceScaling == "Std. Dev.") {
        sourceMin = multiWid->getOverviewStdDevMin();
        sourceMax = multiWid->getOverviewStdDevMax();
      } else {
        qWarning()
            << "Source Scaling could not be identified in LegendLayer.cpp";
      }
    } else if (act == ActiveMode::effZ) {
      if (sourceScaling == "Eff.Z") {
        sourceMin = multiWid->getEffZ_EffZMin();
        sourceMax = multiWid->getEffZ_EffZMax();
      } else if (sourceScaling == "Density") {
        sourceMin = multiWid->getEffZ_DensityMin();
        sourceMax = multiWid->getEffZ_DensityMax();
      } else {
        qWarning()
            << "Source Scaling could not be identified in LegendLayer.cpp";
      }
    } else {
      qWarning() << "ActiveMode could not be identified in LegendLayer.cpp";
    }

    QLinearGradient targetGradient;
    int targetMinVal = 0;
    QString targetMinText = "";
    int targetMaxVal = 1;
    QString targetMaxText = "";

    if (targetScaling == "Hue") {
      targetGradient = this->hueGradient;
      targetMinVal = QColorHueMin;
      targetMinText = QString::number(QColorHueMin) + QColorHueUnit;
      targetMaxVal = QColorHueMax;
      targetMaxText = QString::number(QColorHueMax) + QColorHueUnit;
    } else

        if (targetScaling == "Saturation") {
      targetGradient = this->satGradient;
      targetMinVal = QColorSatMin;
      targetMinText = QString::number(QColorSatMin);
      targetMaxVal = QColorSatMax;
      targetMaxText = QString::number(QColorSatMax);
    } else

        if (targetScaling == "Value") {
      targetGradient = this->valGradient;
      targetMinVal = QColorValueMin;
      targetMinText = QString::number(QColorValueMin);
      targetMaxVal = QColorValueMax;
      targetMaxText = QString::number(QColorValueMax);
    } else {
      qWarning() << "Source Scaling could not be identified in LegendLayer.cpp";
    }

    if (colorbarWidth >= colorbarHeight) {
      // draw legend horizontal (build from top to bottom)
      // # draw name of mapping strategy
      painter.drawText(currentX, currentY,
                       sourceScaling + "->" + targetScaling);
      currentY += spacing;

      // # draw source scale over color bar
      painter.drawLine(currentX, currentY, currentX, currentY + spacing);
      painter.drawLine(currentX + colorbarWidth, currentY,
                       currentX + colorbarWidth, currentY + spacing);

      currentY += spacing;

      painter.drawText(currentX + 1, currentY, QString::number(sourceMin));
      painter.drawText(currentX + colorbarWidth + 1, currentY,
                       QString::number(sourceMax));
      currentY += 1;

      // # draw colorbar itself
      targetGradient.setStart(currentX, currentY);
      targetGradient.setFinalStop(currentX + colorbarWidth, currentY);
      painter.fillRect(QRect(currentX, currentY, colorbarWidth, colorbarHeight),
                       QBrush(targetGradient));

      currentY += colorbarHeight;

      // # draw target scale below color bar
      painter.drawLine(currentX, currentY, currentX, currentY + spacing);
      painter.drawLine(currentX + colorbarWidth, currentY,
                       currentX + colorbarWidth, currentY + spacing);
      currentY += spacing * 2;

      painter.drawText(currentX + 1, currentY, targetMinText);
      painter.drawText(currentX + colorbarWidth + 1, currentY, targetMaxText);
      currentY += (2 * spacing);

    } else {
      // draw legend vertical (build from left to right)
      int top = upperLeft.y() + spacing;
      // # set name of mapping strategy
      QString headerLabel = sourceScaling + "->" + targetScaling;
      int headertop = top;

      top += (this->getTextWidthHeight(painter.font(), headerLabel).second) +
             spacing;
      QRect colorbar(currentX, top, colorbarWidth, colorbarHeight);
      // # calculate space for additional colorbar labels from top to bottom
      QStringList leftTextList;
      leftTextList.append(QString::number(sourceMin));

      QStringList rightTextList;
      rightTextList.append(targetMinText);

      int leftGTextWidth = 0;
      int rightGTextWidth = 0;
      int gTextHeight = this->getTextWidthHeight(painter.font(), "0123456789")
                            .second;  // greatest Text Height

      int actualLineHeigth = gTextHeight + 4;

      int countPossibleLines = colorbar.height() / actualLineHeigth;
      qreal spaceBetweenLines =
          qreal(colorbar.height()) / qreal(countPossibleLines);
      if (countPossibleLines > leftTextList.size()) {
        // there is enough space to add more colorbar scaling lines

        // # get text of new lines
        int additionalLineCount = countPossibleLines - leftTextList.size();
        for (qreal k = 1; k <= additionalLineCount; k++) {
          qreal newLeftValue =
              (spaceBetweenLines * k) *
                  ((sourceMax - sourceMin) / colorbar.height()) +
              sourceMin;
          leftTextList.append(QString::number(newLeftValue, 'f', 2));

          qreal newRighthValue = (spaceBetweenLines * k) *
                                 ((qreal(targetMaxVal) - qreal(targetMinVal)) /
                                      qreal(colorbar.height()) +
                                  qreal(targetMinVal));
          rightTextList.append(QString::number(newRighthValue, 'f', 2));
        }
      }
      leftTextList.append(QString::number(sourceMax));
      rightTextList.append(targetMaxText);

      for (QString text : leftTextList) {
        leftGTextWidth =
            std::max(this->getTextWidthHeight(painter.font(), text).first,
                     leftGTextWidth);
      }
      leftGTextWidth += spacing + 1;

      for (QString text : rightTextList) {
        rightGTextWidth =
            std::max(this->getTextWidthHeight(painter.font(), text).first,
                     rightGTextWidth);
      }
      rightGTextWidth += spacing + 1;

      colorbar.setX(colorbar.x() + leftGTextWidth);  // warning: changes width
      colorbar.setWidth(colorbarWidth);              // resets width

      // # draw semi-transparent background
      int backgroundWidth =
          std::max(leftGTextWidth + colorbar.width() + rightGTextWidth,
                   getTextWidthHeight(painter.font(), headerLabel).first);
      int backgroundHeight = top - upperLeft.y() + colorbar.height();
      QRect backgroundRect =
          QRect(currentX, upperLeft.y(), backgroundWidth, backgroundHeight);
      painter.fillRect(backgroundRect, QColor(150, 150, 150, 150));

      // # draw name of mapping strategy
      painter.drawText(
          currentX,
          headertop +
              getTextWidthHeight(painter.font(), headerLabel).second / 2,
          headerLabel);

      // # draw source scale left of color bar
      for (int j = 0; j < leftTextList.size(); j++) {
        painter.drawLine(colorbar.x() - spacing,
                         colorbar.y() + (spaceBetweenLines * j), colorbar.x(),
                         colorbar.y() + (spaceBetweenLines * j));
        painter.drawText(
            colorbar.x() -
                this->getTextWidthHeight(painter.font(), leftTextList.at(j))
                    .first -
                spacing - 1,
            colorbar.y() + (spaceBetweenLines * j), leftTextList.at(j));
      }

      // # draw colorbar itself
      targetGradient.setStart(colorbar.x() + colorbar.width() / 2,
                              colorbar.y());
      targetGradient.setFinalStop(colorbar.x() + colorbar.width() / 2,
                                  colorbar.y() + colorbar.height());
      painter.fillRect(colorbar, QBrush(targetGradient));

      // # draw target scale right of color bar
      for (int l = 0; l < rightTextList.size(); l++) {
        painter.drawLine(colorbar.x() + colorbar.width(),
                         colorbar.y() + (spaceBetweenLines * l),
                         colorbar.x() + colorbar.width() + spacing,
                         colorbar.y() + (spaceBetweenLines * l));

        painter.drawText(colorbar.x() + colorbar.width() + spacing + 1,
                         colorbar.y() + (spaceBetweenLines * l),
                         rightTextList.at(l));
      }
      currentX += colorbar.x() + colorbar.width() + rightGTextWidth + 2;
    }
  }
};
