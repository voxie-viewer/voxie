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

#include <Voxie/Data/PropertySection.hpp>
#include <Voxie/Data/Prototypes.hpp>
#include <Voxie/Data/SurfaceNode.hpp>

#include <VoxieBackend/Data/HistogramProvider.hpp>
#include <VoxieBackend/Data/SurfaceAttribute.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>

#include <VoxieBackend/IO/OperationRegistry.hpp>
#include <VoxieBackend/IO/SharpThread.hpp>

#include <Voxie/Node/ParameterCopy.hpp>
#include <Voxie/Node/PropertyHelper.hpp>

#include <PluginFilter/Prototypes.hpp>

#include <QtGui/QOpenGLFunctions>

#include "ColorizeSurfaceFromAttribute.hpp"
#include "ColorizeSurfaceFromAttributeOperation.hpp"

using namespace vx::filters;
using namespace vx;
using namespace vx::io;

ColorizeSurfaceFromAttribute::ColorizeSurfaceFromAttribute()
    : FilterNode(getPrototypeSingleton()),
      properties(new ColorizeSurfaceFromAttributeProperties(this)),
      histogramProvider(decltype(histogramProvider)::create()) {
  qRegisterMetaType<QSharedPointer<SurfaceDataTriangleIndexed>>();
  qRegisterMetaType<QSharedPointer<RunFilterOperation>>();

  this->setAutomaticDisplayName("Colorize Surface From Attribute");

  PropertySection* propertySection = new PropertySection();
  attributeSelector = new QComboBox();
  propertySection->addProperty(attributeSelector);

  this->addPropertySection(propertySection);

  // make inputSurfaceChanged get raised when a different surface node is
  // connected to the filter or if the already connected surface gets changed
  forwardSignalFromPropertyNodeOnReconnect(
      properties, &ColorizeSurfaceFromAttributeProperties::inputSurface,
      &ColorizeSurfaceFromAttributeProperties::inputSurfaceChanged,
      &DataNode::dataChanged, this,
      &ColorizeSurfaceFromAttribute::inputSurfaceChanged);
  connect(this, &ColorizeSurfaceFromAttribute::inputSurfaceChanged, this,
          &ColorizeSurfaceFromAttribute::updateUI);

  // connect the current text changed signal of the attributeSelector ComboBox
  // to the onSelectedAttributeChanged method so that it gets calle when the
  // user changes the selected attribute
  connect(attributeSelector, &QComboBox::currentTextChanged, this,
          &ColorizeSurfaceFromAttribute::onSelectedAttributeChanged);
}

void ColorizeSurfaceFromAttribute::updateUI() {
  // save the attribute that was selected previously so that if it still exists
  // after the update we can select it again
  QString oldAttrName = attributeSelector->currentText();

  // remove the old items from the ComboBox, then add all the attributes the new
  // or updated input surface has
  attributeSelector->clear();
  auto* surfaceNode =
      dynamic_cast<SurfaceNode*>(this->properties->inputSurface());

  if (surfaceNode) {
    auto* surfaceData = dynamic_cast<SurfaceDataTriangleIndexed*>(
        surfaceNode->surface().data());

    if (surfaceData) {
      for (auto const& attr : surfaceData->listAttributes()) {
        // TODO: Use displayName() for display, use name() only internally
        attributeSelector->addItem(attr->name());
      }
    }
  }

  // check if an attribute with the same old name exists and if it does select
  // it
  int index = attributeSelector->findText(oldAttrName);
  if (index != -1) {
    attributeSelector->setCurrentIndex(index);
  } else {
    attributeSelector->setCurrentIndex(0);
  }

  // also update the histogram, because attribute values might have changed
  onSelectedAttributeChanged(attributeSelector->currentText());
}

void ColorizeSurfaceFromAttribute::onSelectedAttributeChanged(
    const QString& attributeName) {
  // update the histogram
  auto* surfaceNode =
      dynamic_cast<SurfaceNode*>(this->properties->inputSurface());

  // abort if we don't have a proper surface node
  if (!surfaceNode) return;

  auto* surfaceData =
      dynamic_cast<SurfaceDataTriangleIndexed*>(surfaceNode->surface().data());

  // also abort if we don't have a proper surfaceData node
  if (!surfaceData) return;

  // only update histogram provider if the selected key actually exists ("no
  // selection" is also a selection)
  // TODO: should also maybe clear the old histogram then
  QSharedPointer<SurfaceAttribute> attr =
      surfaceData->getAttributeOrNull(attributeName);
  if (attr) {
    if (attr->getOpenGLType() == GL_INT) {
      // as the SurfaceAttribute class isn't a container and also doesn't
      // provide access to its inner containers (vectors) it uses to save the
      // values we instead have to copy all values to a new vector. This is a
      // bit unnecessary and wastes memory. Should we maybe allow access to
      // SurfaceAttribute' inner containers?
      std::vector<int32_t> values(attr->getSize());
      for (unsigned int i = 0; i < attr->getSize(); i++) {
        values[i] = attr->getInt(i);
      }

      // give the data to the histogram provider for displaying
      histogramProvider->setDataFromContainer(
          HistogramProvider::DefaultBucketCount, values,
          [](int32_t value) { return value; });
    } else {
      std::vector<float> values(attr->getSize());
      for (unsigned int i = 0; i < attr->getSize(); i++) {
        values[i] = attr->getFloat(i);
      }

      histogramProvider->setDataFromContainer(
          HistogramProvider::DefaultBucketCount, values,
          [](float value) { return value; });
    }
  }
}

