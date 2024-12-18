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

#include "PropertyUI.hpp"

#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodeNodeProperty.hpp>
#include <Voxie/Node/NodeProperty.hpp>

#include <QtWidgets/QWidget>

using namespace vx;

PropertyUI::PropertyUI(const QSharedPointer<NodeProperty>& property, Node* node)
    : property_(property), node_(node) {}

PropertyUI::PropertyUI(const QSharedPointer<PropertyInstance>& propertyInstance)
    : propertyInstance_(propertyInstance) {
  auto nodeProp = qSharedPointerDynamicCast<NodeNodeProperty>(propertyInstance);
  if (nodeProp) {
    property_ = nodeProp->property;
    node_ = nodeProp->node;
  }
}

PropertyUI::~PropertyUI() {}

void PropertyUI::init() {
  if (initCalled) {
    qWarning() << "PropertyUI::init() has already been called";
    return;
  }

  // Automatically enable/disable property, only done when a property+node was
  // passed to createUI.
  if (this->property()) {
    QSet<NodeProperty*> deps;
    this->property()->enabledCondition()->collectDependencies(deps);
    Node* obj = this->node();
    if (!obj) {
      qWarning() << "PropertyUI::init() running for destroyed node";
      return;
    }
    for (const auto& dep : deps) {
      connect(node(), &Node::propertyChanged, this,
              [this, dep](const QSharedPointer<NodeProperty>& property,
                          const QVariant& value) {
                Q_UNUSED(value);
                if (property != dep) return;
                Node* obj2 = this->node();
                if (!obj2) {
                  qDebug() << "setEnabled() updater running for destroyed node";
                  return;
                }
                bool enabled =
                    this->property()->enabledCondition()->evaluate(obj2);
                // qDebug() << this->property()->name() << "isEnabled update"
                //          << enabled;
                this->setEnabled(enabled);
              });
    }
    bool enabled = this->property()->enabledCondition()->evaluate(obj);
    // qDebug() << this->property()->name() << "isEnabled init" << enabled;
    this->setEnabled(enabled);
  }

  init2();

  initCalled = true;
}

void PropertyUI::init2() {}

void PropertyUI::setEnabled(bool enabled) { widget()->setEnabled(enabled); }
