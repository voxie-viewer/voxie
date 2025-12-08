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

#include "Types.hpp"

#include <VoxieClient/DBusTypeList.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/DBusUtilVoxie.hpp>
#include <VoxieClient/DBusVariant.hpp>
#include <VoxieClient/Format.hpp>
#include <VoxieClient/JsonDBus.hpp>
#include <VoxieClient/JsonUtil.hpp>
#include <VoxieClient/QtCollections.hpp>

#include <Voxie/DebugOptions.hpp>
#include <Voxie/IVoxie.hpp>

#include <Voxie/Component/HelpCommon.hpp>

#include <Voxie/Data/BoundingBox3D.hpp>
#include <Voxie/Data/ChemicalComposition.hpp>
#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorInterpolator.hpp>
#include <Voxie/Data/ColorizerEntry.hpp>
#include <Voxie/Data/ContainerNode.hpp>
#include <Voxie/Data/GeometricPrimitiveObject.hpp>
#include <Voxie/Data/PiecewisePolynomialFunction.hpp>
#include <Voxie/Data/SurfaceNode.hpp>
#include <Voxie/Data/TensorComponent.hpp>
#include <Voxie/Data/TomographyRawDataNode.hpp>

#include <VoxieBackend/Data/DataProperty.hpp>
#include <VoxieBackend/Data/DataType.hpp>
#include <VoxieBackend/Data/GeometricPrimitive.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveType.hpp>
#include <VoxieBackend/Data/SeriesData.hpp>
#include <VoxieBackend/Data/SurfaceAttribute.hpp>
#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>

#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodeNodeProperty.hpp>
#include <Voxie/Node/NodeProperty.hpp>
#include <Voxie/Node/PropertyHelper.hpp>
#include <Voxie/Node/PropertyUI.hpp>
#include <Voxie/Node/PropertyUIImplBase.hpp>
#include <Voxie/Node/PropertyValueConvertDBus.hpp>
#include <Voxie/Node/StringConversionHelper.hpp>

#include <Voxie/Gui/ColorizerWidget.hpp>
#include <Voxie/Gui/ErrorMessage.hpp>
#include <Voxie/Gui/Int64SpinBox.hpp>
#include <Voxie/Gui/LabelTableView.hpp>
#include <Voxie/Gui/ObjectProperties.hpp>
#include <Voxie/Gui/UnitSpinBox.hpp>

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

#include <QtGui/QVector3D>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>

using namespace vx;

namespace {
class FloatUI : public PropertyUIImplBase<vx::types::Float> {
  QDoubleSpinBox* input;

 public:
  FloatUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    bool hasUnit = property->rawJson().contains("Unit");

    input = hasUnit ? new UnitSpinBox() : new QDoubleSpinBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    // TODO: Do this? Allow both '.' and ',' in values?
    input->setLocale(QLocale::c());

    // TODO: Allow typing more digits than 'decimals' into QDoubleSpinBox,
    // similar to UnitSpinBox

    // input->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    if (hasUnit) {
      QString unitString;
      for (const auto& entry : property->rawJson()["Unit"].toArray()) {
        auto list = entry.toArray();
        QString unit = list[0].toString();
        int exp = list[1].toInt();
        if (exp == 0) continue;

        unitString += unit;
        if (exp != 1) {
          unitString += "^";
          unitString += QString::number(exp);
        }
      }
      input->setSuffix(unitString);
    }

    // TODO: What minimum / maximum values should be used by default?
    double min = -1e+9;
    double max = 1e+9;
    if (property->hasMinimum()) min = property->doubleMinimum();
    if (property->hasMaximum()) max = property->doubleMaximum();
    // qDebug() << property->hasMinimum() << property->hasMaximum() << min <<
    // max;
    input->setRange(min, max);

    if (!hasUnit) input->setDecimals(3);  // TODO

    QObject::connect(
        input, (void(QDoubleSpinBox::*)(double)) & QDoubleSpinBox::valueChanged,
        this, [this](double value) {
          // qDebug() << "Value in QDoubleSpinBox changed to" << value;
          setValueChecked(value);
        });
  }

  void updateUIValue(const double& value) override {
    // qDebug() << "Property change of" << property()->name() << "to" << value;
    this->input->setValue(value);
  }

  QWidget* widget() override { return input; }
};

class IntUI : public PropertyUIImplBase<vx::types::Int> {
  Int64SpinBox* input;

 public:
  IntUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    input = new Int64SpinBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    // qint64 min = std::numeric_limits<qint64>::min();
    // qint64 max = std::numeric_limits<qint64>::max();
    qint64 min = std::numeric_limits<int>::min();
    qint64 max = std::numeric_limits<int>::max();
    if (property->hasMinimum()) min = property->intMinimum();
    if (property->hasMaximum()) max = property->intMaximum();
    input->setRange(min, max);

    QObject::connect(
        input,
        //(void (QSpinBox::*)(int)) & QSpinBox::valueChanged, this,
        (void(Int64SpinBox::*)(qint64)) & Int64SpinBox::valueChanged, this,
        [this](int value) {
          setValueChecked(value);
          // qDebug() << "Value in QSpinBox changed to" << value;
        });
  }

  void updateUIValue(const qint64& value) override {
    this->input->setValue(value);
  }

  QWidget* widget() override { return input; }
};

class IntListUI : public PropertyUI {
  QSpinBox* input;

 public:
  IntListUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUI(property, node) {
    input = new QSpinBox();
    // TODO: fill these UI functions to display int list
  }

  QWidget* widget() override { return input; }
};

class LabelListUI : public PropertyUI {
  LabelViewModel* labelViewModel;
  QTableView* input = new QTableView();
  QSharedPointer<vx::TableData> labelTable;

 public:
  LabelListUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUI(property, node) {
    QObject::connect(node, &Node::propertyChanged, this,
                     &LabelListUI::updateLabelTable);

    QObject::connect(this, &QObject::destroyed, this->input,
                     &QObject::deleteLater);

    updateLabelTable();
  }

  void updateLabelTable() {
    // TODO: This seems to be broken (should not hardcode a property name here)
    if (!this->node()->propertyValues().contains(
            "de.uni_stuttgart.Voxie.ContainerNode")) {
      return;
    }
    auto containerProperty = this->node()->prototype()->getProperty(
        "de.uni_stuttgart.Voxie.ContainerNode", false);

    auto val = this->node()->getNodeProperty(containerProperty);

    auto gpo = dynamic_cast<ContainerNode*>(Node::parseVariantNode(val));

    // TODO: What happens if gpo is null? List should probably be emptied.
    // (Should also not be called gpo)
    if (gpo) {
      auto containerData = gpo->getCompoundPointer();

      if (!containerData.isNull()) {
        this->labelTable = qSharedPointerDynamicCast<TableData>(
            containerData->getElementOrNull("labelTable"));

        // TODO: What happens if there is no labelTable or it is not a
        // TableData?

        if (this->labelTable) {
          labelViewModel = new LabelViewModel(this->labelTable, nullptr);
          this->input = setLabelTableView(this->input, labelViewModel);

          // visual sugar
          this->input->setColumnHidden(
              labelViewModel->getLabelTable()->getColumnIndexByName(
                  "Visibility"),
              true);

          QObject::connect(this->labelViewModel,
                           &QAbstractItemModel::dataChanged, this,
                           &LabelListUI::updateExtractLabelProperty);
        }
      }
    }
  }

  void updateExtractLabelProperty() {
    if (!this->labelTable.isNull()) {
      QList<qulonglong> labelIDsToExport;
      QList<TableRow> tableRows = this->labelTable.data()->getRowsByIndex();

      // Get LabelIDs that shall be exported
      for (const TableRow& row : tableRows) {
        // if Export is true
        if (row.data()
                .at(labelTable->getColumnIndexByName("Export"))
                .value<bool>()) {
          qulonglong labelID =
              row.data()
                  .at(labelTable->getColumnIndexByName("LabelID"))
                  .value<qulonglong>();
          labelIDsToExport.append(labelID);
        }
      }
      // Put labelIDs hat shall be exported into the properties
      this->node()->setNodePropertyTyped<QList<qulonglong>>(
          "de.uni_stuttgart.Voxie.LabelIDs",
          vx::PropertyValueConvertRaw<
              QList<qulonglong>, QList<qulonglong>>::toRaw(labelIDsToExport));
    }
  }

  QWidget* widget() override { return input; }
};

class ThresholdLabelMappingUI : public PropertyUI {
  QSpinBox* input;

 public:
  ThresholdLabelMappingUI(const QSharedPointer<NodeProperty>& property,
                          Node* object)
      : PropertyUI(property, object) {
    input = new QSpinBox();
    // TODO: fill these UI functions to display ThresholdLabelMapping
  }

  QWidget* widget() override { return input; }
};

class ListPosition3DUI : public PropertyUI {
  QSpinBox* input;

 public:
  ListPosition3DUI(const QSharedPointer<NodeProperty>& property, Node* object)
      : PropertyUI(property, object) {
    input = new QSpinBox();
    // TODO: fill these UI functions to display ListPosition3D
  }

  QWidget* widget() override { return input; }
};

class ListPosition3DDoubleTupleUI : public PropertyUI {
  QSpinBox* input;

 public:
  ListPosition3DDoubleTupleUI(const QSharedPointer<NodeProperty>& property,
                              Node* object)
      : PropertyUI(property, object) {
    input = new QSpinBox();
    // TODO: fill these UI functions to display ListPosition3DDoubleTuple
  }

  QWidget* widget() override { return input; }
};

class VolumeIndexListUI : public PropertyUI {
  QSpinBox* input;

 public:
  VolumeIndexListUI(const QSharedPointer<NodeProperty>& property, Node* object)
      : PropertyUI(property, object) {
    input = new QSpinBox();
    // TODO: fill these UI functions to display VolumeIndexList
  }

  QWidget* widget() override { return input; }
};

// TODO: needs large value range, with custom class derived from
// QAbstractSpingBox instead of QSpinBox
class UInt64UI : public PropertyUI {
  QSpinBox* input;

 public:
  UInt64UI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUI(property, node) {
    input = new QSpinBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    QObject::connect(input, (void(QSpinBox::*)(int)) & QSpinBox::valueChanged,
                     this, [this](int value) {
                       // qDebug() << "Value in QSpinBox changed to" << value;
                       if (!this->node()) return;
                       try {
                         this->node()->setNodeProperty(
                             this->property(), vx::make_any<qulonglong>(value));
                       } catch (Exception& e) {
                         qCritical() << "Error while updating property value:"
                                     << e.what();
                       }
                     });

    QObject::connect(node, &Node::propertyChanged, this,
                     [this](const QSharedPointer<NodeProperty>& property2,
                            const vx::Any& value) {
                       // qDebug() << "Property change of" <<
                       // property->name()
                       // << "to" << value;
                       if (property2 != this->property()) return;
                       if (!this->node()) return;
                       // qDebug() << "Property change of" <<
                       // property->name()
                       // << "to" << value;
                       auto valueInt = Node::parseVariant<qulonglong>(value);
                       // TODO: overflow?
                       this->input->setValue(valueInt);
                     });

    try {
      auto value = this->node()->getNodeProperty(this->property());
      auto valueInt = Node::parseVariant<qulonglong>(value);
      // TODO: overflow?
      this->input->setValue(valueInt);
    } catch (Exception& e) {
      qCritical() << "Error while setting initial value:" << e.what();
    }
  }

