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

#include "SliceView.hpp"

#include <Main/Root.hpp>

#include <Main/Gui/PlaneView.hpp>

#include <Voxie/IO/SliceExporter.hpp>

#include <Voxie/Component/Plugin.hpp>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>

#include <QApplication>
#include <QClipboard>
//#include <src/Mian/gui/PosRotValid>

#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

using namespace vx;
using namespace vx::gui;
using namespace vx::plugin;
using namespace vx::io;

// Regular Expresion for an floatnumber
static QRegExp validInput("-?[0-9]+(.[0-9]*)?");

SliceView::SliceView(PlaneNode* slice, QWidget* parent)
    : QWidget(parent), slice(slice) {
  connect(this->slice, &PlaneNode::planeChanged, this,
          &SliceView::sliceChanged);
  connect(slice, &Node::displayNameChanged, this,
          [=]() { this->setWindowTitle(this->slice->displayName()); });
  /*
  connect(slice->getDataset(), &Node::displayNameChanged, this, [=]() {
    this->setWindowTitle(this->slice->getDataset()->displayName() + " - " +
                         this->slice->displayName());
  });
  this->setWindowTitle(this->slice->getDataset()->displayName() + " - " +
                       this->slice->displayName());
  */
  this->setWindowTitle(this->slice->displayName());

  QMetaObject::Connection conni =
      connect(this->slice, &QObject::destroyed, [this]() -> void {
        this->slice = nullptr;
        this->deleteLater();
      });
  connect(this, &QObject::destroyed,
          [=]() -> void { this->disconnect(conni); });

  QVBoxLayout* rootLayout = new QVBoxLayout();
  {
    QFormLayout* form = new QFormLayout();
    {
      QToolBar* toolbar = new QToolBar();

      // Creates a button for the export of single slices
      QToolButton* sliceExport = new QToolButton();
      sliceExport->setIcon(QIcon(":/icons/disk.png"));
      sliceExport->setText("Export Slice");
      sliceExport->setPopupMode(QToolButton::InstantPopup);

      // Creates a button for selecting the Representation of the rotation
      QToolButton* rotationRepresentation = new QToolButton();
      rotationRepresentation->setIcon(QIcon(":/icons/ruler.png"));
      rotationRepresentation->setText("Choose Rotation Representation");
      rotationRepresentation->setPopupMode(QToolButton::InstantPopup);

      // Creates a button for the copy of position and rotation
      QToolButton* copyButt = new QToolButton();
      copyButt->setIcon(QIcon(":/icons/blue-document-export.png"));
      copyButt->setText("Copy Content");
      copyButt->setPopupMode(QToolButton::InstantPopup);

      // Creates a button for the paste of position and rotation
      QToolButton* pasteButt = new QToolButton();
      pasteButt->setIcon(QIcon(":/icons/blue-document-import.png"));
      pasteButt->setText("Paste Content");
      pasteButt->setPopupMode(QToolButton::InstantPopup);

      // Sets the Dropdown Elements and their behavior for the export of slices
      QMenu* contextMenuSliceExporter = new QMenu(this);

      // TODO: provide some new method to export slices?
      /*
      for (const auto& plugin : ::vx::Root::instance()->plugins()) {
        if (plugin->sliceExporters().size() == 0) {
          continue;
        }
        QMenu* pluginAction = contextMenuSliceExporter->addMenu(plugin->name());
        for (SliceExporter* exporter : plugin->sliceExporters()) {
          QAction* action = pluginAction->addAction(exporter->objectName());
          connect(action, &QAction::triggered,
                  [=]() -> void { exporter->exportGui(this->slice); });
        }
      }
      */

      // Sets the Dropdown Elements and their behavior for the Representation of
      // Roatation
      QMenu* contextMenuRotationRepresentation = new QMenu(this);
      contextMenuRotationRepresentation->setToolTip(
          "Selection of the rotation representation");
      QAction* quaternion = new QAction("Quaternion (WXYZ)", this);
      connect(quaternion, &QAction::triggered, [=]() -> void {
        rotationType = true;
        SliceView::sliceChanged();
      });

      QAction* eulerAngles = new QAction("Euler Angles (XYZ)", this);
      connect(eulerAngles, &QAction::triggered, [=]() -> void {
        rotationType = false;
        SliceView::sliceChanged();
      });

      contextMenuRotationRepresentation->addAction(quaternion);
      contextMenuRotationRepresentation->addAction(eulerAngles);

      // Sets the Dropdown Elements and their behavior for copy Button
      QMenu* contextMenuCopy = new QMenu(this);
      contextMenuCopy->setToolTip("Copy Slice Properties");

      QAction* copyPosAndRot = new QAction("Copy Position and Rotation", this);
      connect(copyPosAndRot, &QAction::triggered,
              [=]() -> void {  // called if dropdown element "Copy Position and
                               // Rotation" is clicked
                SliceView::copyPosAndRotToClipboard();
              });

      contextMenuCopy->addAction(copyPosAndRot);

      // Sets the Dropdown Elements and their behavior for paste Buttons
      QMenu* contextMenuPaste = new QMenu(this);
      contextMenuPaste->setToolTip("Selection of Paste options");

      QAction* pastePos = new QAction("Paste Position", this);
      connect(pastePos, &QAction::triggered,
              [=]() -> void {  // called if dropdown element "Paste Position" is
                               // clicked
                PosRotValid* posRotValid = SliceView::analyseClipboardContent();
                SliceView::pastePos(posRotValid);
                delete posRotValid;

                if (this->planeView) this->planeView->update();
                SliceView::sliceChanged();

              });

      contextMenuPaste->addAction(pastePos);

      QAction* pasteRot = new QAction("Paste Rotation", this);
      connect(pasteRot, &QAction::triggered,
              [=]() -> void {  // called if dropdown element "Paste Rotation" is
                               // clicked
                PosRotValid* posRotValid = SliceView::analyseClipboardContent();
                SliceView::pasteRot(posRotValid);
                delete posRotValid;

                if (this->planeView) this->planeView->update();
                SliceView::sliceChanged();

              });

      contextMenuPaste->addAction(pasteRot);

      QAction* pastePosAndRot =
          new QAction("Paste Position and Rotation", this);
      connect(pastePosAndRot, &QAction::triggered,
              [=]() -> void {  // called if dropdown element "Paste Position and
                               // Rotation" is clicked
                PosRotValid* posRotValid = SliceView::analyseClipboardContent();
                SliceView::pastePos(posRotValid);
                SliceView::pasteRot(posRotValid);
                delete posRotValid;

                if (this->planeView) this->planeView->update();
                SliceView::sliceChanged();

              });

      contextMenuPaste->addAction(pastePosAndRot);

      // Sets GUI elements
      sliceExport->setMenu(contextMenuSliceExporter);
      toolbar->addWidget(sliceExport);

      rotationRepresentation->setMenu(contextMenuRotationRepresentation);
      toolbar->addWidget(rotationRepresentation);

      copyButt->setMenu(contextMenuCopy);
      toolbar->addWidget(copyButt);

      pasteButt->setMenu(contextMenuPaste);
      toolbar->addWidget(pasteButt);

      form->addWidget(toolbar);

      // Create an new Textfield with name positionEditX
      this->positionEditX = new QLineEdit();
      this->positionEditX->setToolTip("Position of the pane in the 3D object");
      this->positionEditX->setText(
          QString::number(this->slice->plane()->origin.x()));
      connect(this->positionEditX, &QLineEdit::returnPressed, this,
              &SliceView::positionEdited);

      // Create an new Textfield with name positionEditY
      this->positionEditY = new QLineEdit();
      this->positionEditY->setText(
          QString::number(this->slice->plane()->origin.y()));
      connect(this->positionEditY, &QLineEdit::returnPressed, this,
              &SliceView::positionEdited);

      // Create an new Textfield with name positionEditZ
      this->positionEditZ = new QLineEdit();
      this->positionEditZ->setText(
          QString::number(this->slice->plane()->origin.z()));
      connect(this->positionEditZ, &QLineEdit::returnPressed, this,
              &SliceView::positionEdited);

      // Adds new Rows with label and a Textfield
      form->addRow("Position X", this->positionEditX);
      form->addRow("Position Y", this->positionEditY);
      form->addRow("Position Z", this->positionEditZ);

      // Create an new Textfield with name rotationEditX0
      this->rotationEditX0 = new QLineEdit();
      this->rotationEditX0->setToolTip(
          "Rotation of the pane using the quaternion representation");
      this->rotationEditX0->setText(
          QString::number(this->slice->plane()->rotation.scalar()));
      connect(this->rotationEditX0, &QLineEdit::returnPressed, this,
              &SliceView::rotationEdited);

      // Create an new Textfield with name rotationEditX1
      this->rotationEditX1 = new QLineEdit();
      this->rotationEditX1->setText(
          QString::number(this->slice->plane()->rotation.x()));
      connect(this->rotationEditX1, &QLineEdit::returnPressed, this,
              &SliceView::rotationEdited);

      // Create an new Textfield with name rotationEditX2
      this->rotationEditX2 = new QLineEdit();
      this->rotationEditX2->setText(
          QString::number(this->slice->plane()->rotation.y()));
      connect(this->rotationEditX2, &QLineEdit::returnPressed, this,
              &SliceView::rotationEdited);

      // Create an new Textfield with name rotationEditX3
      this->rotationEditX3 = new QLineEdit();
      this->rotationEditX3->setText(
          QString::number(this->slice->plane()->rotation.z()));
      connect(this->rotationEditX3, &QLineEdit::returnPressed, this,
              &SliceView::rotationEdited);

      // Adds new Rows with label and a Textfield
      form->addRow("Rotation Scalar", this->rotationEditX0);
      form->addRow("Rotation X", this->rotationEditX1);
      form->addRow("Rotation Y", this->rotationEditX2);
      form->addRow("Rotation Z", this->rotationEditX3);
    }
    rootLayout->addLayout(form);

    QHBoxLayout* glView = new QHBoxLayout();
    {
      this->planeView = nullptr;
      // TODO: create a PlaneView somehow?
      /*
      this->planeView = new PlaneView(this->slice);

      this->planeView->setSizePolicy(QSizePolicy::Expanding,
                                     QSizePolicy::Expanding);

      glView->addWidget(this->planeView);
      */
    }
    rootLayout->addLayout(glView);
  }
  this->setLayout(rootLayout);
}

