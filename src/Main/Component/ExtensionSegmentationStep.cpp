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

#include "ExtensionSegmentationStep.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusUtil.hpp>

#include <Voxie/Component/HelpCommon.hpp>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

#include <VoxieBackend/Property/PropertyType.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/Types.hpp>

#include <Main/Root.hpp>

#include <VoxieBackend/Component/Extension.hpp>

#include <QtWidgets/QMessageBox>

using namespace vx;
using namespace vx::io;

ExtensionSegmentationStep::ExtensionSegmentationStep(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QString& scriptFilename)
    : SegmentationStep("ExtensionSegmentationStep", prototype),
      scriptFilename_(scriptFilename),
      infoString(prototype->rawJson()["StepTip"].toString()) {
  qRegisterMetaType<QSharedPointer<vx::io::Operation>>();

  connect(this, &ExtensionSegmentationStep::error, this,
          [](const Exception& e, const QSharedPointer<QString>& scriptOutput) {
            QString message = e.message();
            if (scriptOutput && *scriptOutput != "") {
              message += "\n\nScript output:\n";
              message += *scriptOutput;
            }

            qWarning() << "Error during ExtensionSegmentationStep:" << message;
            // Delay calling QMessageBox::exec() until later to avoid having a
            // main loop in the event handler. TODO: This should probably not
            // use QMessageBox::exec().
            enqueueOnMainThread([message] {
              QMessageBox(QMessageBox::Critical,
                          vx::voxieRoot().mainWindow()->windowTitle(),
                          QString("Error during ExtensionSegmentationStep: %1")
                              .arg(message),
                          QMessageBox::Ok, vx::voxieRoot().mainWindow())
                  .exec();
            });
          });
  // TODO: Also display output of filter script when filter succeeds?

  this->setAutomaticDisplayName(prototype->displayName());
}

ExtensionSegmentationStep::~ExtensionSegmentationStep() {}

void ExtensionSegmentationStep::resetUIWidget() {}

void ExtensionSegmentationStep::onVoxelOpFinished(bool status) {
  Q_UNUSED(status);
}