  QWidget* widget() override { return input; }
};

class BooleanUI : public PropertyUI {
  QCheckBox* input;

 public:
  BooleanUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUI(property, node) {
    input = new QCheckBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    QObject::connect(input, &QCheckBox::stateChanged, this, [this](int state) {
      // qDebug() << "Value in QCheckBox changed to" << state;
      if (!this->node()) return;
      try {
        bool value = state != Qt::Unchecked;
        this->node()->setNodeProperty(this->property(),
                                      vx::make_any<bool>(value));
      } catch (Exception& e) {
        qCritical() << "Error while updating property value:" << e.what();
      }
    });

    QObject::connect(
        node, &Node::propertyChanged, this,
        [this](const QSharedPointer<NodeProperty>& property2,
               const vx::Any& value) {
          // qDebug() << "Property change of" <<
          // property->name()
          // << "to" << value;
          if (property2 != this->property()) return;
          if (!this->node()) return;
          // qDebug() << "Property change of" <<
          // property->name()
          // << "to" << value;
          auto valueCast = Node::parseVariant<bool>(value);
          this->input->setCheckState(valueCast ? Qt::Checked : Qt::Unchecked);
        });

    try {
      auto value = this->node()->getNodeProperty(this->property());
      auto valueCast = Node::parseVariant<bool>(value);
      this->input->setCheckState(valueCast ? Qt::Checked : Qt::Unchecked);
    } catch (Exception& e) {
      qCritical() << "Error while setting initial value:" << e.what();
    }
  }

  QWidget* widget() override { return input; }
};

class FileNameUI : public PropertyUI {
  QWidget* widget_;
  QLineEdit* input;
  QPushButton* button;
  QPointer<QFileDialog> fileDialog;

 public:
  FileNameUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUI(property, node) {
    input = new QLineEdit();
    button = new QPushButton();
    button->setIcon(QIcon(":/icons/folder-search-result.png"));
    connect(button, &QPushButton::pressed, this, [=]() {
      auto path = input->text();
      if (!QDir(path).exists()) path = QDir::currentPath();
      if (!fileDialog) {
        fileDialog = new QFileDialog();

        fileDialog->setNameFilters(property->patterns());
        fileDialog->setOption(QFileDialog::DontUseNativeDialog, true);
        fileDialog->setFileMode(QFileDialog::ExistingFiles);

        QObject::connect(fileDialog, &QDialog::finished, this, [this]() {
          if (fileDialog->selectedFiles().count() != 1) {
            qWarning() << "FileNameUI: Only 1 File allowed";
            return;
          }
          QString directory = fileDialog->selectedFiles()[0];
          if (!directory.isEmpty()) {
            if (!this->node()) return;
            try {
              this->node()->setNodeProperty(this->property(),
                                            vx::make_any<QString>(directory));
            } catch (Exception& e) {
              qCritical() << "Error while updating property value:" << e.what();
            }
          }
          fileDialog->deleteLater();
        });
        fileDialog->show();
      } else {
        fileDialog->show();
        fileDialog->raise();
      }
    });
    widget_ = new QWidget();
    auto layout = new QHBoxLayout();
    layout->addWidget(input);
    layout->addWidget(button);
    widget_->setLayout(layout);

    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    // TODO: Use textEdited or textChanged?
    QObject::connect(
        input, (void(QLineEdit::*)(const QString&)) & QLineEdit::textEdited,
        this, [this](const QString& value) {
          // qDebug() << "Value in QLineEdit changed to" << value;
          if (!this->node()) return;
          try {
            this->node()->setNodeProperty(this->property(),
                                          vx::make_any<QString>(value));
          } catch (Exception& e) {
            qCritical() << "Error while updating property value:" << e.what();
          }
        });

    QObject::connect(node, &Node::propertyChanged, this,
                     [this](const QSharedPointer<NodeProperty>& property2,
                            const vx::Any& value) {
                       // qDebug() << "Property change of" <<
                       // property2->name()
                       // << "to" << value;
                       if (property2 != this->property()) return;
                       if (!this->node()) return;
                       // qDebug() << "Property change of" <<
                       // property2->name()
                       // << "to" << value;
                       auto valueCast = Node::parseVariant<QString>(value);
                       this->input->setText(valueCast);
                     });

    try {
      auto value = this->node()->getNodeProperty(this->property());
      auto valueCast = Node::parseVariant<QString>(value);
      this->input->setText(valueCast);
    } catch (Exception& e) {
      qCritical() << "Error while setting initial value:" << e.what();
    }
  }

  ~FileNameUI() {
    if (!fileDialog.isNull()) fileDialog->deleteLater();
  }

  QWidget* widget() override { return widget_; }
};

class StringUI : public PropertyUIImplBase<vx::types::String> {
  QLineEdit* input;

 public:
  StringUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    input = new QLineEdit();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    // TODO: Use textEdited or textChanged?
    QObject::connect(
        input, (void(QLineEdit::*)(const QString&)) & QLineEdit::textEdited,
        this, [this](const QString& value) {
          if (vx::debug_option::Log_Properties_UI()->enabled())
            qDebug() << "Value in QLineEdit changed to" << value;
          this->setValueChecked(value);
        });

    try {
      auto value = this->node()->getNodeProperty(this->property());
      auto valueCast = Node::parseVariant<QString>(value);
      this->input->setText(valueCast);
    } catch (Exception& e) {
      qCritical() << "Error while setting initial value:" << e.what();
    }
  }

  void updateUIValue(const QString& value) override {
    if (vx::debug_option::Log_Properties_UI()->enabled())
      qDebug() << "Property change of" << property()->name() << "to" << value;
    this->input->setText(value);
  }

  QWidget* widget() override { return input; }
};

class EnumerationUI : public PropertyUI {
  QComboBox* input;

 public:
  EnumerationUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUI(property, node) {
    input = new QComboBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    QObject::connect(
        input, (void(QComboBox::*)(int)) & QComboBox::activated, this,
        [this](int index) {
          // qDebug() << "Value in QComboBox changed to" << index;
          if (!this->node()) return;
          try {
            auto valueVar = input->itemData(index);
            if (valueVar.userType() != qMetaTypeId<QString>())
              throw Exception("de.uni_stuttgart.Voxie.Error",
                              QString() + "Got a " +
                                  QMetaType::typeName(valueVar.userType()) +
                                  " instead of a QString in QComboBox");
            auto value = valueVar.value<QString>();
            this->node()->setNodeProperty(this->property(),
                                          vx::make_any<QString>(value));
          } catch (Exception& e) {
            qCritical() << "Error while updating property value:" << e.what();
          }
        });

    QObject::connect(node, &Node::propertyChanged, this,
                     [this](const QSharedPointer<NodeProperty>& property2,
                            const vx::Any& value) {
                       // qDebug() << "Property change of" <<
                       // property->name()
                       // << "to" << value;
                       if (property2 != this->property()) return;
                       if (!this->node()) return;
                       // qDebug() << "Property change of" <<
                       // property->name()
                       // << "to" << value;
                       auto valueCast = Node::parseVariant<QString>(value);

                       int index;
                       bool found = false;
                       for (int i = 0; i < input->count(); i++) {
                         if (input->itemData(i).toString() == valueCast) {
                           index = i;
                           found = true;
                         }
                       }
                       if (!found) {
                         qWarning() << "Unable to find value '" + valueCast +
                                           "' for enum for property " +
                                           this->property()->name();
                         index = 0;
                       }
                       input->setCurrentIndex(index);
                     });

    try {
      auto value = this->node()->getNodeProperty(this->property());
      auto valueCast = Node::parseVariant<QString>(value);

      int index;
      bool found = false;
      int i = 0;
      QList<EnumEntry> entries = this->property()->enumEntries();
      std::sort(entries.begin(), entries.end(),
                [](const EnumEntry& v1, const EnumEntry& v2) {
                  if (v1.uiPosition() < v2.uiPosition()) return true;
                  if (v1.uiPosition() > v2.uiPosition()) return false;
                  return v1.name() < v2.name();
                });
      for (const auto& entry : entries) {
        input->addItem(entry.displayName(),
                       QVariant::fromValue<QString>(entry.name()));
        if (valueCast == entry.name()) {
          index = i;
          found = true;
        }
        i++;
      }

      if (!found) {
        qWarning() << "Unable to find value '" + valueCast +
                          "' for enum for property " + this->property()->name();
        index = 0;
      }
      input->setCurrentIndex(index);
    } catch (Exception& e) {
      qCritical() << "Error while setting initial value:" << e.what();
    }
  }

  QWidget* widget() override { return input; }
};

class DataTypeUI : public PropertyUIImplBase<vx::types::DataType> {
  QMap<DataType, int> typeToIndex;
  QMap<int, DataType> indexToType;

  QComboBox* input;

 public:
  DataTypeUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    input = new QComboBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    input->clear();
    int index = 0;

    for (const auto& type : vx::DataTypeNames.keys()) {
      auto name = vx::DataTypeNames.value(type);

      typeToIndex[type] = index;
      indexToType[index] = type;

      // input->addItem(name, QVariant::fromValue<DataType>(type));
      input->addItem(name, QVariant::fromValue<int>(index));

      index++;
    }

    QObject::connect(
        input, (void(QComboBox::*)(int)) & QComboBox::activated, this,
        [this](int index2) { setValueChecked(indexToType.value(index2)); });
  }

  void updateUIValue(const vx::DataType& value) override {
    input->setCurrentIndex(typeToIndex[value]);
  }

  QWidget* widget() override { return input; }
};

// TODO: move somewhere else
// TODO: remove, replace by MakeHandButton in src/Voxie/Gui/MakeHandButton?
namespace {
class MakeHandButton : public QPushButton {
 public:
  MakeHandButton(QWidget* parent = 0) : QPushButton(parent) {}

 protected:
  void enterEvent(QEvent* event) override {
    QPushButton::enterEvent(event);

    setCursor(Qt::PointingHandCursor);
  }
};
}  // namespace

class ColorUI : public PropertyUIImplBase<vx::types::Color> {
 public:
  QWidget* widget_;

  QPushButton* colorWidget;
  QColorDialog* colorPicker;

  bool showAlpha;
  QSlider* alphaSlider;
  bool ignoreAlphaSliderUpdate = false;

  ColorUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    this->showAlpha = false;
    const auto& json = property->rawJson();
    if (json.contains("ShowAlpha"))
      this->showAlpha = json["ShowAlpha"].toBool();

    this->colorWidget = new MakeHandButton();
    QObject::connect(this, &QObject::destroyed, this->colorWidget,
                     &QObject::deleteLater);
    this->colorWidget->setToolTip("Choose Color");
    this->colorWidget->setFixedSize(40, 25);
    setStyle(Color::black());

    this->colorPicker = new QColorDialog();
    QObject::connect(this, &QObject::destroyed, this->colorPicker,
                     &QObject::deleteLater);
    this->colorPicker->setOption(QColorDialog::ShowAlphaChannel);
    // On Ubuntu 18.04 the native dialog does not seem to allow to edit the
    // alpha channel
    this->colorPicker->setOption(QColorDialog::DontUseNativeDialog);

