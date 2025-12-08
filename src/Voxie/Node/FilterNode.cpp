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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "FilterNode.hpp"

#include <Voxie/Component/HelpCommon.hpp>

#include <QtWidgets/QFormLayout>

#include <VoxieClient/DBusAdaptors.hpp>

#include <VoxieClient/ObjectExport/Client.hpp>

#include <VoxieBackend/IO/OperationResult.hpp>

#include <Voxie/DebugOptions.hpp>

#include <Voxie/Data/VolumeNode.hpp>

#include <Voxie/Node/FilterNode.hpp>
#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/WeakParameterCopy.hpp>

#include <Voxie/PropertyObjects/PreviewBoxNode.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <string>

using namespace vx;

namespace vx {
namespace {
class FilterNodeAdaptorImpl : public FilterNodeAdaptor,
                              public FilterObjectAdaptor {
  FilterNode* object;

 public:
  FilterNodeAdaptorImpl(FilterNode* object)
      : FilterNodeAdaptor(object),
        FilterObjectAdaptor(object),
        object(object) {}
  ~FilterNodeAdaptorImpl() override {}

  bool previewActive() const override { return object->previewBox()->active(); }

  vx::TupleVector<double, 4> previewPoint() const override {
    for (Node* parentNode : object->parentNodes()) {
      if (VolumeNode* input = dynamic_cast<VolumeNode*>(parentNode)) {
        auto voxelData =
            qSharedPointerDynamicCast<VolumeDataVoxel>(input->volumeData());
        if (voxelData) {
          auto size = (object->previewBox()->origin() - voxelData->origin()) /
                      voxelData->getSpacing();
          return vx::TupleVector<double, 4>(round(size.x()), round(size.y()),
                                            round(size.z()), 0);
        }
        // TODO: Non-voxel datasets?
      }
    }
    // TODO: Handle better.
    return vx::TupleVector<double, 4>(0, 0, 0, 0);
  }

  QDBusObjectPath RunFilter(
      const QDBusObjectPath& client,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      Client* clientPtr =
          qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
      if (!clientPtr) {
        throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                        "Cannot find client object");
      }

      auto op = object->run(false);
      auto result = OperationResult::create(op);

      clientPtr->incRefCount(result);
      return ExportedObject::getPath(result);
    } catch (Exception& e) {
      return e.handle(object);
    }
  }
};
}  // namespace
}  // namespace vx

FilterNode::FilterNode(const QSharedPointer<NodePrototype>& prototype)
    : Node("FilterNode", prototype) {
  new FilterNodeAdaptorImpl(this);

  if (voxieRoot().isHeadless()) return;

  auto mainVBoxLayout = new QVBoxLayout();

  this->debuggerSupportEnabled = new QCheckBox("Enable debugger support");
  mainVBoxLayout->addWidget(this->debuggerSupportEnabled);
  this->debuggerSupportEnabled->hide();

  auto CalculateButtonLayout = new QHBoxLayout();
  mainVBoxLayout->addLayout(CalculateButtonLayout);
  CalculateButtonLayout->setContentsMargins(0, 0, 0, 0);
  this->calculateButton = new QPushButton("Calculate");
  this->calculateButton->setEnabled(false);
  CalculateButtonLayout->addWidget(this->calculateButton);
  this->stopProcessButton = new QToolButton();
  this->stopProcessButton->setIcon(QIcon(":/icons/cross.png"));
  this->stopProcessButton->setToolTip("Stop process");
  this->stopProcessButton->setEnabled(false);
  CalculateButtonLayout->addWidget(this->stopProcessButton);

  this->displayLabel = new QLabel();
  voxieRoot().connectLinkHandler(this->displayLabel);
  mainVBoxLayout->addWidget(displayLabel);

  this->mainPropertyWidget = new QWidget();
  this->mainPropertyWidget->setWindowTitle("Filter");
  this->mainPropertyWidget->setLayout(mainVBoxLayout);
  this->addPropertySection(this->mainPropertyWidget);

  connect(this, &Node::displayNameChanged, this->calculateButton,
          &QPushButton::setWindowTitle);
  connect(this, &Node::parentChanged, this, [&](Node* node) {
    (void)node;
    this->previewBox_ = new PreviewBox(QVector3D(), QVector3D(), false);
    for (Node* parentNode : this->parentNodes()) {
      if (PreviewBoxNode* previewNode =
              dynamic_cast<PreviewBoxNode*>(parentNode)) {
        this->previewBox_ = previewNode->getPreviewBox();
      }
    }
  });
  connect(this->calculateButton, &QPushButton::released, this,
          [this]() { this->run(); });
  connect(this, &Node::parentChanged, this,
          &FilterNode::displayLabelParentChildChanged);
  connect(this, &Node::childChanged, this,
          &FilterNode::displayLabelParentChildChanged);

  QSet<NodeProperty*> deps;
  this->prototype()->runFilterEnabledCondition()->collectDependencies(deps);
  for (const auto& dep : deps) {
    connect(this, &Node::propertyChanged, this,
            [this, dep](const QSharedPointer<NodeProperty>& property,
                        const vx::Any& value) {
              Q_UNUSED(value);
              if (property != dep) return;
              updateCalculateButtonState();
            });
  }
  updateCalculateButtonState();

  // TODO: Add runFilterAutomatically property to NodePrototype
  // TODO: Make this more configurable, allow changing this in the UI?
  if (this->prototype()->rawJson()["RunFilterAutomatically"].toBool()) {
    connect(this, &Node::propertyChanged, this,
            &FilterNode::triggerAutomaticFilterRun);
  }
}

