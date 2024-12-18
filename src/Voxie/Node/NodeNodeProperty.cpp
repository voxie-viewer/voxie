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

#include "NodeNodeProperty.hpp"

PropertyInstance::~PropertyInstance() {}

NodeNodeProperty::~NodeNodeProperty() {}

QSharedPointer<PropertyType> NodeNodeProperty::type() const {
  return property->type();
}
QVariant NodeNodeProperty::get() const {
  return node->getNodeProperty(property);
}
void NodeNodeProperty::set(const QVariant& value) const {
  node->setNodeProperty(property, value);
}
QMetaObject::Connection NodeNodeProperty::connectChanged(
    QObject* obj, std::function<void(const QVariant&)>&& callback) const {
  // TODO: Avoid listening to all property changes
  return QObject::connect(
      this->node, &vx::Node::propertyChanged, obj,
      [prop = this->property, callback](
          const QSharedPointer<NodeProperty>& property, const QVariant& value) {
        if (property != prop) return;
        callback(value);
      });
}