    if (this->showAlpha) {
      this->alphaSlider = new QSlider();
      QObject::connect(this, &QObject::destroyed, this->alphaSlider,
                       &QObject::deleteLater);

      this->alphaSlider->setMinimum(0);
      this->alphaSlider->setMaximum(65536);
      this->alphaSlider->setSingleStep(2048);
      this->alphaSlider->setPageStep(16384);
      this->alphaSlider->setOrientation(Qt::Horizontal);
      this->alphaSlider->setToolTip("Adjust the opacity");
      this->alphaSlider->setTickPosition(QSlider::TicksAbove);
      this->alphaSlider->setTickInterval(2048);

      auto labels = new QHBoxLayout();
      QObject::connect(this, &QObject::destroyed, labels,
                       &QObject::deleteLater);
      auto transparent = new QLabel("Transparent");
      labels->addWidget(transparent);
      labels->addStretch(1);
      auto opaque = new QLabel("Opaque");
      labels->addWidget(opaque);
      auto alphaLayout = new QVBoxLayout();
      QObject::connect(this, &QObject::destroyed, alphaLayout,
                       &QObject::deleteLater);
      alphaLayout->addLayout(labels);
      alphaLayout->addWidget(this->alphaSlider);

      widget_ = new QWidget();
      QObject::connect(this, &QObject::destroyed, widget_,
                       &QObject::deleteLater);
      auto layout = new QHBoxLayout(widget_);
      // layout->addWidget(this->alphaSlider);
      layout->addLayout(alphaLayout);
      layout->addWidget(this->colorWidget);

      connect(this->alphaSlider, &QSlider::valueChanged, [this](int value) {
        Color current;
        try {
          current = getValue();
        } catch (Exception& e) {
          qCritical() << "Error while reading property value:" << e.what();
          return;
        }

        // This is needed when setAlphaSlider() was called by the color
        // update to prevent this from calling setValueChecked() a second
        // time
        if (ignoreAlphaSliderUpdate) return;

        current.setAlpha(std::max(0.0, std::min(1.0, value / 65536.0)));
        setValueChecked(current);
        setStyle(current);
      });
    } else {
      this->alphaSlider = nullptr;
      widget_ = this->colorWidget;
    }

    connect(this->colorWidget, &QPushButton::clicked, [=]() -> void {
      Color current;
      try {
        current = getValue();
      } catch (Exception& e) {
        qCritical() << "Error while reading property value:" << e.what();
        return;
      }

      this->colorPicker->setCurrentColor(current.asQColor());
      this->colorPicker->exec();
      QColor col = this->colorPicker->selectedColor();
      if (col.isValid()) {
        Color c = col;
        setValueChecked(c);
        setStyle(c);
        setAlphaSlider(c);
      }
    });
  }

  bool isMultiline() override { return showAlpha; }

  void setStyle(const vx::Color& color) {
    this->colorWidget->setStyleSheet(
        "QWidget {background: " + color.asQColor().name() +
        "} QPushButton {border: 1px solid lightGray}");
  }

  void setAlphaSlider(const vx::Color& color) {
    if (!this->alphaSlider) return;
    this->ignoreAlphaSliderUpdate = true;
    this->alphaSlider->setValue((int)std::round(color.alpha() * 65536));
    this->ignoreAlphaSliderUpdate = false;
  }

  void updateUIValue(const vx::Color& value) override {
    setStyle(value);
    setAlphaSlider(value);
  }

  QWidget* widget() override { return widget_; }
};

class Point2DUI : public PropertyUIImplBase<vx::types::Point2D> {
 public:
  QLineEdit* entryX;
  QLineEdit* entryY;
  QWidget* widget_;
  bool suppressUpdate = false;

  static float myParseFloat(const QString& str, bool* ok) {
    QString str2 = str;
    str2.replace(",", ".");
    return str2.toFloat(ok);
  }

  Point2DUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    widget_ = new QWidget();
    QObject::connect(this, &QObject::destroyed, widget_, &QObject::deleteLater);
    auto layout = new QHBoxLayout(widget_);
    entryX = new QLineEdit();
    layout->addWidget(entryX);
    entryY = new QLineEdit();
    layout->addWidget(entryY);

    QObject::connect(
        entryX, (void(QLineEdit::*)(const QString&)) & QLineEdit::textEdited,
        this, [this](const QString& value) {
          // qDebug() << "Value in QLineEdit changed to" << value;
          bool ok = false;
          auto valueF = myParseFloat(value, &ok);
          if (!ok) {
            qWarning() << "Could not parse float value";
            return;
          }
          QPointF current;
          try {
            current = getValue();
          } catch (Exception& e) {
            qCritical() << "Error while reading property value:" << e.what();
            return;
          }
          setValueChecked(QPointF(valueF, current.y()));
        });

    // TODO: avoid code duplication?
    QObject::connect(
        entryY, (void(QLineEdit::*)(const QString&)) & QLineEdit::textEdited,
        this, [this](const QString& value) {
          // qDebug() << "Value in QLineEdit changed to" << value;
          bool ok = false;
          auto valueF = myParseFloat(value, &ok);
          if (!ok) {
            qWarning() << "Could not parse float value";
            return;
          }
          QPointF current;
          try {
            current = getValue();
          } catch (Exception& e) {
            qCritical() << "Error while reading property value:" << e.what();
            return;
          }
          setValueChecked(QPointF(current.x(), valueF));
        });
  }

  void updateUIValue(const QPointF& value) override {
    // qDebug() << "updateUIValue" << value << suppressUpdate;
    // if (!suppressUpdate)
    entryX->setText(QString::number(value.x()));
    entryY->setText(QString::number(value.y()));
  }

  QWidget* widget() override { return widget_; }
};

class Position3DUI : public PropertyUIImplBase<vx::types::Position3D> {
 public:
  ObjectProperties* posWidget;
  bool suppressUpdate = false;

  Position3DUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    posWidget = new ObjectProperties(nullptr, true, false);
    QObject::connect(this, &QObject::destroyed, posWidget,
                     &QObject::deleteLater);
    connect(posWidget, &ObjectProperties::positionChanged, this,
            [this](QVector3D pos) {
              // qDebug() << "pos" << pos;
              suppressUpdate = true;
              setValueChecked(pos);
              suppressUpdate = false;
            });
  }

  void updateUIValue(const QVector3D& value) override {
    // qDebug() << "updateUIValue" << value << suppressUpdate;
    if (!suppressUpdate) posWidget->setPosition(value);
  }

  QWidget* widget() override { return posWidget; }
};

class SizeInteger3DUI : public PropertyUIImplBase<vx::types::SizeInteger3D> {
 public:
  QWidget* widget_;
  Int64SpinBox* entry[3];

  SizeInteger3DUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    widget_ = new QWidget();
    auto layout = new QHBoxLayout();
    widget_->setLayout(layout);
    QObject::connect(this, &QObject::destroyed, widget_, &QObject::deleteLater);
    for (int i = 0; i < 3; i++) {
      entry[i] = new Int64SpinBox();
      layout->addWidget(entry[i], 1);
      entry[i]->setRange(0, std::numeric_limits<qint64>::max());
      QObject::connect(
          entry[i],
          //(void (QSpinBox::*)(int)) & QSpinBox::valueChanged, this,
          (void(Int64SpinBox::*)(qint64)) & Int64SpinBox::valueChanged, this,
          [this, i](int value) {
            // qDebug() << "valueChanged" << i << value;
            if (value < 0) {
              qWarning() << "SizeInteger3DUI: Got negative value";
              return;
            }

            vx::Vector<quint64, 3> vec;
            try {
              vec = getValue();
            } catch (Exception& e) {
              qCritical() << "Error while reading property value:" << e.what();
              return;
            }
            vec[i] = value;
            setValueChecked(vec);
          });
    }
  }

  void updateUIValue(const vx::Vector<quint64, 3>& value) override {
    // qDebug() << "updateUIValue" << value << suppressUpdate;
    for (int i = 0; i < 3; i++) entry[i]->setValue(value[i]);
  }

  QWidget* widget() override { return widget_; }
};

// TODO: Don't use QVector3D
class Box3DAxisAlignedUI
    : public PropertyUIImplBase<vx::types::Box3DAxisAligned> {
 public:
  QWidget* widget_;
  ObjectProperties* posWidgetMin;
  ObjectProperties* posWidgetMax;

  Box3DAxisAlignedUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    widget_ = new QWidget();
    QObject::connect(this, &QObject::destroyed, widget_, &QObject::deleteLater);
    auto layout = new QVBoxLayout(widget_);

    posWidgetMin = new ObjectProperties(nullptr, true, false);
    layout->addWidget(posWidgetMin);
    posWidgetMax = new ObjectProperties(nullptr, true, false);
    layout->addWidget(posWidgetMax);

    connect(posWidgetMin, &ObjectProperties::positionChanged, this,
            [this](QVector3D pos) {
              // qDebug() << "min" << pos;
              auto old = getValue();
              setValueChecked(
                  BoundingBox3D(vectorCast<double>(toVector(pos)), old.max()));
            });
    connect(posWidgetMax, &ObjectProperties::positionChanged, this,
            [this](QVector3D pos) {
              // qDebug() << "max" << pos;
              auto old = getValue();
              setValueChecked(
                  BoundingBox3D(old.min(), vectorCast<double>(toVector(pos))));
            });
  }

  void updateUIValue(const BoundingBox3D& value) override {
    // qDebug() << "updateUIValue" << value << suppressUpdate;
    posWidgetMin->setPosition(toQVector(vectorCastNarrow<float>(value.min())));
    posWidgetMax->setPosition(toQVector(vectorCastNarrow<float>(value.max())));
  }

  QWidget* widget() override { return widget_; }
};

class Orientation3DUI : public PropertyUIImplBase<vx::types::Orientation3D> {
 public:
  ObjectProperties* posWidget;
  bool suppressUpdate = false;

  Orientation3DUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    posWidget = new ObjectProperties(nullptr, false, true);
    QObject::connect(this, &QObject::destroyed, posWidget,
                     &QObject::deleteLater);
    connect(posWidget, &ObjectProperties::rotationChanged, this,
            [this](QQuaternion rot) {
              // qDebug() << "rot" << rot;
              suppressUpdate = true;
              setValueChecked(rot);
              suppressUpdate = false;
            });
  }

  void updateUIValue(const QQuaternion& value) override {
    // qDebug() << "updateUIValue" << value << suppressUpdate;
    if (!suppressUpdate) posWidget->setRotation(value);
  }

  QWidget* widget() override { return posWidget; }
};