QSharedPointer<RunFilterOperation> ColorizeSurfaceFromAttribute::calculate(
    bool isAutomaticFilterRun) {
  Q_UNUSED(isAutomaticFilterRun);
  // TODO: Get parameter from somewhere else, don't create RunFilterOperation
  // here
  auto parameterCopy = ParameterCopy::getParameters(this);
  QSharedPointer<RunFilterOperation> operation(
      new RunFilterOperation(), [](QObject* obj) { obj->deleteLater(); });

  ColorizeSurfaceFromAttributePropertiesCopy properties =
      parameterCopy->properties()[parameterCopy->mainNodePath()];

  auto colorizeOperation = new ColorizeSurfaceFromAttributeOperation(operation);

  QList<ColorizerEntry> colorizerEntries = properties.inputColorizer();
  Colorizer* colorizer = new Colorizer();
  colorizer->setEntries(colorizerEntries);

  // abort if we couldn't get inputSurface
  if (properties.inputSurfaceRaw() == QDBusObjectPath("/")) {
    qWarning() << "ColorizeSurfaceFromAttribute::calculate(): No surface node "
                  "connected";
    // TODO: What should be done here?
    return operation;
  }

  // TODO: Don't use the input surface object here / in colorizeModel()
  auto inputSurface = dynamic_cast<SurfaceNode*>(
      PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
          properties.inputSurfaceRaw()));
  // abort if we couldn't get inputSurface
  if (!inputSurface) {
    qWarning() << "ColorizeSurfaceFromAttribute::calculate(): Could not find a "
                  "SurfaceNode parent node";
    // TODO: What should be done here?
    return operation;
  }

  auto surfData = qSharedPointerDynamicCast<SurfaceDataTriangleIndexed>(
      parameterCopy->getData(properties.inputSurfaceRaw()).data());
  // abort if input surface data could not be cast to what we need
  if (!surfData) {
    qWarning()
        << "ColorizeSurfaceFromAttribute::calculate(): Input surface data is "
           "not SurfaceDataTriangleIndexed";
    // TODO: What should be done here?
    return operation;
  }

  // abort if for some reason the attribute name selected in the ComboBox isn't
  // actually an attribute of the surface
  if (!surfData->getAttributeOrNull(attributeSelector->currentText())) {
    qWarning()
        << "ColorizeSurfaceFromAttribute::calculate(): Selected attribute "
           "from surface to be colorized "
           "could not be found in the surface!";
    // TODO: What should be done here?
    return operation;
  }

  SharpThread* thread = new SharpThread([=]() -> void {
    colorizeOperation->colorizeModel(
        inputSurface, attributeSelector->currentText(), colorizer);
    delete colorizeOperation;
    delete colorizer;
  });

  colorizeOperation->moveToThread(thread);

  // When the isosurface gets destroyed, cancel the operation
  // The thread might continue in the background until it actually processes
  // the cancellation, but this result (if any) will be ignored
  connect(this, &QObject::destroyed, operation.data(), &Operation::cancel);
  connect(thread, &QThread::finished, thread, &QObject::deleteLater);

  // update the child surface when the colorization is done
  connect(colorizeOperation,
          &ColorizeSurfaceFromAttributeOperation::colorizationDone, this,
          &ColorizeSurfaceFromAttribute::updateOutputSurface);

  OperationRegistry::instance()->addOperation(operation);

  thread->start();
  return operation;
}

QSharedPointer<QObject> ColorizeSurfaceFromAttribute::getPropertyUIData(
    QString propertyName) {
  if (propertyName ==
      "de.uni_stuttgart.Voxie.Filter.ColorizeSurfaceFromAttribute."
      "InputColorizer") {
    return histogramProvider;
  } else {
    return Node::getPropertyUIData(propertyName);
  }
}

void ColorizeSurfaceFromAttribute::updateOutputSurface(
    const QSharedPointer<SurfaceDataTriangleIndexed>& outputSurface,
    QSharedPointer<vx::io::RunFilterOperation> operation) {
  if (outputSurface) {
    QSharedPointer<SurfaceNode> surfaceNode;

    // check if a child surface node already exists and use that if it does
    // TODO: Clean up, don't use childNodes()
    for (Node* childNode : this->childNodes()) {
      if (SurfaceNode* output = dynamic_cast<SurfaceNode*>(childNode)) {
        surfaceNode = output->thisShared();
      }
    }

    // if we find no existing child surface node create a new one
    if (!surfaceNode) {
      surfaceNode = createNode<SurfaceNode>();
      this->addChildNode(surfaceNode.data());
    }

    // set the surface data of the child surface node to the new data that our
    // filter generated
    surfaceNode->setSurface(outputSurface);
  }

  operation->emitFinished();
}

NODE_PROTOTYPE_IMPL(ColorizeSurfaceFromAttribute)