SliceView::~SliceView() {
  if (this->slice != nullptr) this->slice->deleteLater();
}

/**
 * @brief This method is called if the slice is changed and updated the values
 */
void SliceView::sliceChanged() {
  this->rotationEditX0->setEnabled(true);
  this->positionEditX->setText(
      QString::number(this->slice->plane()->origin.x()));
  this->positionEditY->setText(
      QString::number(this->slice->plane()->origin.y()));
  this->positionEditZ->setText(
      QString::number(this->slice->plane()->origin.z()));

  SliceView::changedRotation();
}

/**
 * @brief This method is called by sliceChanged() and updated the rotation
 */
void SliceView::changedRotation() {
  if (rotationType) {
    this->rotationEditX0->setText(
        QString::number(this->slice->plane()->rotation.scalar()));
    this->rotationEditX1->setText(
        QString::number(this->slice->plane()->rotation.x()));
    this->rotationEditX2->setText(
        QString::number(this->slice->plane()->rotation.y()));
    this->rotationEditX3->setText(
        QString::number(this->slice->plane()->rotation.z()));
  } else {
    this->rotationEditX0->setText("");
    this->rotationEditX0->setEnabled(false);
    this->rotationEditX1->setText(
        QString::number(this->slice->plane()->rotation.toEulerAngles().x()));
    this->rotationEditX2->setText(
        QString::number(this->slice->plane()->rotation.toEulerAngles().y()));
    this->rotationEditX3->setText(
        QString::number(this->slice->plane()->rotation.toEulerAngles().z()));
  }

  if (this->planeView) this->planeView->update();
}