class ValueColorMappingUI
    : public PropertyUIImplBase<vx::types::ValueColorMapping> {
 public:
  ColorizerWidget* colorizerWidget;
  bool ignoreChange = false;
  bool verbose = false;

  ValueColorMappingUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    auto colorizer = QSharedPointer<vx::Colorizer>::create();

    this->colorizerWidget = new ColorizerWidget();
    this->colorizerWidget->setColorizer(colorizer);

    // Associate the node's histogram property with the colorizer UI
    if (auto histogramProvider =
            qSharedPointerDynamicCast<vx::HistogramProvider>(
                node->getPropertyUIData(property->name()))) {
      this->colorizerWidget->setHistogramProvider(histogramProvider);
    }

    QObject::connect(this, &QObject::destroyed, this->colorizerWidget,
                     &QObject::deleteLater);

    QObject::connect(colorizer.data(), &Colorizer::mappingChanged, this,
                     [this, colorizer] {
                       ignoreChange = true;
                       setValueChecked(colorizer->getEntriesAsQList());
                       ignoreChange = false;
                     });
  }

  void updateUIValue(const QList<ColorizerEntry>& value) override {
    if (ignoreChange) {
      if (verbose) qDebug() << "Updating ValueColorMappingUI (ignored)";
      return;
    }
    colorizerWidget->getColorizer()->setEntries(value);
  }

  QWidget* widget() override { return colorizerWidget; }

  bool isMultiline() override { return true; }
};

class GeometricPrimitiveUI
    : public PropertyUIImplBase<vx::types::GeometricPrimitive> {
  QComboBox* input;
  bool suppressUpdate = false;
  QSharedPointer<NodeProperty> parentProperty;
  QMap<quint64, int> idToIndex;
  bool haveAllowedPrimitives;
  QSet<QString> allowedPrimitives;

 public:
  GeometricPrimitiveUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase(property, node) {
    input = new QComboBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    const auto& json = property->rawJson();
    if (json.contains("ParentProperty")) {
      QString parentPropertyName = json["ParentProperty"].toString();
      parentProperty =
          node->prototype()->getProperty(parentPropertyName, false);
    } else {
      parentProperty = QSharedPointer<NodeProperty>();
      // This is only valid if the node itself is a GeometricPrimitiveNode
      if (node->prototype()->name() !=
          "de.uni_stuttgart.Voxie.Data.GeometricPrimitive")
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            "No ParentProperty key for GeometricPrimitive property " +
                property->name() + " in node " + node->prototype()->name());
    }
    haveAllowedPrimitives = json.contains("AllowedPrimitives");
    if (haveAllowedPrimitives) {
      for (const auto& val : json["AllowedPrimitives"].toArray())
        allowedPrimitives << val.toString();
    }

    if (parentProperty) {
      forwardSignalFromPropertyOnReconnect(
          node, parentProperty, &DataNode::dataChangedFinished, this,
          &GeometricPrimitiveUI::updateComboBox);
    } else {
      QObject::connect(dynamic_cast<DataNode*>(node),
                       &DataNode::dataChangedFinished, this,
                       &GeometricPrimitiveUI::updateComboBox);
    }

    QObject::connect(input, (void(QComboBox::*)(int)) & QComboBox::activated,
                     this, [this](int index) {
                       // qDebug() << "Value in QComboBox changed to" <<
                       // index;
                       auto valueVar = input->itemData(index);
                       if (valueVar.userType() != qMetaTypeId<quint64>()) {
                         qWarning() << "Got a "
                                    << QMetaType::typeName(valueVar.userType())
                                    << " instead of a quint64 in QComboBox";
                         return;
                       }
                       auto value = valueVar.value<quint64>();
                       suppressUpdate = true;
                       setValueChecked(value);
                       suppressUpdate = false;
                     });

    updateComboBox();
  }

  void updateUIValue(const quint64& currentID) override {
    // qDebug() << "updateUIValue" << currentID << suppressUpdate;

    if (suppressUpdate) return;

    int index = 0;
    if (idToIndex.contains(currentID)) index = idToIndex[currentID];
    input->setCurrentIndex(index);
  }

  QMap<quint64, QSharedPointer<GeometricPrimitive>> getPrimitives() {
    try {
      GeometricPrimitiveNode* gpo;
      if (parentProperty) {
        auto val = this->node()->getNodeProperty(parentProperty);
        gpo =
            dynamic_cast<GeometricPrimitiveNode*>(Node::parseVariantNode(val));
      } else {
        gpo = dynamic_cast<GeometricPrimitiveNode*>(this->node().data());
      }
      if (!gpo) return QMap<quint64, QSharedPointer<GeometricPrimitive>>();
      auto gpd = gpo->geometricPrimitiveData();
      if (!gpd) return QMap<quint64, QSharedPointer<GeometricPrimitive>>();
      return gpd->primitives();
    } catch (Exception& e) {
      qWarning() << "Error in GeometricPrimitiveUI::getPrimitives():"
                 << e.what();
      return QMap<quint64, QSharedPointer<GeometricPrimitive>>();
    }
  }

  void updateComboBox() {
    quint64 currentID = 0;
    try {
      currentID = getValue();
    } catch (Exception& e) {
      qCritical() << "Error while reading property value:" << e.what();
      return;
    }

    // qDebug() << "updateComboBox" << currentID << suppressUpdate;

    auto primitives = getPrimitives();
    // qDebug() << "updateComboBox prim #" << primitives.count();

    int i = 0;
    int index = 0;
    input->clear();
    idToIndex.clear();
    {
      input->addItem("<None>", QVariant::fromValue<quint64>(0));
      idToIndex[0] = i;
      i++;
    }
    if (currentID != 0 && !primitives.contains(currentID)) {
      input->addItem(QString("<Missing element #%1>").arg(currentID),
                     QVariant::fromValue<quint64>(currentID));
      idToIndex[currentID] = i;
      index = i;
      i++;
    }
    for (const auto& id : primitives.keys()) {
      auto primitive = primitives[id];
      if (haveAllowedPrimitives &&
          !allowedPrimitives.contains(primitive->primitiveType()->name()))
        continue;
      input->addItem(primitive->name(), QVariant::fromValue<quint64>(id));
      idToIndex[id] = i;
      if (id == currentID) index = i;
      i++;
    }
    input->setCurrentIndex(index);
  }

  QWidget* widget() override { return input; }
};

class SurfaceAttributeNameUI
    : public PropertyUIImplBase<vx::types::SurfaceAttributeName> {
  QComboBox* input;
  bool suppressUpdate = false;
  QSharedPointer<NodeProperty> parentProperty;
  QList<QString> values;

 public:
  SurfaceAttributeNameUI(const QSharedPointer<NodeProperty>& property,
                         Node* node)
      : PropertyUIImplBase(property, node) {
    input = new QComboBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    const auto& json = property->rawJson();

    QString parentPropertyName = json["ParentProperty"].toString();
    parentProperty = node->prototype()->getProperty(parentPropertyName, false);

    forwardSignalFromPropertyOnReconnect(
        node, parentProperty, &DataNode::dataChangedFinished, this,
        &SurfaceAttributeNameUI::updateComboBox);

    QObject::connect(input, (void(QComboBox::*)(int)) & QComboBox::activated,
                     this, [this](int index) {
                       // qDebug() << "Value in QComboBox changed to" << index;
                       auto valueVar = input->itemData(index);
                       if (valueVar.userType() != qMetaTypeId<QString>()) {
                         qWarning() << "Got a "
                                    << QMetaType::typeName(valueVar.userType())
                                    << " instead of a QString in QComboBox";
                         return;
                       }
                       auto value = valueVar.value<QString>();
                       suppressUpdate = true;
                       setValueChecked(value);
                       suppressUpdate = false;
                     });

    updateComboBox();
  }

  void updateUIValue(const QString& currentValue) override {
    // qDebug() << "updateUIValue" << currentValue << suppressUpdate;

    if (suppressUpdate) return;

    int index = -1;
    for (int i = 0; i < values.size(); i++) {
      if (values[i] == currentValue) {
        index = i;
        break;
      }
    }
    if (index == -1) {
      updateComboBox();
    } else {
      input->setCurrentIndex(index);
    }
  }

  QList<QSharedPointer<SurfaceAttribute>> getAttributes() {
    try {
      auto val = this->node()->getNodeProperty(parentProperty);

      auto obj = dynamic_cast<SurfaceNode*>(Node::parseVariantNode(val));
      if (!obj) return {};

      auto data = obj->surface();
      if (!data) return {};

      return data->listAttributes();
    } catch (Exception& e) {
      qWarning() << "Error in SurfaceAttributeNameUI::getAttributes():"
                 << e.what();
      return {};
    }
  }

  void updateComboBox() {
    QString currentValue;
    try {
      currentValue = getValue();
    } catch (Exception& e) {
      qCritical() << "Error while reading property value:" << e.what();
      return;
    }

    // qDebug() << "updateComboBox" << currentValue << suppressUpdate;

    auto attributes = getAttributes();

    int index = -1;
    input->clear();
    values.clear();
    {
      input->addItem("<None>", QVariant::fromValue<QString>(""));
      if (currentValue == "") index = values.size();
      values << "";
    }
    for (const auto& attribute : attributes) {
      input->addItem(attribute->displayName(),
                     QVariant::fromValue<QString>(attribute->name()));
      if (attribute->name() == currentValue) index = values.size();
      values << attribute->name();
    }
    if (index == -1) {
      input->addItem("Other attribute: " + currentValue,
                     QVariant::fromValue<QString>(currentValue));
      index = values.size();
      values << currentValue;
    }
    input->setCurrentIndex(index);
  }

  QWidget* widget() override { return input; }
};

class TomographyRawDataImageKindUI
    : public PropertyUIImplBase<vx::types::TomographyRawDataImageKind> {
  QComboBox* input;
  bool suppressUpdate = false;
  QSharedPointer<NodeProperty> parentProperty;
  QList<QJsonObject> values;

 public:
  TomographyRawDataImageKindUI(const QSharedPointer<NodeProperty>& property,
                               Node* node)
      : PropertyUIImplBase(property, node) {
    input = new QComboBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    const auto& json = property->rawJson();

    QString parentPropertyName = json["ParentProperty"].toString();
    parentProperty = node->prototype()->getProperty(parentPropertyName, false);

    forwardSignalFromPropertyOnReconnect(
        node, parentProperty, &DataNode::dataChangedFinished, this,
        &TomographyRawDataImageKindUI::updateComboBox);

    QObject::connect(input, (void(QComboBox::*)(int)) & QComboBox::activated,
                     this, [this](int index) {
                       // qDebug() << "Value in QComboBox changed to" << index;
                       auto valueVar = input->itemData(index);
                       if (valueVar.userType() != qMetaTypeId<QJsonObject>()) {
                         qWarning() << "Got a "
                                    << QMetaType::typeName(valueVar.userType())
                                    << " instead of a QJsonObject in QComboBox";
                         return;
                       }
                       auto value = valueVar.value<QJsonObject>();
                       suppressUpdate = true;
                       setValueChecked(value);
                       suppressUpdate = false;
                     });

    updateComboBox();
  }

  void updateUIValue(const QJsonObject& currentValue) override {
    // qDebug() << "updateUIValue" << currentValue << suppressUpdate;

    if (suppressUpdate) return;

    int index = -1;
    for (int i = 0; i < values.size(); i++) {
      if (values[i] == currentValue) {
        index = i;
        break;
      }
    }
    if (index == -1) {
      updateComboBox();
    } else {
      input->setCurrentIndex(index);
    }
  }

  QList<QJsonObject> getImageKinds() {
    try {
      auto val = this->node()->getNodeProperty(parentProperty);

      auto obj =
          dynamic_cast<TomographyRawDataNode*>(Node::parseVariantNode(val));
      if (!obj) return {};

      auto data = obj->dataAccessor();
      if (!data) return {};

      return data->availableImageKinds();
    } catch (Exception& e) {
      qWarning() << "Error in ImageKindUI::getImageKinds():" << e.what();
      return {};
    }
  }

  static QString getShortenedName(const QJsonObject& kind, const QString& str) {
    // QString kindStr = QJsonDocument(kind).toJson();
    QString kindStr = str;
    if (kind.contains("Description")) {
      QString descr = kind["Description"].toString();
      // TODO: Make this more sophisticated
      if (descr.length() <= 25) return descr;
      kindStr += ": " + descr.left(15) + "…" + descr.right(7);
    }
    return kindStr;
  }

  void updateComboBox() {
    QJsonObject currentValue;
    try {
      currentValue = getValue();
    } catch (Exception& e) {
      qCritical() << "Error while reading property value:" << e.what();
      return;
    }

    // qDebug() << "updateComboBox" << currentValue << suppressUpdate;

    auto kinds = getImageKinds();

    int index = -1;
    input->clear();
    values.clear();
    for (const auto& kind : kinds) {
      QString kindStr =
          getShortenedName(kind, "Kind " + QString::number(values.size()));
      input->addItem(kindStr, QVariant::fromValue<QJsonObject>(kind));
      if (kind == currentValue) index = values.size();
      values << kind;
    }
    if (currentValue != QJsonObject{} && index == -1) {
      QString kindStr = getShortenedName(currentValue, "Other value");
      input->addItem(kindStr, QVariant::fromValue<QJsonObject>(currentValue));
      index = values.size();
      values << currentValue;
    }
    input->setCurrentIndex(index);
  }

  QWidget* widget() override { return input; }
};