FilterNode::~FilterNode() {
  // TODO: Clean this up
  // This is needed because otherwise displayLabelParentChildChanged() might be
  // called during the Node destructor (which will cause a segmentation fault
  // because displayLabelConnections is already destroyed).
  QObject::disconnect(this, nullptr, this, nullptr);
}

QList<QString> FilterNode::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.FilterNode",
  };
}

bool FilterNode::isAllowedChild(NodeKind kind) {
  return kind == NodeKind::Data;
}

bool FilterNode::isAllowedParent(NodeKind kind) {
  return kind == NodeKind::Data;
}

bool FilterNode::isCreatableChild(NodeKind) { return false; }

void FilterNode::updateCalculateButtonState() {
  bool enable = prototype()->runFilterEnabledCondition()->evaluate(this);

  // TODO: disable button while a process is running
  /*
  // TODO: Make sure this is updated when a process is started?
  if (this->process && this->process.data()->state() != QProcess::Running) {
    enable = false;
  }
  */
  this->calculateButton->setEnabled(enable);
}

void FilterNode::displayLabelParentChildChanged() {
  // qDebug() << "FilterNode::displayLabelParentChildChanged()";
  for (const auto& connection : displayLabelConnections) {
    QObject::disconnect(connection);
  }
  displayLabelConnections.clear();

  for (const auto& obj : this->parentNodes())
    displayLabelConnections << QObject::connect(
        obj, &Node::displayNameChanged, this, &FilterNode::updateDisplayLabel);
  for (const auto& obj : this->childNodes())
    displayLabelConnections << QObject::connect(
        obj, &Node::displayNameChanged, this, &FilterNode::updateDisplayLabel);

  updateDisplayLabel();
}

void FilterNode::updateDisplayLabel() {
  // qDebug() << "FilterNode::updateDisplayLabel()";
  QString displayText;
  for (Node* parentNode : this->parentNodes()) {
    displayText.append("Parent: <a href=\"" +
                       vx::help::uriForDBusObject(parentNode).toHtmlEscaped() +
                       "\">" + parentNode->displayName() + "</a> <br/>");
  }
  for (Node* childNode : this->childNodes()) {
    displayText.append("Child: <a href=\"" +
                       vx::help::uriForDBusObject(childNode).toHtmlEscaped() +
                       "\">" + childNode->displayName() + "</a> <br/>");
  }
  this->displayLabel->setText(displayText);
}