/**
 * @brief This methode is called if an value of the position is edited in an
 * Textfield
 */
void SliceView::positionEdited() {
  if (::validInput.exactMatch(this->positionEditX->text()) &&
      ::validInput.exactMatch(this->positionEditY->text()) &&
      ::validInput.exactMatch(this->positionEditZ->text())) {
    this->slice->setOrigin(QVector3D(this->positionEditX->text().toFloat(),
                                     this->positionEditY->text().toFloat(),
                                     this->positionEditZ->text().toFloat()));
    if (this->planeView) this->planeView->update();
  } else {
    this->positionEditX->setText(
        QString::number(this->slice->plane()->origin.x()));
    this->positionEditY->setText(
        QString::number(this->slice->plane()->origin.y()));
    this->positionEditZ->setText(
        QString::number(this->slice->plane()->origin.z()));
  }
}

/**
 * @brief This methode is called if an value of the rotation is edited in an
 * Textfield
 */
void SliceView::rotationEdited() {
  if (::validInput.exactMatch(this->rotationEditX0->text()) &&
      ::validInput.exactMatch(this->rotationEditX1->text()) &&
      ::validInput.exactMatch(this->rotationEditX2->text()) &&
      ::validInput.exactMatch(this->rotationEditX3->text())) {
    this->slice->setRotation(QQuaternion(this->rotationEditX0->text().toFloat(),
                                         this->rotationEditX1->text().toFloat(),
                                         this->rotationEditX2->text().toFloat(),
                                         this->rotationEditX3->text().toFloat())
                                 .normalized());
    if (this->planeView) this->planeView->update();
  } else {
    SliceView::changedRotation();
  }
}