class TomographyRawDataImageListUI
    : public PropertyUIImplBase<vx::types::TomographyRawDataImageList> {
  QComboBox* input;
  bool suppressUpdate = false;
  QSharedPointer<NodeProperty> parentProperty;
  QList<std::tuple<QString, QJsonObject>> values;

 public:
  TomographyRawDataImageListUI(const QSharedPointer<NodeProperty>& property,
                               Node* node)
      : PropertyUIImplBase(property, node) {
    input = new QComboBox();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    const auto& json = property->rawJson();

    QString parentPropertyName = json["ParentProperty"].toString();
    parentProperty = node->prototype()->getProperty(parentPropertyName, false);

    forwardSignalFromPropertyOnReconnect(
        node, parentProperty, &DataNode::dataChangedFinished, this,
        &TomographyRawDataImageListUI::updateComboBox);

    QObject::connect(
        input, (void(QComboBox::*)(int)) & QComboBox::activated, this,
        [this](int index) {
          // qDebug() << "Value in QComboBox changed to" << index;
          auto valueVar = input->itemData(index);
          if (valueVar.userType() !=
              qMetaTypeId<std::tuple<QString, QJsonObject>>()) {
            qWarning() << "Got a " << QMetaType::typeName(valueVar.userType())
                       << " instead of a std::tuple<QString, "
                          "QJsonObject> in QComboBox";
            return;
          }
          auto value = valueVar.value<std::tuple<QString, QJsonObject>>();
          suppressUpdate = true;
          setValueChecked(value);
          suppressUpdate = false;
        });

    updateComboBox();
  }

  bool isMultiline() override { return true; }

  void updateUIValue(
      const std::tuple<QString, QJsonObject>& currentValue) override {
    // qDebug() << "updateUIValue" << currentValue << suppressUpdate;

    if (suppressUpdate) return;

    int index = -1;
    for (int i = 0; i < values.size(); i++) {
      if (values[i] == currentValue) {
        index = i;
        break;
      }
    }
    if (index == -1) {
      updateComboBox();
    } else {
      input->setCurrentIndex(index);
    }
  }

  QList<std::tuple<QString, QJsonObject>> getImageLists() {
    try {
      auto val = this->node()->getNodeProperty(parentProperty);

      auto obj =
          dynamic_cast<TomographyRawDataNode*>(Node::parseVariantNode(val));
      if (!obj) return {};

      auto data = obj->dataAccessor();
      if (!data) return {};

      return data->availableImageLists();
    } catch (Exception& e) {
      qWarning() << "Error in ImageListUI::getImageLists():" << e.what();
      return {};
    }
  }

  static QString getShortenedName(const std::tuple<QString, QJsonObject>& list,
                                  const QString& str) {
    QString listStr = str;
    if (std::get<0>(list) == "" && std::get<1>(list) == QJsonObject{}) {
      return "None";
    } else if (std::get<0>(list) ==
               "de.uni_stuttgart.Voxie.TomographyRawDataImageListType."
               "ImageStream") {
      QString name = std::get<1>(list)["StreamName"].toString();
      // TODO: Make this more sophisticated
      if (name.length() <= 25) return "Image stream \"" + name + "\"";
      listStr +=
          ": Image stream \"" + name.left(17) + "…" + name.right(3) + "\"";
    } else if (std::get<0>(list) ==
               "de.uni_stuttgart.Voxie.TomographyRawDataImageListType."
               "GeometryImageList") {
      QString geometryType = std::get<1>(list)["GeometryType"].toString();
      QJsonArray path = std::get<1>(list)["Path"].toArray();
      // QString name;
      QString name = geometryType;
      for (int i = 0; i < path.count(); i++) {
        // if (i != 0)
        name += " / ";
        if (path[i].isString())
          name += path[i].toString();
        else
          name += QString::number(path[i].toDouble());
      }
      // TODO: Make this more sophisticated
      if (name.length() <= 40) return name;
      listStr += ": " + name.left(30) + "…" + name.right(5) + "\"";
    } else {
      qWarning() << "Unknown TomographyRawDataImageListType:"
                 << std::get<0>(list);
    }
    return listStr;
  }

  void updateComboBox() {
    std::tuple<QString, QJsonObject> currentValue;
    try {
      currentValue = getValue();
    } catch (Exception& e) {
      qCritical() << "Error while reading property value:" << e.what();
      return;
    }

    // qDebug() << "updateComboBox" << currentValue << suppressUpdate;

    auto lists = getImageLists();

    int index = -1;
    input->clear();
    values.clear();
    for (const auto& il : lists) {
      QString ilStr =
          getShortenedName(il, "List " + QString::number(values.size()));
      input->addItem(ilStr,
                     QVariant::fromValue<std::tuple<QString, QJsonObject>>(il));
      if (il == currentValue) index = values.size();
      values << il;
    }
    if (currentValue != std::make_tuple("", QJsonObject{}) && index == -1) {
      QString ilStr = getShortenedName(currentValue, "Other value");
      input->addItem(
          ilStr,
          QVariant::fromValue<std::tuple<QString, QJsonObject>>(currentValue));
      index = values.size();
      values << currentValue;
    }
    input->setCurrentIndex(index);
  }

  QWidget* widget() override { return input; }
};

template <typename TypeInfo>
class NodeReferenceUIBase : public PropertyUIImplBase<TypeInfo> {
 protected:
  QLabel* label;
  QList<QMetaObject::Connection> nameChangeConnectionList;
  QList<QMetaObject::Connection> tagChangeConnectionList;

 public:
  NodeReferenceUIBase(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUIImplBase<TypeInfo>(property, node) {
    label = new QLabel();
    QObject::connect(this, &QObject::destroyed, label, &QObject::deleteLater);
    this->label = new QLabel();
    voxieRoot().connectLinkHandler(this->label);
  }

  virtual void setDependencies(const typename TypeInfo::QtType& value) = 0;

  virtual QString createLabelHTML(const typename TypeInfo::QtType& value) = 0;

  void updateUIValue(const typename TypeInfo::QtType& value) override {
    for (const auto& connection : nameChangeConnectionList)
      QObject::disconnect(connection);
    nameChangeConnectionList.clear();

    for (const auto& connection : tagChangeConnectionList)
      QObject::disconnect(connection);
    tagChangeConnectionList.clear();

    setDependencies(value);

    updateLabel();
  }

  void updateLabel() {
    // qDebug() << "updateLabel()";
    typename TypeInfo::QtType value;
    try {
      value = this->getValue();
    } catch (Exception& e) {
      qCritical() << "Error while reading property value:" << e.what();
      return;
    }
    QString text = createLabelHTML(value);
    this->label->setText(text);
  }

  static QString getShortenedName(Node* obj) {
    QString name = obj->displayName();
    // TODO: Make this more sophisticated
    if (name.length() <= 30) return name;
    return name.left(17) + "…" + name.right(10);
  }

  QWidget* widget() override { return label; }
};

class NodeReferenceUI : public NodeReferenceUIBase<vx::types::NodeReference> {
 public:
  NodeReferenceUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : NodeReferenceUIBase(property, node) {}

  void setDependencies(Node* const& value) override {
    if (value) {
      nameChangeConnectionList
          << QObject::connect(value, &Node::displayNameChanged, this,
                              &NodeReferenceUIBase::updateLabel);
      if (value->nodeKind() == NodeKind::Data) {
        tagChangeConnectionList << QObject::connect(
            dynamic_cast<DataNode*>(value), &DataNode::tagsChanged, this,
            &NodeReferenceUIBase::updateLabel);
      }
    }
  }

  QString createLabelHTML(Node* const& value) override {
    this->label->setToolTip("");
    if (!value) {
      return "-";
    } else {
      auto obj = value;
      auto dataObj = dynamic_cast<DataNode*>(obj);
      QString text;
      QString warnIconString = "";
      if (!dataObj->doTagsMatch(property())) {
        warnIconString = "<html><img src=':/icons/exclamation.png'></html>";
        this->label->setToolTip(
            "Tags do not match\n\nInput Tags: " +
            NodeTag::joinDisplayNames(dataObj->getTags(), ", ") +
            "\nRequired Tags: " +
            NodeTag::joinDisplayNames(property()->inTags(), ", "));
      }
      text.append("<a href=\"" +
                  vx::help::uriForDBusObject(obj).toHtmlEscaped() + "\">" +
                  getShortenedName(obj).toHtmlEscaped() + "</a>" +
                  warnIconString);
      return text;
    }
  }
};

class NodeReferenceListUI
    : public NodeReferenceUIBase<vx::types::NodeReferenceList> {
 public:
  NodeReferenceListUI(const QSharedPointer<NodeProperty>& property, Node* node)
      : NodeReferenceUIBase(property, node) {}

  void setDependencies(const QList<Node*>& value) override {
    for (const auto& obj : value) {
      nameChangeConnectionList
          << QObject::connect(obj, &Node::displayNameChanged, this,
                              &NodeReferenceUIBase::updateLabel);
      if (obj->nodeKind() == NodeKind::Data) {
        tagChangeConnectionList << QObject::connect(
            dynamic_cast<DataNode*>(obj), &DataNode::tagsChanged, this,
            &NodeReferenceUIBase::updateLabel);
      }
    }
  }

  QString createLabelHTML(const QList<Node*>& value) override {
    QString text;
    // bool first = true;
    for (const auto& obj : value) {
      // if (!first)
      //  text.append(", ");
      text.append("<a href=\"" +
                  vx::help::uriForDBusObject(obj).toHtmlEscaped() + "\">" +
                  getShortenedName(obj).toHtmlEscaped() + "</a><br/>");
      // first = false;
    }
    return text;
  }
};

class OutputNodeReferenceUI
    : public NodeReferenceUIBase<vx::types::OutputNodeReference> {
 public:
  OutputNodeReferenceUI(const QSharedPointer<NodeProperty>& property,
                        Node* node)
      : NodeReferenceUIBase(property, node) {}

  void setDependencies(Node* const& value) override {
    if (value) {
      nameChangeConnectionList
          << QObject::connect(value, &Node::displayNameChanged, this,
                              &NodeReferenceUIBase::updateLabel);
      if (value->nodeKind() == NodeKind::Data) {
        tagChangeConnectionList << QObject::connect(
            dynamic_cast<DataNode*>(value), &DataNode::tagsChanged, this,
            &NodeReferenceUIBase::updateLabel);
      }
    }
  }

  QString createLabelHTML(Node* const& value) override {
    if (!value) {
      return "-";
    } else {
      auto obj = value;
      QString text;
      text.append("<a href=\"" +
                  vx::help::uriForDBusObject(obj).toHtmlEscaped() + "\">" +
                  getShortenedName(obj).toHtmlEscaped() + "</a>");
      return text;
    }
  }
};

class PiecewisePolynomialFunctionUI : public PropertyUI {
  QSpinBox* input;