QSharedPointer<vx::io::RunFilterOperation> FilterNode::run(
    bool isAutomaticFilterRun) {
  // create a copy of the current parameters that we will use to later know if
  // parameters changed and that we can also pass to the calculate() method for
  // the filter to use
  auto currentRunParameters =
      ParameterCopy::getParameters(this)->createWeakCopy();

  // Should this set runParameters to nullptr? Should not be needed because any
  // change to the data should be visible because of data change tracking.
  // this->runParameters = QSharedPointer<WeakParameterCopy>();

  if (vx::debug_option::Log_FilterChangeTracking()->get())
    qDebug() << "Starting filter run for" << this->getPath().path();

  QSharedPointer<vx::io::RunFilterOperation> op =
      this->calculate(isAutomaticFilterRun);

  op->onFinished(
      this, [this, currentRunParameters](
                const QSharedPointer<vx::io::Operation::ResultError>& result) {
        // If filter failed, force rerun next time.
        // TODO: In many cases (e.g. isosurface extraction) cancelling the
        // filter will be considered success (but should be considered failure).
        if (result) {
          // This probably should not be needed because the change to the data
          // version should trigger a re-run anyway (if anything has changed).
          this->runParameters = QSharedPointer<WeakParameterCopy>();
          if (vx::debug_option::Log_FilterChangeTracking()->get())
            qDebug() << "Changed runParameters of" << this->getPath().path()
                     << "to nullptr";
          return;
        }

        auto newParams = ParameterCopy::getParameters(this)->createWeakCopy();

        QMap<QDBusObjectPath, WeakParameterCopy::WeakDataInfo> newDataMap =
            currentRunParameters->dataMap();
        QMap<QDBusObjectPath, QSharedPointer<NodePrototype>> newPrototypes =
            currentRunParameters->prototypes();
        QMap<QDBusObjectPath, QSharedPointer<const QMap<QString, vx::Any>>>
            newProperties = currentRunParameters->properties();
        if (!newProperties.value(currentRunParameters->mainNodePath())) {
          qCritical() << "Cannot find mainNodePath in properties";
          return;
        }
        QMap<QString, vx::Any> newSelfProperties =
            *newProperties[currentRunParameters->mainNodePath()];

        for (auto& property : prototype()->nodeProperties()) {
          // skip properties that are not output references
          if (!property->isReference() || !property->isOutputReference())
            continue;

          // Copy value of output property
          newSelfProperties[property->name()] =
              (*newParams->properties()[currentRunParameters->mainNodePath()])
                  [property->name()];

          auto objPath = this->getNodePropertyTyped<QDBusObjectPath>(property);

          // Copy data for output property node
          newDataMap.insert(objPath, newParams->dataMap()[objPath]);

          // Copy properties for output properties node
          newProperties[objPath] = newParams->properties()[objPath];

          // Copy prototypes for output properties node
          newPrototypes[objPath] = newParams->prototypes()[objPath];

          // TODO: Need to do anything for extensionInfo?
        }

        newProperties[currentRunParameters->mainNodePath()] =
            createQSharedPointer<QMap<QString, vx::Any>>(newSelfProperties);

        this->runParameters = createQSharedPointer<WeakParameterCopy>(
            currentRunParameters->mainNodePath(), newProperties, newPrototypes,
            currentRunParameters->extensionInfo(), newDataMap);
        if (vx::debug_option::Log_FilterChangeTracking()->get())
          qDebug() << "Changed runParameters of" << this->getPath().path()
                   << "to" << currentRunParameters->toString();
      });

  return op;
}

bool FilterNode::needsRecalculation() {
  QSharedPointer<WeakParameterCopy> currentParams =
      ParameterCopy::getParameters(this)->createWeakCopy();

  if (vx::debug_option::Log_FilterChangeTracking()->get())
    qDebug() << "FilterNode::needsRecalculation: last ="
             << (runParameters ? runParameters->toString() : "nullptr")
             << ", current = "
             << (currentParams ? currentParams->toString() : "nullptr");
  if (currentParams == runParameters) {
    if (vx::debug_option::Log_FilterChangeTracking()->get())
      qDebug() << "FilterNode::needsRecalculation: false because of identity";
    return false;
  }
  if (!currentParams || !runParameters) {
    if (vx::debug_option::Log_FilterChangeTracking()->get())
      qDebug() << "FilterNode::needsRecalculation: true because of null value";
    return true;
  }
  bool res = *currentParams != *runParameters;
  if (vx::debug_option::Log_FilterChangeTracking()->get())
    qDebug() << "FilterNode::needsRecalculation: Result:" << res;
  return res;
}

QMap<QString, QString> FilterNode::getExtensionInfo() { return {}; }

void FilterNode::triggerAutomaticFilterRun() {
  if (this->automaticFilterRunRunning) {
    this->automaticFilterRunPending = true;
    return;
  }

  this->automaticFilterRunPending = false;
  this->automaticFilterRunRunning = true;
  auto op = this->run(true);
  op->onFinished(
      this,
      [this](const QSharedPointer<vx::io::Operation::ResultError>& result) {
        Q_UNUSED(result);
        this->automaticFilterRunRunning = false;
        if (this->automaticFilterRunPending) {
          triggerAutomaticFilterRun();
        }
      });
}