/**
 * @brief This Method copy the Position and Rotation into the system clipboard
 */
void SliceView::copyPosAndRotToClipboard() {
  QString str = QString::number(this->slice->plane()->origin.x());
  str.operator+=("$");
  str.operator+=(QString::number(this->slice->plane()->origin.y()));
  str.operator+=("$");
  str.operator+=(QString::number(this->slice->plane()->origin.z()));

  str.operator+=("§");

  str.operator+=(QString::number(this->slice->plane()->rotation.scalar()));
  str.operator+=("$");
  str.operator+=(QString::number(this->slice->plane()->rotation.x()));
  str.operator+=("$");
  str.operator+=(QString::number(this->slice->plane()->rotation.y()));
  str.operator+=("$");
  str.operator+=(QString::number(this->slice->plane()->rotation.z()));

  QClipboard* clip = QApplication::clipboard();
  clip->setText(str);
}

PosRotValid* SliceView::analyseClipboardContent() {
  bool valid = true;
  QString syntaxWarningAllg = "Input from clipboard haven´t the right syntax";
  QString syntaxWarningValues = "not the right number of values";
  QString syntaxWarningSpecialChar =
      "not the right number of special character";

  QClipboard* clip = QApplication::clipboard();
  QString str = clip->text();

  str = str.trimmed();
  QStringList strPos;
  QStringList strRot;
  if (str.count("$") == 5 && str.count("§") == 1) {
    QStringList strList = str.split("§");
    strPos = strList.at(0).split("$");
    strRot = strList.at(1).split("$");

    if (strPos.size() == 3 && strRot.size() == 4) {
      for (int i = 0; i < strPos.size(); i++) {
        if (!::validInput.exactMatch(strPos.at(i))) {
          qWarning() << syntaxWarningAllg << ": strPos";
          valid = false;
          break;
        }
      }
      for (int i = 0; i < strRot.size(); i++) {
        if (!::validInput.exactMatch(strRot.at(i))) {
          qWarning() << syntaxWarningAllg << ": strRot";
          valid = false;
          break;
        }
      }

    } else {
      qWarning() << syntaxWarningAllg << ": " << syntaxWarningValues;
      valid = false;
    }
  } else {
    qWarning() << syntaxWarningAllg << ": " << syntaxWarningSpecialChar;
    valid = false;
  }
  PosRotValid* posRotValid = new PosRotValid(strPos, strRot, valid);
  return posRotValid;
}

void SliceView::pastePos(PosRotValid* posRotValid) {
  if (posRotValid->isValid() && posRotValid->getStrPos().size() == 3) {
    this->slice->setOrigin(QVector3D(posRotValid->getStrPos().at(0).toFloat(),
                                     posRotValid->getStrPos().at(1).toFloat(),
                                     posRotValid->getStrPos().at(2).toFloat()));
  }
}

void SliceView::pasteRot(PosRotValid* posRotValid) {
  if (posRotValid->isValid() && posRotValid->getStrRot().size() == 4) {
    this->slice->setRotation(
        QQuaternion(posRotValid->getStrRot().at(0).toFloat(),
                    posRotValid->getStrRot().at(1).toFloat(),
                    posRotValid->getStrRot().at(2).toFloat(),
                    posRotValid->getStrRot().at(3).toFloat()));
  }
}
