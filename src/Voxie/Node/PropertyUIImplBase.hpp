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

#pragma once

#include <VoxieBackend/Property/PropertyType.hpp>

#include <Voxie/Node/PropertyUI.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>

#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodeNodeProperty.hpp>
#include <Voxie/Node/NodeProperty.hpp>

namespace vx {

template <typename TypeInfo_>
class PropertyUIImplBase : public PropertyUI {
 public:
  typedef TypeInfo_ TypeInfo;
  typedef typename TypeInfo::RawType RawType;
  typedef typename TypeInfo::QtType CppType;

  bool suppressForwardToUI = false;
  bool suppressForwardFromUI = false;

 protected:
  // TODO: Merge code with other constructor, i.e. by calling it with
  // NodeNodeProperty instance?
  PropertyUIImplBase(const QSharedPointer<NodeProperty>& property, Node* node)
      : PropertyUI(property, node) {
    if (property->type() != TypeInfo::type()) {
      qDebug() << "Trying to construct a property UI with incorrect type:"
               << property->name() << "has type" << property->type()->name()
               << "but the property UI expects" << TypeInfo::type()->name();
    }

    // TODO: listen only for changes of this one property
    QObject::connect(
        node, &Node::propertyChanged, this,
        [this](const QSharedPointer<NodeProperty>& property2,
               const QVariant& value) {
          try {
            // qDebug() << "Property change of" << property->name()
            // << "to" << value;
            if (property2 != this->property()) return;
            Node* o = this->node();
            if (!o) return;  // Probably should never happen
            if (suppressForwardToUI) return;
            auto valRaw = Node::parseVariant<RawType>(value);
            auto val =
                PropertyValueConvertRaw<RawType, CppType>::fromRaw(valRaw);
            suppressForwardFromUI = true;
            updateUIValue(val);
          } catch (Exception& e) {
            qCritical() << "Error while updating property UI:" << e.what();
          }
          suppressForwardFromUI = false;
        });
  }
  PropertyUIImplBase(const QSharedPointer<PropertyInstance>& propertyInstance)
      : PropertyUI(propertyInstance) {
    if (propertyInstance->type() != TypeInfo::type()) {
      qDebug() << "Trying to construct a property UI with incorrect type:"
               << "Property has type" << propertyInstance->type()->name()
               << "but the property UI expects" << TypeInfo::type()->name();
    }

    propertyInstance->connectChanged(this, [this](const QVariant& value) {
      try {
        Node* o = this->node();
        if (!o) return;  // Probably should never happen
        if (suppressForwardToUI) return;
        auto valRaw = Node::parseVariant<RawType>(value);
        auto val = PropertyValueConvertRaw<RawType, CppType>::fromRaw(valRaw);
        suppressForwardFromUI = true;
        updateUIValue(val);
      } catch (Exception& e) {
        qCritical() << "Error while updating property UI:" << e.what();
      }
      suppressForwardFromUI = false;
    });
  }
  ~PropertyUIImplBase() override {}

  // Will be called whenever the property value is changed. This method can
  // throw exception (which will be caught and displayed)
  virtual void updateUIValue(const CppType& value) = 0;

  // Will throw if the node is already destroyed or there is an error while
  // getting the value
  CppType getValue() {
    Node* o = this->node();
    if (!o) {
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "getUIValue() called for destroyed node");
    }
    auto valVariant = o->getNodeProperty(this->property());
    auto valRaw = Node::parseVariant<RawType>(valVariant);
    return PropertyValueConvertRaw<RawType, CppType>::fromRaw(valRaw);
  }

  // Will catch errors, will ignore the call if the node is already destroyed
  // Will ignore calls while an updateUIValue() call is running
  void setValueChecked(const CppType& value) {
    // qDebug() << "Updating value to" << value;
    // qDebug() << "Updating value";
    Node* o = this->node();
    if (!o) return;
    if (suppressForwardFromUI) return;
    try {
      auto valRaw = PropertyValueConvertRaw<RawType, CppType>::toRaw(value);
      suppressForwardToUI = true;
      o->setNodeProperty(this->property(),
                         QVariant::fromValue<RawType>(valRaw));
    } catch (Exception& e) {
      qCritical() << "Error while updating property value:" << e.what();
    }
    suppressForwardToUI = false;
  }

  void init2() override {
    try {
      Node* o = this->node();
      if (!o)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Node already destoryed in init2()");
      auto value = o->getNodeProperty(this->property());
      auto valRaw = Node::parseVariant<RawType>(value);
      auto val = PropertyValueConvertRaw<RawType, CppType>::fromRaw(valRaw);
      updateUIValue(val);
    } catch (Exception& e) {
      qCritical() << "Error while setting initial value:" << e.what();
    }
  }
};

}  // namespace vx