 public:
  PiecewisePolynomialFunctionUI(const QSharedPointer<NodeProperty>& property,
                                Node* object)
      : PropertyUI(property, object) {
    input = new QSpinBox();
    // TODO: fill these UI functions to display PiecewisePolynomialFunction
  }

  QWidget* widget() override { return input; }
};

class ChemicalCompositionUI;
class ChemicalCompositionEditDialog : public QDialog {
  QPlainTextEdit* edit;
  QPushButton* ok;
  // TODO: Use a QPointer?
  ChemicalCompositionUI* ui;

 public:
  ChemicalCompositionEditDialog(ChemicalCompositionUI* ui)
      : QDialog(vx::voxieRoot().mainWindow()), ui(ui) {
    this->resize(500 / 96.0 * this->logicalDpiX(),
                 450 / 96.0 * this->logicalDpiY());
    QVBoxLayout* layout = new QVBoxLayout();
    this->setLayout(layout);

    edit = new QPlainTextEdit("", this);
    layout->addWidget(edit);
    // edit->setReadOnly(true);

    ok = new QPushButton("Ok");
    layout->addWidget(ok);
    QObject::connect(ok, &QPushButton::clicked, this,
                     &ChemicalCompositionEditDialog::editingDone);
  }
  ~ChemicalCompositionEditDialog() {
    // qDebug() << "~ChemicalCompositionEditDialog()";
  }

  void editingDone();

  void setJson(const QJsonArray& json) {
    auto text = QJsonDocument(json).toJson();
    edit->document()->setPlainText(text);
  }
};

class ChemicalCompositionUI
    : public PropertyUIImplBase<vx::types::ChemicalComposition> {
  QWidget* widget_;
  QLineEdit* input;
  QPushButton* button;
  QPointer<ChemicalCompositionEditDialog> dialog;

 public:
  ChemicalCompositionUI(
      const QSharedPointer<PropertyInstance>& propertyInstance)
      : PropertyUIImplBase(propertyInstance) {
    input = new QLineEdit();
    QObject::connect(this, &QObject::destroyed, input, &QObject::deleteLater);

    button = new QPushButton();
    // TODO: Icon
    button->setIcon(QIcon(":/icons/folder-search-result.png"));
    connect(button, &QPushButton::pressed, this, [=]() {
      try {
        // auto comp = ChemicalComposition::parse(this->getValue());
        // auto json = comp->toJson();
        auto json = this->getValue();
        if (!dialog) {
          dialog = new ChemicalCompositionEditDialog(this);
          QObject::connect(dialog, &QDialog::finished, dialog,
                           &QObject::deleteLater);
        } else {
          dialog->show();
          dialog->raise();
        }
        dialog->setJson(vx::expectArray(json));
        dialog->show();
      } catch (vx::Exception& e) {
        vx::showErrorMessage("Got exception while show dialog", e);
      }
    });

    widget_ = new QWidget();
    QObject::connect(this, &QObject::destroyed, widget_, &QObject::deleteLater);

    auto layout = new QHBoxLayout();
    layout->addWidget(input);
    layout->addWidget(button);
    widget_->setLayout(layout);

    input->setReadOnly(true);
  }

  void updateUIValue(const QJsonValue& value) override {
    if (vx::debug_option::Log_Properties_UI()->enabled())
      qDebug() << "Property change of" << property()->name() << "to" << value;
    auto comp = ChemicalComposition::parse(value);
    this->input->setText(comp->toString());
  }

  void setValue(const QJsonValue& value) {
    this->setValueChecked(value);
    this->updateUIValue(value);
  }

  QWidget* widget() override { return widget_; }
};

// TODO
class QuantityUI : public PropertyUIImplBase<vx::types::Quantity> {
 public:
  QLabel* todo;

  QuantityUI(const QSharedPointer<PropertyInstance>& propertyInstance)
      : PropertyUIImplBase(propertyInstance) {
    todo = new QLabel("TODO");
    QObject::connect(this, &QObject::destroyed, todo, &QObject::deleteLater);
  }

  void updateUIValue(const QString& value) override { (void)value; }

