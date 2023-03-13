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

#include "ExtensionFilterNode.hpp"

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

ExtensionFilterNode::ExtensionFilterNode(
    const QSharedPointer<vx::NodePrototype>& prototype,
    const QString& scriptFilename)
    : FilterNode(prototype), scriptFilename_(scriptFilename) {
  if (!voxieRoot().isHeadless()) debuggerSupportEnabled->show();

  connect(this, &ExtensionFilterNode::error, this,
          [](const Exception& e, const QSharedPointer<QString>& scriptOutput) {
            QString message = e.message();
            if (scriptOutput && *scriptOutput != "") {
              message += "\n\nScript output:\n";
              message += *scriptOutput;
            }

            qWarning() << "Error during filter:" << message;
            // Delay calling QMessageBox::exec() until later to avoid having a
            // main loop in the event handler. TODO: This should probably not
            // use QMessageBox::exec().
            if (!voxieRoot().isHeadless()) {
              enqueueOnMainThread([message] {
                QMessageBox(QMessageBox::Critical,
                            Root::instance()->mainWindow()->windowTitle(),
                            QString("Error during filter: %1").arg(message),
                            QMessageBox::Ok, Root::instance()->mainWindow())
                    .exec();
              });
            }
          });
  // TODO: Also display output of filter script when filter succeeds?

  this->setAutomaticDisplayName(prototype->displayName());

  auto extension =
      qSharedPointerDynamicCast<Extension>(this->prototype()->container());
  if (extension && extension->offerShowSource()) {
    this->sourceCodeButton->setText(
        this->sourceCodeButton->text() + " " + "<a href=\"" +
        vx::help::uriForEditSource(this->prototype()).toHtmlEscaped() +
        "\">Source code</a>");
  }
}
ExtensionFilterNode::~ExtensionFilterNode() {}

QSharedPointer<RunFilterOperation> ExtensionFilterNode::calculate() {
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
    obj->setParentNodeGroup(parentNodeGroup());
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
  auto parameterCopy = ParameterCopy::getParameters(this);
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

  auto op = RunFilterOperation::create(this, parameterCopy);

  // Start of code which must be on main thread

  op->setDescription("Run filter " + displayName());
  OperationRegistry::instance()->addOperation(op);

  // TODO: Merge some of the code with ScriptImporter::load()?
  auto exOp = ExternalOperationRunFilter::create(op, parameters, references);

  auto ext =
      qSharedPointerDynamicCast<Extension>(this->prototype()->container());
  if (!ext) {
    throw Exception("de.uni_stuttgart.Voxie.InternalError",
                    "extension is nullptr in ExtensionFilterNode");
  }
  if (debuggerSupportEnabled && debuggerSupportEnabled->isChecked())
    ext->startOperationDebug(exOp);
  else
    ext->startOperation(exOp);

  // TODO: Update calculate button state, for start, finished and error

  auto scriptOutput = op->scriptOutput();

  connect(op.data(), &RunFilterOperation::beforeFinished, this,
          [this, scriptOutput,
           outputs](const QSharedPointer<Operation::ResultBase>& res0) {
            if (auto res =
                    qSharedPointerDynamicCast<Operation::ResultSuccess>(res0)) {
              auto res2 =
                  qSharedPointerDynamicCast<RunFilterOperation::Result>(res);
              if (!res) {
                qCritical() << "Unable to cast to RunFilterOperation::Result";
                return;
              }
              const auto& result = res2->data();
              try {
                for (const auto& output : *outputs) {
                  if (!output) {
                    qWarning()
                        << "Output was destroyed before result was calculated";
                    continue;
                  }

                  auto path = output->getPath();
                  if (!result.contains(path)) {
                    qWarning()
                        << "Did not find result for output" << path.path();
                    continue;
                  }
                  auto outputResult = result[path];

                  if (outputResult.contains("Data")) {
                    auto dataPath = dbusGetVariantValue<QDBusObjectPath>(
                        outputResult["Data"]);
                    auto data = vx::Data::lookupOptional(dataPath);
                    auto outputAsData = dynamic_cast<DataNode*>(output.data());
                    if (outputAsData) {
                      outputAsData->setData(data);
                    } else {
                      qCritical()
                          << "Output node" << path.path() << "not a DataNode";
                    }
                  }
                  // TODO: Use DataVersion?

                  if (outputResult.contains("Properties")) {
                    auto properties =
                        dbusGetVariantValue<QMap<QString, QDBusVariant>>(
                            outputResult["Properties"]);
                    for (const auto& key : properties.keys()) {
                      auto property =
                          output->prototype()->getProperty(key, false);
                      output->setNodeProperty(
                          property, property->type()->dbusToRaw(
                                        QDBusVariant(properties[key])));
                    }
                  }
                }
              } catch (Exception& e) {
                error(e, scriptOutput);
              }
            } else if (auto result =
                           qSharedPointerDynamicCast<Operation::ResultError>(
                               res0)) {
              error(*result->error(), scriptOutput);
            } else {
              qCritical() << "Unkown result for in RunFilterOperation::Result";
              return;
            }
          });

  return op;
}

namespace vx {
class ExternalOperationRunFilterAdaptorImpl
    : public ExternalOperationRunFilterAdaptor {
  ExternalOperationRunFilter* object;

 public:
  ExternalOperationRunFilterAdaptorImpl(ExternalOperationRunFilter* object)
      : ExternalOperationRunFilterAdaptor(object), object(object) {}
  ~ExternalOperationRunFilterAdaptorImpl() override {}

  void Finish(const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& result,
              const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      object->checkClient();

      // qDebug() << "ExternalOperation::Finish" << data.path();

      if (object->operation()->isFinished())
        throw Exception("de.uni_stuttgart.Voxie.InvalidOperation",
                        "ExternalOperationRunFilter is already finished");

      object->operation()->finish(
          createQSharedPointer<RunFilterOperation::Result>(result));

      object->isFinished = true;
      object->cleanup();
    } catch (vx::Exception& e) {
      e.handle(object);
      return;
    }
  }

  QDBusObjectPath filterNode() const override {
    try {
      return ExportedObject::getPath(object->filterNode());
    } catch (vx::Exception& e) {
      return ExportedObject::getPath(nullptr);
    }
  }
  QDBusObjectPath filterObject() const override {
    try {
      return ExportedObject::getPath(object->filterNode());
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

// TODO: Make sure that operation->filterNode() is still alive
ExternalOperationRunFilter::ExternalOperationRunFilter(
    const QSharedPointer<vx::io::RunFilterOperation>& operation,
    const QMap<QDBusObjectPath, QMap<QString, QDBusVariant>>& parameters,
    const QSharedPointer<QList<QSharedPointer<ExportedObject>>>& references)
    : ExternalOperation(operation),
      operation_(operation),
      filterNode_(operation->filterNode()),
      filterNodePath_(operation->filterNode()->getPath()),
      prototype_(operation->filterNode()->prototype()),
      parameters_(parameters),
      references(references) {
  new ExternalOperationRunFilterAdaptorImpl(this);

  // TODO: Change ExternalOperationImport and merge this code into the
  // FinishError() method
  connect(this, &ExternalOperation::error, this, [this](const Exception& err) {
    this->operation()->finish(createQSharedPointer<Operation::ResultError>(
        createQSharedPointer<Exception>(err)));
  });
}
ExternalOperationRunFilter::~ExternalOperationRunFilter() {}

void ExternalOperationRunFilter::cleanup() { references.reset(); }

QString ExternalOperationRunFilter::action() { return "RunFilter"; }

QString ExternalOperationRunFilter::name() { return prototype_->name(); }
}  // namespace vx
