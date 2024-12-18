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

#include <PluginFilter/CreateSurface/CreateSurface.hpp>
#include <PluginFilter/Prototypes.hpp>

#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/SurfaceBuilder.hpp>

#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieBackend/IO/OperationRegistry.hpp>

#include <QApplication>
#include <QInputDialog>
#include <QLabel>
#include <QObject>
#include <QSharedPointer>
#include <Voxie/Data/SurfaceNode.hpp>
#include "Cuberille.hpp"
#include "IsosurfaceExtractionOperation.hpp"
#include "MarchingCubes.hpp"
#include "SurfaceExtractor.hpp"

VX_NODE_INSTANTIATION(vx::filters::CreateSurface)

using namespace vx::filters;
using namespace vx;
using namespace vx::io;

CreateSurface::CreateSurface()
    : FilterNode(getPrototypeSingleton()),
      properties(new CreateSurfaceProperties(this)) {
  qRegisterMetaType<QSharedPointer<SurfaceDataTriangleIndexed>>();
  qRegisterMetaType<QSharedPointer<RunFilterOperation>>();
  PropertySection* section = new PropertySection();
  this->setAutomaticDisplayName("Create Surface");
  this->addPropertySection(section);
  this->algorithmInput =
      section->addComboBoxProperty({"Cuberille", "Marching Cubes"});
  this->algorithmInput->setCurrentIndex(1);
}

QSharedPointer<vx::io::RunFilterOperation> CreateSurface::calculate(
    bool isAutomaticFilterRun) {
  Q_UNUSED(isAutomaticFilterRun);
  QSharedPointer<SurfaceExtractor> extractor;

  if (this->algorithmInput->currentIndex() == 0) {
    extractor.reset(new Cuberille(), [](QObject* obj) { obj->deleteLater(); });
  } else {
    extractor.reset(new MarchingCubes(),
                    [](QObject* obj) { obj->deleteLater(); });
  }

  QSharedPointer<RunFilterOperation> operation(
      new RunFilterOperation(), [](QObject* obj) { obj->deleteLater(); });

  // This object will be moved to the newly created thread and will be deleted
  // on this thread once the operation is finished
  auto extractionOperation = new IsosurfaceExtractionOperation(
      operation, extractor, this->properties->threshold(), this->inverted);

  VolumeNode* dataSet =
      dynamic_cast<VolumeNode*>(this->properties->inputVolume());
  if (!dataSet) {
    qWarning() << "CreateSurface::calculate(): Could not find a VolumeNode "
                  "parent node";
    return operation;
  }

  VolumeNode* labelSet =
      dynamic_cast<VolumeNode*>(this->properties->labelVolume());

  SharpThread* thread =
      new SharpThread([extractionOperation, dataSet, labelSet]() -> void {
        extractionOperation->generateModel(dataSet, labelSet);
        delete extractionOperation;
      });
  extractionOperation->moveToThread(thread);

  // When the isosurface gets destroyed, cancel the operation
  // The thread might continue in the background until it actually processes
  // the cancellation, but this result (if any) will be ignored
  connect(this, &QObject::destroyed, operation.data(), &Operation::cancel);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);
  connect(extractionOperation, &IsosurfaceExtractionOperation::generationDone,
          this, &CreateSurface::updateSurface);

  OperationRegistry::instance()->addOperation(operation);

  thread->start();
  return operation;
}

void CreateSurface::updateSurface(
    VolumeNode* dataSet,
    const QSharedPointer<SurfaceDataTriangleIndexed>& surfacePointer,
    QSharedPointer<RunFilterOperation> operation) {
  (void)dataSet;
  // qDebug() << "updateSurface1";
  if (surfacePointer) {
    QSharedPointer<SurfaceNode> surfaceNode;

    // check if a child surface node already exists and use that if it does
    // TODO: Clean up, don't use childNodes()
    for (Node* childNode : this->childNodes()) {
      if (SurfaceNode* output = dynamic_cast<SurfaceNode*>(childNode)) {
        surfaceNode = output->thisShared();
      }
    }

    if (!surfaceNode) {
      surfaceNode = createNode<SurfaceNode>();
      this->addChildNode(surfaceNode.data());
    }

    // qDebug() << "updateSurface2";
    surfaceNode->setSurface(surfacePointer);
  }
  operation->emitFinished();
}

QVariant CreateSurface::getNodePropertyCustom(QString key) {
  // TODO nils konstante
  if (key == "de.uni_stuttgart.Voxie.Filter.CreateSurface.Algorithm") {
    return QVariant(this->algorithmInput->currentText());
  } else {
    return Node::getNodePropertyCustom(key);
  }
}

void CreateSurface::setNodePropertyCustom(QString key, QVariant value) {
  if (key == "de.uni_stuttgart.Voxie.Filter.CreateSurface.Algorithm") {
    if ((QMetaType::Type)value.type() != QMetaType::QString) {
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Wrong type for algorithm value. String needed.");
    }
    auto index = this->algorithmInput->findText(value.toString());
    if (index != -1) {
      this->algorithmInput->setCurrentIndex(index);
    }
  } else {
    Node::setNodePropertyCustom(key, value);
  }
}