  QWidget* widget() override { return todo; }
};

// TODO
class TensorComponentUI
    : public PropertyUIImplBase<vx::types::TensorComponent> {
 public:
  QLabel* todo;

  TensorComponentUI(const QSharedPointer<PropertyInstance>& propertyInstance)
      : PropertyUIImplBase(propertyInstance) {
    todo = new QLabel("TODO");
    QObject::connect(this, &QObject::destroyed, todo, &QObject::deleteLater);
  }

  void updateUIValue(const TensorComponent& value) override { (void)value; }

  QWidget* widget() override { return todo; }
};

void ChemicalCompositionEditDialog::editingDone() {
  try {
    auto text = edit->document()->toPlainText();
    auto json = parseJsonData(text.toUtf8(), "<edit field>");
    auto comp = ChemicalComposition::parse(json.array());
    ui->setValue(json.array());
    this->deleteLater();
  } catch (vx::Exception& e) {
    vx::showErrorMessage("Got exception parsing chemical composition", e);
  }
}

class SeriesEntrySelectionUI
    : public PropertyUIImplBase<vx::types::SeriesEntrySelection> {
 public:
  QSharedPointer<NodeProperty> parentProperty;
  QWidget* widget_;
  QVBoxLayout* layout;
  QList<QSharedPointer<SeriesDimension>> currentDimensions;
  QList<QPointer<QComboBox>> currentDimensionsComboBoxes;
  QList<QWidget*> childWidgets;

  SeriesEntrySelectionUI(const QSharedPointer<NodeProperty>& property,
                         Node* node)
      : PropertyUIImplBase(property, node) {
    const auto& json = property->rawJson();

    QString parentPropertyName = vx::expectString(json["ParentProperty"]);
    parentProperty = node->prototype()->getProperty(parentPropertyName, false);

    forwardSignalFromPropertyOnReconnect(node, parentProperty,
                                         &DataNode::dataChangedFinished, this,
                                         &SeriesEntrySelectionUI::updateUI);

    widget_ = new QWidget();
    layout = new QVBoxLayout();
    widget_->setLayout(layout);
    QObject::connect(this, &QObject::destroyed, widget_, &QObject::deleteLater);

    updateUI();
  }

  bool isMultiline() override { return true; }

  QList<QSharedPointer<SeriesDimension>> getSeriesDimensions() {
    try {
      auto val = this->node()->getNodeProperty(parentProperty);
      auto node = Node::parseVariantNode(val);
      if (!node) return {};

      auto dataNode = dynamic_cast<DataNode*>(node);
      if (!dataNode) {
        qWarning() << "SeriesEntrySelectionUI: Node is not DataNode";
        return {};
      }

      auto data = dataNode->data();
      if (!data) return {};

      auto seriesData = qSharedPointerDynamicCast<SeriesData>(data);
      if (!seriesData) {
        // This will happen e.g. if a normal volume is opened in the
        // SliceVisualizer.
        // qWarning() << "SeriesEntrySelectionUI: Data is not SeriesData";
        return {};
      }

      return seriesData->dimensions();
    } catch (Exception& e) {
      qWarning() << "Error in SeriesEntrySelectionUI::getSeriesDimensions():"
                 << e.what();
      return {};
    }
  }

  void updateUI() {
    try {
      auto dimensions = getSeriesDimensions();
      // qDebug() << "SeriesEntrySelectionUI::updateUI" <<
      // currentDimensions.size()
      //          << dimensions.size();

      bool change = false;
      if (dimensions.size() == currentDimensions.size()) {
        for (int i = 0; i < dimensions.size(); i++)
          if (dimensions[i] != currentDimensions[i]) change = true;
      } else {
        change = true;
      }
      if (!change) return;

      QMap<QString, vx::DBusVariant> currentValues;
      try {
        currentValues = getValue();
      } catch (Exception& e) {
        qCritical() << "SeriesEntrySelectionUI::updateUI: Error while reading "
                       "property value:"
                    << e.what();
        return;
      }

      // qDebug() << "SeriesEntrySelectionUI::updateUI: Updating";

      for (const auto& child : childWidgets) delete child;
      childWidgets.clear();
      currentDimensionsComboBoxes.clear();

      QList<QPointer<QComboBox>> newCurrentDimensionsComboBoxes;
      for (const auto& dimension : dimensions) {
        auto child = new QWidget();
        auto hbox = new QHBoxLayout();
        child->setLayout(hbox);

        auto label = new QLabel(dimension->property()->displayName());
        hbox->addWidget(label);

        bool haveValue = currentValues.contains(dimension->property()->name());
        vx::Any currentValue;
        if (haveValue) {
          // TODO: Error handling (values where conversion fails probably should
          // be ignored)
          // currentValue = dimension->property()->type()->dbusToRaw(
          currentValue = dimension->property()->type()->dbusVoxieToRaw(
              getQMapValue(currentValues, dimension->property()->name()));
        }
        // qDebug() << haveValue << currentValue;

        auto box = new QComboBox();
        hbox->addWidget(box);
        int index = -1;
        for (int i = 0; i < dimension->entries().size(); i++) {
          auto entry = dimension->entries()[i];
          QString desc = vx::format(
              "{}: {}", i, dimension->property()->type()->valueToString(entry));
          box->addItem(desc, QVariant::fromValue<int>(i));

          if (index == -1) {
            if (haveValue) {
              if (dimension->property()->type()->equals(currentValue, entry))
                index = i;
            } else {
              // Select the first value if this is a new dimension
              index = i;
            }
          }
        }
        if (index == -1) {
          // TODO: Remove this after another element was selected? Or just set
          // index to 0?
          index = dimension->entries().size();
          box->addItem("<Unknown>", QVariant::fromValue<int>(index));
        }
        box->setCurrentIndex(index);

        QObject::connect(
            box, (void(QComboBox::*)(int)) & QComboBox::activated, this,
            [this, box, dimension](int newIndex) {
              // qDebug() << "Value in QComboBox changed to" << newIndex;
              if (!this->node()) return;
              try {
                auto valueVar = box->itemData(newIndex);
                if (valueVar.userType() != qMetaTypeId<int>())
                  throw Exception("de.uni_stuttgart.Voxie.Error",
                                  QString() + "Got a " +
                                      QMetaType::typeName(valueVar.userType()) +
                                      " instead of a int in QComboBox");
                auto value = valueVar.value<int>();
                this->updateFromUI(dimension, value);
              } catch (Exception& e) {
                qCritical()
                    << "Error while updating property value : " << e.what();
              }
            });

        // TODO: Clean up a lot of stuff, e.g. handling of different dimensions,
        // where the mapping between combo box index and data is stored, etc.

        auto backwardButton =
            new QPushButton(QIcon(":/icons-voxie/skip-previous.png"), "");
        hbox->addWidget(backwardButton);
        QObject::connect(backwardButton, &QPushButton::clicked,
                         // TODO: Make sure that box is not destroyed?
                         this, [this, box, dimension]() {
                           // qDebug() << "Backward" << box <<
                           // box->currentIndex();
                           int newIndex = box->currentIndex() - 1;
                           if (newIndex < 0) {
                             return;
                           }
                           // qDebug() << "Setting" <<
                           // dimension->property()->name() << "to" << newIndex;
                           try {
                             updateFromUI(dimension, newIndex, false);
                           } catch (Exception& e) {
                             qCritical()
                                 << "Error while updating property value : "
                                 << e.what();
                           }
                         });

        auto playButton = new QPushButton(QIcon(":/icons-voxie/play.png"), "");
        hbox->addWidget(playButton);
        auto running = createQSharedPointer<bool>(false);
        // TODO: Clean up lifetime of timer? Currently timer is kept alive by
        // the reference from the playButton lambda.
        auto timer = makeSharedQObject<QTimer>();
        vx::debug_option::SeriesEntrySelection_SlideshowTime()
            ->registerAndCallChangeHandler(timer.data(), [timer](double time) {
              qDebug() << "Setting timer to" << time;
              timer->setInterval(time * 1000);
            });
        QObject::connect(
            playButton, &QPushButton::clicked, this,
            [this, playButton, running, timer]() {
              if (*running) {
                playButton->setIcon(QIcon(":/icons-voxie/play.png"));
                *running = false;
                timer->stop();
              } else {
                playButton->setIcon(QIcon(":/icons-voxie/pause.png"));
                *running = true;
                timer->start();
              }
            });
        // TODO: Make sure that box and playButton is not destroyed?
        QObject::connect(
            timer.data(), &QTimer::timeout, this,
            [this, box, dimension, running, timer, playButton]() {
              // qDebug() << "Timer forward" << box << box->currentIndex();
              int newIndex = box->currentIndex() + 1;
              if (newIndex >= box->count()) {
                // TODO: Clean up
                // TODO: Do the stop earlier?
                playButton->setIcon(QIcon(":/icons-voxie/play.png"));
                *running = false;
                timer->stop();
                return;
              }
              // qDebug() << "Setting" << dimension->property()->name() << "to"
              //          << newIndex;
              try {
                updateFromUI(dimension, newIndex, false);
              } catch (Exception& e) {
                qCritical()
                    << "Error while updating property value : " << e.what();
              }
            });

        auto forwardButton =
            new QPushButton(QIcon(":/icons-voxie/skip-next.png"), "");
        hbox->addWidget(forwardButton);
        QObject::connect(forwardButton, &QPushButton::clicked,
                         // TODO: Make sure that box is not destroyed?
                         this, [this, box, dimension]() {
                           // qDebug() << "Forward" << box <<
                           // box->currentIndex();
                           int newIndex = box->currentIndex() + 1;
                           if (newIndex >= box->count()) {
                             return;
                           }
                           // qDebug() << "Setting" <<
                           // dimension->property()->name() << "to"
                           //          << newIndex;
                           try {
                             updateFromUI(dimension, newIndex, false);
                           } catch (Exception& e) {
                             qCritical()
                                 << "Error while updating property value : "
                                 << e.what();
                           }
                         });

        childWidgets << child;
        newCurrentDimensionsComboBoxes << box;
        layout->addWidget(child);
      }

      currentDimensions = dimensions;
      currentDimensionsComboBoxes = newCurrentDimensionsComboBoxes;
    } catch (Exception& e) {
      qWarning() << "Error in SeriesEntrySelectionUI::updateUI():" << e.what();
    }
  }

  // Can throw
  // TODO: Rename?
  void updateFromUI(const QSharedPointer<SeriesDimension>& dimension, int value,
                    bool isUpdateFromUI = true) {
    // qDebug() << "Update" << dimension->property()->name() << "to" << value;

    // - Set dimension to value if value is valid
    // - Keep all dimensions in currentDimensions
    // - Drop all other dimensions

    auto current = getValue();
    QMap<QString, vx::DBusVariant> newValues;
    for (const auto& dim : currentDimensions) {
      if (dim == dimension && value >= 0 &&
          value < dimension->entries().size()) {
        newValues.insert(dim->property()->name(),
                         // dimension->property()->type()->rawToDBus(
                         dimension->property()->type()->rawToDBusVoxie(
                             dimension->entries()[value]));
      } else if (current.contains(dim->property()->name())) {
        newValues.insert(dim->property()->name(),
                         getQMapValue(current, dim->property()->name()));
      }
    }
    // qDebug() << vx::toString(newValues);
    setValueChecked(newValues, isUpdateFromUI);
  }

  // Can throw
  // void updateUIValue(const SeriesEntrySelection& value) override {
  void updateUIValue(const QMap<QString, vx::DBusVariant>& newValues) override {
    // TODO: To some extent copied from updateUI(), should probably be merged
    for (int dimIndex = 0; dimIndex < currentDimensions.size(); dimIndex++) {
      const auto& dimension = currentDimensions[dimIndex];
      bool haveValue = newValues.contains(dimension->property()->name());
      vx::Any currentValue;
      if (haveValue) {
        // TODO: Error handling (values where conversion fails probably should
        // be ignored)
        // currentValue = dimension->property()->type()->dbusToRaw(
        currentValue = dimension->property()->type()->dbusVoxieToRaw(
            getQMapValue(newValues, dimension->property()->name()));
      }
      // qDebug() << haveValue << currentValue;

      if (dimIndex >= currentDimensionsComboBoxes.size()) {
        qWarning() << "dimIndex >= currentDimensionsComboBoxes.size()";
        continue;
      }
      QComboBox* box = currentDimensionsComboBoxes[dimIndex];
      if (!box) {
        qWarning() << "currentDimensionsComboBoxes[dimIndex] is nullptr";
        continue;
      }

      int index = -1;
      for (int i = 0; i < dimension->entries().size(); i++) {
        auto entry = dimension->entries()[i];

        if (index == -1) {
          if (haveValue) {
            if (dimension->property()->type()->equals(currentValue, entry))
              index = i;
          } else {
            // Select the first value if this is a new dimension
            index = i;
          }
        }
      }
      if (index == -1) {
        // TODO: Remove this after another element was selected? Or just set
        // index to 0?
        index = dimension->entries().size();
        if (box->count() == index)
          box->addItem("<Unknown>", QVariant::fromValue<int>(index));
      }
      box->setCurrentIndex(index);
    }
  }

  QWidget* widget() override { return widget_; }
};
}  // namespace

template <typename T>
static int
defaultValueCompare(  // const QSharedPointer<NodeProperty>& property,
    const T& v1, const T& v2) {
  //(void)property;

  bool v1IsNan = !(v1 == v1);
  bool v2IsNan = !(v2 == v2);
  if (v1IsNan && v2IsNan)
    // nan == nan
    return 0;
  // nan is smaller than everything else
  else if (v1IsNan)
    return -1;
  else if (v2IsNan)
    return 1;

  if (v1 < v2)
    return -1;
  else if (v2 < v1)
    return 1;
  else
    return 0;
}

namespace vx {
namespace {
template <typename T>
T parseNotSupported(const QJsonValue& value) {
  Q_UNUSED(value);
  throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                  QString() + "Parsing a " +
                      QMetaType::typeName(qMetaTypeId<T>()) +
                      " from JSON not supported");
}

// TODO: Should this use DBusAsJSON instead?
template <typename T>
struct ParseJsonFun;

template <typename T>
T parseJson(const QJsonValue& value) {
  return ParseJsonFun<T>::parse(value);
}

template <>
struct ParseJsonFun<double> {
  static double parse(const QJsonValue& value) {
    // TODO: Move to vx::expectDouble()?
    if (value.isString() && value.toString() == "NaN")
      return std::numeric_limits<double>::quiet_NaN();
    if (value.isString() && value.toString() == "Infinity")
      return std::numeric_limits<double>::infinity();
    if (value.isString() && value.toString() == "-Infinity")
      return -std::numeric_limits<double>::infinity();
    return vx::expectDouble(value);
  }
};

/*
template <>
struct ParseJsonFun<int> {
  static int parseInt(const QJsonValue &value) {
if (!value.isDouble())
  throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                           "JSON value is not a number");
auto val = value.toInt(0);
auto val1 = value.toInt(1);
if (val != val1)
  throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                           "JSON value is not an integer");
return val;
}
};
*/

template <>
struct ParseJsonFun<qint32> {
  static qint32 parse(const QJsonValue& value) {
    if (!value.isDouble())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not a number");
    auto val = value.toDouble();
    if (val < std::numeric_limits<qint32>::min())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is too small");
    if (val > std::numeric_limits<qint32>::max())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is too large");
    return (qint32)val;
  }
};

template <>
struct ParseJsonFun<quint32> {
  static quint32 parse(const QJsonValue& value) {
    if (!value.isDouble())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not a number");
    auto val = value.toDouble();
    if (val < 0)
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is negative");
    if (val > std::numeric_limits<quint32>::max())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is too large");
    return (quint32)val;
  }
};

template <>
struct ParseJsonFun<quint64> {
  static quint64 parse(const QJsonValue& value) {
    if (!value.isDouble())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not a number");
    auto val = value.toDouble();
    if (val < 0)
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is negative");
    // TODO: Large values will be rounded during JSON parsing. Avoid that
    // somehow?
    return (quint64)val;
  }
};

template <>
struct ParseJsonFun<qint64> {
  static qint64 parse(const QJsonValue& value) {
    if (!value.isDouble())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not a number");
    auto val = value.toDouble();
    // TODO: Large values will be rounded during JSON parsing. Avoid that
    // somehow?
    return (qint64)val;
  }
};

template <>
struct ParseJsonFun<bool> {
  static bool parse(const QJsonValue& value) {
    if (!value.isBool())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not a boolean");
    return value.toBool();
  }
};

template <>
struct ParseJsonFun<QString> {
  static QString parse(const QJsonValue& value) {
    if (!value.isString())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not a string");
    return value.toString();
  }
};

template <typename T>
struct ParseJsonFun<QList<T>> {
  static QList<T> parse(const QJsonValue& value) {
    if (!value.isArray())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not an array");
    QJsonArray array = value.toArray();
    QList<T> result;
    for (const auto& member : array) {
      result << parseJson<T>(member);
    }
    return result;
  }
};

template <typename... T>
struct ParseJsonFun<std::tuple<T...>> {
  template <size_t i>
  static void set(std::tuple<T...>& res, const QJsonArray& array,
                  std::integral_constant<size_t, i>) {
    static_assert(i <= sizeof...(T), "i <= sizeof...(T)");
    static_assert(i > 0, "i > 0");

    set(res, array, std::integral_constant<size_t, i - 1>());

    std::get<i - 1>(res) =
        parseJson<typename std::tuple_element<i - 1, std::tuple<T...>>::type>(
            array[i - 1]);
  }

