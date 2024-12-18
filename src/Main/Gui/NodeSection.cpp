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

#include "NodeSection.hpp"

#include <Main/Gui/ButtonLabel.hpp>
#include <Main/Gui/HorizontalScrollArea.hpp>

#include <Voxie/Node/Node.hpp>

#include <QtCore/QDebug>
#include <QtCore/QVariant>

#include <QtWidgets/QAction>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>

vx::NodeSection::NodeSection(Node* node, QWidget* contentWidget, bool customUi,
                             bool isInitiallyExpanded)
    : contentWidget_(contentWidget), node_(node), expanded_(true) {
  QString title = contentWidget->windowTitle();
  if (title.length() == 0) {
    qDebug() << "Window title not set for contentWidget" << contentWidget;
  }

  // scrollArea_ = new QScrollArea();
  scrollArea_ = new HorizontalScrollArea();
  scrollArea_->setWidget(contentWidget);

  QVBoxLayout* layout = new QVBoxLayout();
  layout->setMargin(0);
  {
    QWidget* headerContainer = new QWidget();
    headerContainer->setStyleSheet(
        "QWidget { background-color: gray; color: white }");
    {
      QHBoxLayout* headerBox = new QHBoxLayout();
      headerBox->setMargin(0);
      headerBox->setSpacing(0);
      {
        auto spaceX = 24 / 96.0 * this->logicalDpiX();
        auto spaceY = 24 / 96.0 * this->logicalDpiY();
        QSpacerItem* spacer =
            new QSpacerItem(spaceX
                            // + (closeable ? spaceX : 0)
                            ,
                            spaceY, QSizePolicy::Minimum, QSizePolicy::Minimum);
        headerBox->addSpacerItem(spacer);

        QLabel* header = new QLabel("<b>" + title + "</b>");
        connect(contentWidget, &QWidget::windowTitleChanged, header, [=]() {
          header->setText("<b>" + contentWidget->windowTitle() + "</b>");
        });
        header->setAlignment(Qt::AlignCenter);
        headerBox->addWidget(header);

        // button for custom ui widget in sidepanel to return to main view
        if (customUi) {
          auto customUiButton = new vx::gui::ButtonLabel();
          customUiButton->setPixmap(QPixmap(":/icons/arrow-return.png"));
          connect(customUiButton, &vx::gui::ButtonLabel::clicked, this,
                  [this]() { Q_EMIT this->closeCustomUi(this->node_); });
          // TODO: Is this correct?
          // TODO: Is this needed? This seems to work without this because
          // destroying the object also deselects it.
          if (0)
            connect(
                node, &QObject::destroyed, this,
                [this]() { Q_EMIT this->closeCustomUi(nullptr); },
                Qt::QueuedConnection);

          headerBox->addWidget(customUiButton);
        } else {
          /*
          if (closeable) {
            auto closeButton = new vx::gui::ButtonLabel();
            closeButton->setPixmap(QPixmap(":/icons/cross-script.png"));
            connect(closeButton, &vx::gui::ButtonLabel::clicked, this,
                    &QObject::deleteLater);
            headerBox->addWidget(closeButton);
          }
          */

          this->expandButton = new vx::gui::ButtonLabel();
          connect(expandButton, &vx::gui::ButtonLabel::clicked, this,
                  [this]() -> void { this->setExpanded(!this->isExpanded()); });
          headerBox->addWidget(this->expandButton);

          // setExpandButtonIcon();
          this->setExpanded(isInitiallyExpanded);
        }
      }
      headerContainer->setLayout(headerBox);
    }
    layout->addWidget(headerContainer);

    // layout->addWidget(contentWidget);

    layout->addWidget(scrollArea_);
  }
  this->setLayout(layout);
  this->setWindowTitle(title);
  connect(contentWidget, &QObject::destroyed, this, &QAction::deleteLater);
  // TODO: Rename / don't use properties
  this->setProperty("section", QVariant::fromValue(contentWidget));
  contentWidget->setProperty("dockWidget", QVariant::fromValue(this));
}

void vx::NodeSection::setExpanded(bool expanded) {
  this->expanded_ = expanded;

  // this->contentWidget()->setVisible(expanded);
  this->scrollArea_->setVisible(expanded);

  setExpandButtonIcon();
}

void vx::NodeSection::setExpandButtonIcon() {
  if (this->isExpanded()) {
    this->expandButton->setPixmap(QPixmap(":/icons/chevron.png"));
  } else {
    this->expandButton->setPixmap(QPixmap(":/icons/chevron-expand.png"));
  }
}