QSharedPointer<OperationResult> ExtensionSegmentationStep::calculate(
    QSharedPointer<ParameterCopy> parameterCopy,
    QSharedPointer<ContainerData> containerData, QDBusObjectPath inputVolume) {
  size_t outputCounts = 0;
  for (const auto& property : this->prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;
    outputCounts++;
  }

  // Create output nodes if needed
  auto outputs = createQSharedPointer<QList<QPointer<Node>>>();
  for (const auto& property : this->prototype()->nodeProperties()) {
    if (!property->isReference() || !property->isOutputReference()) continue;

    if (property->type() != types::OutputNodeReferenceType()) {
      qWarning() << "Unknown output property type:" << property->typeName();
      continue;
    }

    auto currentValue = parseVariantNode(this->getNodeProperty(property));
    if (currentValue) {
      if (auto dataNode = dynamic_cast<DataNode*>(currentValue)) {
        dataNode->setTags(property->outTags());
      }
      outputs->append(currentValue);
      if (outputCounts > 1)
        currentValue->setAutomaticDisplayName(
            "Output '" + property->displayName() + "' of " + displayName());
      else
        currentValue->setAutomaticDisplayName("Output of " + displayName());
      continue;
    }

    auto allowedTypes = property->allowedTypes();
    if (allowedTypes.size() == 0) {
      qWarning() << "Empty allowedTypes for output property"
                 << property->name();
      continue;
    }
    auto type = allowedTypes[0].lock();
    if (!type) {
      qWarning() << " allowedTypes[0] has been destroyed for output property"
                 << property->name();
      continue;
    }

    QMap<QString, QVariant> properties;
    QList<Node*> inputs;
    QMap<QString, QDBusVariant> options;
    auto obj = type->create(properties, inputs, options);
    auto dataObj = qSharedPointerDynamicCast<DataNode>(obj);
    dataObj->setTags(property->outTags());
    outputs->append(obj.data());
    if (outputCounts > 1)
      obj->setAutomaticDisplayName("Output '" + property->displayName() +
                                   "' of " + displayName());
    else
      obj->setAutomaticDisplayName("Output of " + displayName());

    this->setNodeProperty(property, getVariant(obj.data()));
  }

  // Find all referenced nodes
  QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> parameters;
  auto references =
      createQSharedPointer<QList<QSharedPointer<ExportedObject>>>();
  for (const auto& nodePath : parameterCopy->properties().keys()) {
    const auto& properties = parameterCopy->properties()[nodePath];
    const auto& prototype = parameterCopy->prototypes()[nodePath];

    QMap<QString, QDBusVariant>& result = parameters[nodePath];

    QMap<QString, QDBusVariant> propertiesDBus;
    for (const auto& propertyName : properties->keys()) {
      propertiesDBus[propertyName] =
          prototype->getProperty(propertyName, false)
              ->type()
              ->rawToDBus((*properties)[propertyName]);
    }
    result["PrototypeName"] = dbusMakeVariant<QString>(prototype->name());
    result["Properties"] =
        dbusMakeVariant<QMap<QString, QDBusVariant>>(propertiesDBus);

    if (parameterCopy->extensionInfo().contains(nodePath)) {
      auto info = parameterCopy->extensionInfo()[nodePath];
      QMap<QString, QDBusVariant> infoDBus;
      for (const auto& name : info.keys()) {
        infoDBus[name] = dbusMakeVariant<QString>(info[name]);
      }
      result["ExtensionInfo"] =
          dbusMakeVariant<QMap<QString, QDBusVariant>>(infoDBus);
    }

    if (parameterCopy->dataMap().contains(nodePath)) {
      auto info = parameterCopy->getData(nodePath);
      if (!info.data()) {
        result["Data"] = dbusMakeVariant<QDBusObjectPath>(QDBusObjectPath("/"));
      } else {
        result["Data"] =
            dbusMakeVariant<QDBusObjectPath>(info.data()->getPath());
        result["DataVersion"] =
            dbusMakeVariant<QDBusObjectPath>(info.version()->getPath());
        references->append(info.data());
        references->append(info.version());
      }
    }
  }

  // Run filter operation

  // delete op;
  // auto op = RunFilterOperation::create(this, parameterCopy);

  // Start of code which must be on main thread

  // op->setDescription("Run SegmentationStep " + displayName());

  auto op = OperationSimple::create();

  OperationRegistry::instance()->addOperation(op);

  // TODO: Merge some of the code with ScriptImporter::load()?
  auto exOp = ExternalOperationRunSegmentationStep::create(
      op, parameters, references, this->thisShared(), containerData,
      inputVolume);

  auto ext =
      qSharedPointerDynamicCast<Extension>(this->prototype()->container());
  if (!ext) {
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "extension is nullptr in ExtensionSegmentationStep");
  }
  // Maybe add later debugger support
  // if (debuggerSupportEnabled->isChecked())
  //   ext->startOperationDebug(exOp);
  // else

  // ext->startOperationDebug(exOp);
  ext->startOperation(exOp);

  connect(
      exOp.data(), &ExternalOperationRunSegmentationStep::error, this,
      [=](const vx::Exception& err) { this->error(err, op->scriptOutput()); });

  // TODO: Update calculate button state, for start, finished and error

  auto opRes = OperationResult::create(op);

  return opRes;
}

QMap<QString, QString> ExtensionSegmentationStep::getExtensionInfo() {
  auto ext =
      qSharedPointerDynamicCast<Extension>(this->prototype()->container());
  if (!ext) {
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "extension is nullptr in ExtensionSegmentationStep");
  }
  return ext->getExtensionInfo();
}

QString ExtensionSegmentationStep::getInfoString() { return this->infoString; }

bool ExtensionSegmentationStep::isAllowedChild(NodeKind node) {
  Q_UNUSED(node);
  return false;
}

bool ExtensionSegmentationStep::isAllowedParent(NodeKind node) {
  return node == NodeKind::Data;
}
bool ExtensionSegmentationStep::isCreatableChild(NodeKind) { return false; }