  static void set(std::tuple<T...>& res, const QJsonArray& array,
                  std::integral_constant<size_t, 0>) {
    Q_UNUSED(res);
    Q_UNUSED(array);
  }

  static std::tuple<T...> parse(const QJsonValue& value) {
    auto array = expectArray(value);

    if (!value.isArray())
      if (array.size() != sizeof...(T))
        throw Exception(
            "de.uni_stuttgart.Voxie.InvalidJsonType",
            vx::format("JSON array does not contain {} entries", sizeof...(T)));

    std::tuple<T...> res;
    set(res, array, std::integral_constant<size_t, sizeof...(T)>());
    return res;
  }
};

template <typename T>
struct ParseJsonFun<std::tuple<T, T, T>> {
  static vx::TupleVector<T, 3> parse(const QJsonValue& value) {
    if (!value.isArray())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not an array");
    QJsonArray array = value.toArray();
    if (array.size() != 3)
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON array does not contain 3 entries");
    return vx::TupleVector<T, 3>(parseJson<T>(array[0]), parseJson<T>(array[1]),
                                 parseJson<T>(array[2]));
  }
};

template <typename T>
struct ParseJsonFun<std::tuple<T, T, T, T>> {
  static vx::TupleVector<T, 4> parse(const QJsonValue& value) {
    if (!value.isArray())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not an array");
    QJsonArray array = value.toArray();
    if (array.size() != 4)
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON array does not contain 4 entries");
    return vx::TupleVector<T, 4>(parseJson<T>(array[0]), parseJson<T>(array[1]),
                                 parseJson<T>(array[2]),
                                 parseJson<T>(array[3]));
  }
};

// TODO: Make this independent of the number of parameters, avoid special case
// when all the parameters are the same?
template <typename T, typename U, typename V>
struct ParseJsonFun<std::tuple<T, U, V>> {
  static std::tuple<T, U, V> parse(const QJsonValue& value) {
    if (!value.isArray())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not an array");
    QJsonArray array = value.toArray();
    if (array.size() != 3)
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON array does not contain 3 entries");
    return std::tuple<T, U, V>(parseJson<T>(array[0]), parseJson<U>(array[1]),
                               parseJson<V>(array[2]));
  }
};

template <typename T>
struct ParseJsonFun<QMap<QString, T>> {
  static QMap<QString, T> parse(const QJsonValue& value) {
    if (!value.isObject())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not an object");
    QJsonObject object = value.toObject();
    QMap<QString, T> result;
    for (const auto& key : object.keys()) {
      result.insert(key, parseJson<T>(object[key]));
    }
    return result;
  }
};

template <>
struct ParseJsonFun<QJsonObject> {
  static QJsonObject parse(const QJsonValue& value) {
    if (!value.isObject())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not an object");
    return value.toObject();
  }
};

template <>
struct ParseJsonFun<QJsonValue> {
  static QJsonValue parse(const QJsonValue& value) { return value; }
};

/*
template <>
struct ParseJsonFun<QDBusVariant> {
static QDBusVariant parse(const QJsonValue& value) {
  return vx::jsonToDBus(value);
}
};
*/

template <>
struct ParseJsonFun<vx::DBusVariant> {
  static vx::DBusVariant parse(const QJsonValue& value) {
    return vx::jsonToDBusVoxie(value);
  }
};

template <>
struct ParseJsonFun<std::tuple<QString, QJsonObject>> {
  static std::tuple<QString, QJsonObject> parse(const QJsonValue& value) {
    if (!value.isArray())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not an array");
    QJsonArray array = value.toArray();
    if (array.size() != 2)
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON array does not contain 3 entries");
    if (!array[0].isString())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not a string");
    if (!array[1].isObject())
      throw Exception("de.uni_stuttgart.Voxie.InvalidJsonType",
                      "JSON value is not an object");
    return std::make_tuple(array[0].toString(), array[1].toObject());
  }
};

template <typename T>
QString valueToString(const vx::Any& value,
                      const vx::PropertyType* type = nullptr) {
  // Check if the type contained in the vx::Any matches the template argument
  if (value.type() != typeid(T)) {
    throw Exception(
        "de.uni_stuttgart.Voxie.InvalidStringConversionType",
        QStringLiteral("Type mismatch during string conversion: Raw type '") +
            vx::getTypeNameString<T>() + "' does not match variant type '" +
            vx::toString(value.type()) + "'");
  } else if (StringConversionFallbackHelper<T>::isFallback && type != nullptr) {
    // No toString available? Convert to type name instead, if given
    return type->displayName();
  } else {
    // Use string conversion specialization
    return StringConversionHelper<T>::toString(vx::any_cast<T>(value));
  }
}

// TODO: Remove? (Needed by QVariant for TableData)
template <typename T>
QString valueToStringQVariant(const QVariant& value,
                              const vx::PropertyType* type = nullptr) {
  // Check if the type contained in the QVariant matches the template argument
  if (!value.canConvert<T>()) {
    throw Exception(
        "de.uni_stuttgart.Voxie.InvalidStringConversionType",
        QStringLiteral("Type mismatch during string conversion: Raw type '") +
            typeid(T).name() + "' does not match variant type '" +
            value.typeName() + "'");
  } else if (StringConversionFallbackHelper<T>::isFallback && type != nullptr) {
    // No toString available? Convert to type name instead, if given
    return type->displayName();
  } else {
    // Use string conversion specialization
    return StringConversionHelper<T>::toString(value.value<T>());
  }
}

QString piecewisePolynomialFunctionToString(
    const std::tuple<QList<std::tuple<double, std::tuple<double, double>>>,
                     QList<QList<double>>>& raw) {
  return vx::PiecewisePolynomialFunction::parse(raw).toString();
}

QString piecewisePolynomialFunctionGetDescription(
    const std::tuple<QList<std::tuple<double, std::tuple<double, double>>>,
                     QList<QList<double>>>& raw) {
  return vx::PiecewisePolynomialFunction::parse(raw).description();
}

QString chemicalCompositionToString(const QJsonValue& json) {
  return vx::ChemicalComposition::parse(json)->toString();
}

QString chemicalCompositionGetDescription(const QJsonValue& json) {
  return vx::ChemicalComposition::parse(json)->description();
}

void verifyEnum(NodeProperty& property, const QString& value) {
  for (const auto& entry : property.enumEntries()) {
    if (value == entry.name()) return;
    // Note: Don't consider compatibility names
    for (const auto& compat : entry.compatibilityNames()) {
      if (value == compat)
        throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                        "Value '" + value +
                            "' is not a non-canonical form of '" +
                            entry.name() + "' for property " + property.name());
    }
  }
  throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                  "Value '" + value +
                      "' is not a valid enum entry for property " +
                      property.name());
}
QString canonicalizeEnum(NodeProperty& property, const QString& value) {
  // TODO: speed this up?
  for (const auto& entry : property.enumEntries()) {
    if (value == entry.name()) return entry.name();
    for (const auto& compat : entry.compatibilityNames()) {
      if (value == compat) return entry.name();
    }
  }
  throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                  "Value '" + value +
                      "' is not a valid enum entry for property " +
                      property.name());
}
/*
static EnumEntry getEnumEntry(const QSharedPointer<NodeProperty>& property,
                              const QString& name) {
  for (const auto& entry : property->enumEntries()) {
    if (name == entry.name()) return entry;
    // Note: Don't consider compatibility names
  }
  throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                  "Value '" + name +
                      "' is not a valid enum entry for property " +
                      property->name());
}
int compareEnum(const QSharedPointer<NodeProperty>& property, const QString& v1,
              const QString& v2) {
auto e1 = getEnumEntry(property, v1);
auto e2 = getEnumEntry(property, v2);

int res;

res = defaultValueCompare(property, e1.uiPosition(), e2.uiPosition());
if (res) return res;

res = defaultValueCompare(property, e1.name(), e2.name());

return res;
}
*/

void verifyDataType(const std::tuple<QString, quint32, QString>& value) {
  try {
    vx::parseDataTypeStruct(value);
  } catch (Exception& e) {
    throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                    "Value ('" + std::get<0>(value) + "', " +
                        QString::number(std::get<1>(value)) + ", '" +
                        std::get<2>(value) +
                        "' is not a valid data type: " + e.message());
  }
}

void verifyValueColorMapping(
    const QList<std::tuple<double, vx::TupleVector<double, 4>, int>>& value) {
  double lastVal = -std::numeric_limits<double>::infinity();
  bool isFirst = true;
  for (const auto& entry : value) {
    auto v = std::get<0>(entry);
    if (std::isnan(v)) {
      if (!isFirst)
        throw Exception(
            "de.uni_stuttgart.Voxie.InvalidPropertyValue",
            "ValueColorMapping has NaN value at other than first position");
    } else {
      // Currently the same value multiple times is allowed
      if (v < lastVal)
        throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                        "ValueColorMapping values not sorted");
      lastVal = v;
    }

    auto c = std::get<1>(entry);
    if (std::get<0>(c) < 0 || std::get<0>(c) > 1)
      throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                      "ValueColorMapping has invalid R value");
    if (std::get<1>(c) < 0 || std::get<1>(c) > 1)
      throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                      "ValueColorMapping has invalid G value");
    if (std::get<2>(c) < 0 || std::get<2>(c) > 1)
      throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                      "ValueColorMapping has invalid B value");
    if (std::get<3>(c) < 0 || std::get<3>(c) > 1)
      throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                      "ValueColorMapping has invalid A value");

    int interpolator = std::get<2>(entry);
    if (interpolator < 0 ||
        interpolator >= ColorInterpolator::InterpolationTypeCount) {
      throw Exception("de.uni_stuttgart.Voxie.InvalidPropertyValue",
                      "ValueColorMapping has invalid interpolator");
    }

    isFirst = false;
  }
}
}  // namespace
}  // namespace vx

// TODO: Remove? (Needed by QVariant for TableData)
template <typename T>
static T parseQVariant(const QVariant& variant) {
  if (variant.userType() != qMetaTypeId<T>())
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    QString() + "Got invalid type: Expected " +
                        QMetaType::typeName(qMetaTypeId<T>()) + ", got " +
                        QMetaType::typeName(variant.userType()));
  return variant.value<T>();
}

#include "Types.List.cpp"

QList<QSharedPointer<Component>> vx::allTypesAsComponents() {
  return {LIST_ALL_TYPES};
}
