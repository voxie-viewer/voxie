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

#include "SegmentationStep.hpp"

using namespace vx;

SegmentationStep::SegmentationStep(
    QString prototypeName, const QSharedPointer<NodePrototype>& prototype)
    : Node(prototypeName, prototype) {
  this->setGraphVisibility(false);

  this->initializeRunStepButton();
  propSection = new PropertySection("Segmentation Step Properties");
}

QSharedPointer<SegmentationStep> SegmentationStep::clone(bool registerNode) {
  QMap<QString, QVariant> map = QMap<QString, QVariant>();
  for (const auto& property : this->prototype()->nodeProperties())
    map[property->name()] = this->getNodeProperty(property);

  return qSharedPointerDynamicCast<SegmentationStep>(this->prototype()->create(
      map, QList<Node*>(), QMap<QString, QDBusVariant>(), registerNode));
}

void SegmentationStep::initializeRunStepEnabledCondition() {
  QJsonObject json = this->prototype()->rawJson();
  if (json.contains("RunSegmentationStepEnabledCondition")) {
    this->runStepEnabledCondition_ = PropertyCondition::parse(
        this->prototype(),
        json["RunSegmentationStepEnabledCondition"].toObject());
  } else {
    this->runStepEnabledCondition_ =
        createQSharedPointer<PropertyConditionTrue>();
  }
}

void SegmentationStep::initializeRunStepButton() {
  this->runStepButton = new QPushButton("Run Step");
  this->runStepButton->setToolTip(QString("Run & append step to history"));

  this->initializeRunStepEnabledCondition();

  // Update calculate enabled condition if properties/ dependencies change
  QSet<NodeProperty*> deps;

  this->getRunStepEnabledCondition()->collectDependencies(deps);
  for (const auto& dep : deps) {
    connect(this, &Node::propertyChanged, this,
            [this, dep](const QSharedPointer<NodeProperty>& property,
                        const QVariant& value) {
              Q_UNUSED(value);
              if (property != dep) return;
              this->updateRunStepButtonState();
            });
  }

  this->updateRunStepButtonState();
}

void SegmentationStep::updateRunStepButtonState() {
  bool enable = this->getRunStepEnabledCondition()->evaluate(this);

  // TODO: disable button while a process is running
  /*
  // TODO: Make sure this is updated when a process is started?
  if (this->process && this->process.data()->state() != QProcess::Running) {
    enable = false;
  }
  */
  this->runStepButton->setEnabled(enable);
}

void SegmentationStep::fillPropertySection(LabelViewModel* labelViewModel) {
  this->setLabelViewModel(labelViewModel);

  if (this->prototype()->rawJson().contains("UI")) {
    QJsonArray UIEntries = this->prototype()
                               ->rawJson()["UI"]
                               .toObject()["SegmentationStepUIEntries"]
                               .toArray();

    // Go over Each UI entry
    for (QJsonValueRef UIEntry : UIEntries) {
      auto spSection = UIEntry.toObject();

      // create a own property section for each UI Element
      // QString displayName = spSection["DisplayName"].toString();
      // auto newSection = new PropertySection(displayName);

      QString type = spSection["Type"].toString();

      // Use custom UI
      if (type == "Custom") {
        QString propertyNameCustom = spSection["Name"].toString();
        auto customPropertySection =
            this->getCustomPropertySectionContent(propertyNameCustom);
        this->propSection->addProperty(customPropertySection);

      }  // Use default UI
      else if (type == "Property") {
        // Add default propertyUI to propertySection
        QString propertyName = spSection["Property"].toString();
        QSharedPointer<NodeProperty> property =
            this->prototype()->getProperty(propertyName, false);

        addNodePropertyUIToSection(this, this->propSection, property,
                                   QJsonObject());
      }
    }
  }

  this->addPropertySection(this->propSection);
}

StepKind SegmentationStep::getStepKind() {
  if (this->prototype()->rawJson().contains("StepKind")) {
    QString kind = this->prototype()->rawJson()["StepKind"].toString();

    if (kind == "SelectionStep")
      return StepKind::SelectionStep;
    else if (kind == "MetaStep")
      return StepKind::MetaStep;
    else if (kind == "LabelStep")
      return StepKind::LabelStep;
    else if (kind == "ExtensionStep")
      return StepKind::ExtensionStep;
    else
      return StepKind::None;
  }
  return StepKind::None;
}

QSharedPointer<OperationResult> SegmentationStep::runThreaded(
    const QString& description,
    std::function<void(const QSharedPointer<vx::io::Operation>&)> f) {
  return vx::io::Operation::runThreaded<vx::OperationSimple>(
      description,
      [fun = std::move(f)](const QSharedPointer<vx::io::Operation>& op) {
        fun(op);
        return createQSharedPointer<vx::io::Operation::ResultSuccess>();
      });
}

SegmentationStep::~SegmentationStep() {}

QMap<QString, QString> SegmentationStep::getExtensionInfo() { return {}; }