QList<QString> ExtensionSegmentationStep::supportedDBusInterfaces() {
  return {};
}

void ExtensionSegmentationStep::initializeCustomUIPropSections() {}

namespace vx {
class ExternalOperationRunSegmentationStepAdaptorImpl
    : public ExternalOperationRunSegmentationStepAdaptor {
  ExternalOperationRunSegmentationStep* object;

 public:
  ExternalOperationRunSegmentationStepAdaptorImpl(
      ExternalOperationRunSegmentationStep* object)
      : ExternalOperationRunSegmentationStepAdaptor(object), object(object) {}
  ~ExternalOperationRunSegmentationStepAdaptorImpl() {}

  void Finish(const QDBusObjectPath& labelDataVersion,
              const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      object->checkClient();

      auto labelDataVersionPointer = DataVersion::lookup(labelDataVersion);

      if (!(labelDataVersionPointer->data() ==
            object->containerData()->getElement("labelVolume"))) {
        throw Exception("de.uni_stuttgart.Voxie.Exception",
                        "labelDataVersionPointer is unequal labelVolume in "
                        "containerData");
      }

      if (object->operation()->isFinished())
        throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "ExternalOperationRunFilter is already finished");

      object->isFinished = true;
      object->cleanup();
    } catch (vx::Exception& e) {
      e.handle(object);
      return;
    }
  }

  virtual QDBusObjectPath labelData() const override {
    try {
      return ExportedObject::getPath(
          object->containerData()->getElement("labelVolume"));
    } catch (vx::Exception& e) {
      return ExportedObject::getPath(nullptr);
    }
  }

  virtual QDBusObjectPath containerData() const override {
    try {
      return ExportedObject::getPath(object->containerData());
    } catch (vx::Exception& e) {
      return ExportedObject::getPath(nullptr);
    }
  }

  virtual QDBusObjectPath inputNode() const override {
    try {
      return object->inputVolume();
    } catch (vx::Exception& e) {
      return ExportedObject::getPath(nullptr);
    }
  }

  QDBusObjectPath segmentationStepNode() const override {
    try {
      return ExportedObject::getPath(object->SegmentationStep());
    } catch (vx::Exception& e) {
      return ExportedObject::getPath(nullptr);
    }
  }

  QMap<QDBusObjectPath, QMap<QString, QDBusVariant>> parameters()
      const override {
    try {
      return object->parameters();
    } catch (vx::Exception& e) {
      return QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>();
    }
  }
};

// TODO: Make sure that operation->SegmentationStep() is still alive
ExternalOperationRunSegmentationStep::ExternalOperationRunSegmentationStep(
    const QSharedPointer<vx::io::Operation>& operation,
    const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& parameters,
    const QSharedPointer<QList<QSharedPointer<vx::ExportedObject>>>& references,
    QSharedPointer<vx::SegmentationStep> step,
    QSharedPointer<vx::ContainerData> containerData,
    QDBusObjectPath inputVolume)
    : ExternalOperation(operation),
      operation_(operation),
      segmentationStep_(step),
      SegmentationStepPath_(step->getPath()),
      containerData_(containerData),
      inputVolume_(inputVolume),
      parameters_(parameters),
      references(references) {
  new ExternalOperationRunSegmentationStepAdaptorImpl(this);

  // TODO: Change ExternalOperationImport and merge this code into the
  // FinishError() method
  connect(this, &ExternalOperation::error, this, [this](const Exception& err) {
    this->operation()->finish(createQSharedPointer<Operation::ResultError>(
        createQSharedPointer<Exception>(err)));
  });
}
ExternalOperationRunSegmentationStep::~ExternalOperationRunSegmentationStep() {}

void ExternalOperationRunSegmentationStep::cleanup() { references.reset(); }

QString ExternalOperationRunSegmentationStep::action() {
  return "RunSegmentationStep";
}

QString ExternalOperationRunSegmentationStep::name() {
  return prototype_->name();
}
}  // namespace vx
