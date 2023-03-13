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

#include "Load.hpp"

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351
#include <QtDBus/QDBusInterface>

#include <Voxie/Node/DataNode.hpp>

#include <VoxieBackend/IO/Importer.hpp>
#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationImport.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <VoxieClient/Exception.hpp>

#include <Main/Gui/CoreWindow.hpp>
#include <Main/Gui/SidePanel.hpp>
#include <Main/Root.hpp>

#include <QtCore/QFileInfo>

using namespace vx;
using namespace vx::io;

QSharedPointer<QList<QSharedPointer<Importer>>> Load::getImporters(
    vx::Root* root) {
  auto result = createQSharedPointer<QList<QSharedPointer<Importer>>>(
      root->allComponents()->listComponentsTyped<vx::io::Importer>());

  std::sort(result->begin(), result->end(),
            [](const QSharedPointer<Importer>& l1,
               const QSharedPointer<Importer>& l2) {
              return l1->filter().filterString() < l2->filter().filterString();
            });
  return result;
}

// TODO: Remove this method?
QSharedPointer<OperationResultImport> Load::openFile(
    vx::Root* root, const QSharedPointer<Importer>& importer,
    const QString& fileName, const QMap<QString, QVariant>& properties,
    bool doCreateNode) {
  auto absFileName = QFileInfo(fileName).absoluteFilePath();

  auto result = importer->import(absFileName, doCreateNode, properties);

  if (result->doCreateNode() != doCreateNode)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "result->doCreateNode != doCreateNode");

  if (doCreateNode)
    Load::registerCreateNode(root, result, absFileName, importer, properties);

  return result;
}

void Load::registerCreateNode(
    vx::Root* root, const QSharedPointer<OperationResultImport>& result,
    const QString& fileName, const QSharedPointer<Importer>& importer,
    const QMap<QString, QVariant>& properties) {
  result->onBeforeFinishedSuccess(
      root,  // TODO?
      [root, fileName, importer,
       properties](const QSharedPointer<Operation::ResultSuccess>& res) {
        auto res2 = qSharedPointerDynamicCast<OperationImport::Result>(res);
        if (!res) {
          qCritical() << "Unable to cast to OperationImport::Result";
          return;
        }

        auto data = res2->data();
        QSharedPointer<NodePrototype> resultPrototype;
        auto prototypes =
            root->components()->listComponentsTyped<NodePrototype>();
        // TODO: Do this faster?
        for (const auto& interfaceName : data->supportedDBusInterfaces()) {
          for (const auto& prototype : prototypes) {
            if (prototype->nodeKind() != NodeKind::Data) continue;

            bool found = false;
            for (const auto& str : prototype->supportedDataDBusInterfaces()) {
              if (str == interfaceName) {
                found = true;
                break;
              }
            }
            if (!found) continue;

            if (resultPrototype) {
              // TODO: Should this trigger an error? The operation has already
              // succeeded here
              qCritical() << "Error during loading: The DBus interface"
                          << interfaceName << "is supported both by"
                          << resultPrototype->name() << "and"
                          << prototype->name();
              return;
            }
            resultPrototype = prototype;
          }

          if (resultPrototype) break;
        }
        if (!resultPrototype) {
          // TODO: Should this trigger an error? The operation has already
          // succeeded here
          qCritical() << "Import operation got unexpected object";
          return;
        }
        QSharedPointer<Node> obj0;
        try {
          obj0 =
              resultPrototype->create(QMap<QString, QVariant>(), QList<Node*>(),
                                      QMap<QString, QDBusVariant>());
        } catch (vx::Exception& e) {
          // TODO: Should this trigger an error? The operation has already
          // succeeded here
          qCritical() << "Error while creating node for import result"
                      << e.message();
          return;
        }
        auto obj = qSharedPointerDynamicCast<DataNode>(obj0);
        if (!obj) {
          // Should never happen because the code above searches for
          // NodeKind::Data prototypes
          // TODO: Should this trigger an error? The operation has already
          // succeeded here
          qCritical() << "Import result node is not a DataNode";
          return;
        }
        obj->setData(data);

        if (!voxieRoot().isHeadless()) {
          // TODO: Even without headless mode, this should only be done if this
          // is triggered by the user, not if this is triggered by some script

          // set the parent node group of this new node to the current node
          // group the user is in
          obj->setParentNodeGroup(
              ((gui::CoreWindow*)voxieRoot().mainWindow())
                  ->sidePanel->dataflowWidget->currentNodeGroup());
        }

        res2->setDataNode(obj.data(), obj->getPath());
        obj->setFileInfo(QFileInfo(fileName), importer, properties);
        obj->setAutomaticDisplayName(obj->getFileInfo().fileName());
      });
}

QSharedPointer<OperationResultImport> Load::openFile(vx::Root* root,
                                                     const QString& file,
                                                     bool doCreateNode) {
  const auto& importers = getImporters(root);
  for (const auto& importer : *importers) {
    if (importer->filter().matches(file)) {
      // Use default properties when the importer was not specified
      QMap<QString, QVariant> properties;
      for (const auto& prop : importer->properties())
        properties[prop->name()] = prop->defaultValue();

      return openFile(root, importer, file, properties, doCreateNode);
    }
  }

  auto op = OperationImport::create();
  auto result = OperationImport::ResultWrapper::create(op, doCreateNode);
  auto error = createQSharedPointer<Operation::ResultError>(
      createQSharedPointer<vx::Exception>(
          "de.uni_stuttgart.Voxie.NoImporterFound",
          "No importer found for file " + file));
  enqueueOnThread(op.data(), [op, error] {
    // TODO: Does this have to be delayed?
    op->finish(error);
  });
  return result;
}
