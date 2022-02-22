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

// TODO: split into one file for the voxie-specific stuff and one file for the
// generic stuff?

// TODO: clean up, unify code?

#include <Voxie/Node/Node.hpp>
#include <Voxie/Node/NodeProperty.hpp>

#include <Voxie/Node/Types.hpp>

#include <QtCore/QDebug>
#include <QtCore/QPointer>

#include <typeinfo>

namespace vx {
template <typename SourceProperties, typename PropType, typename RealType,
          typename Target, typename TargetSignal, typename... Pars>
class ForwardSignalFromPropertyNodeHelper : public QObject {
  QPointer<SourceProperties> properties;
  PropType* (SourceProperties::*getter)();
  void (SourceProperties::*changedSignal)(PropType*);
  void (RealType::*signal)(Pars...);
  QPointer<Target> target;
  TargetSignal targetSignal;

  QMetaObject::Connection connection;

  void disconnect() {
    // qDebug() << "disconnect";
    if (connection) {
      // qDebug() << "disconnect2";
      QObject::disconnect(connection);
      connection = QMetaObject::Connection();
    }
  }

  void connect(RealType* real) {
    // qDebug() << "connect";
    connection = QObject::connect(real, signal, target, targetSignal);
  }

 public:
  ForwardSignalFromPropertyNodeHelper(
      SourceProperties* properties, PropType* (SourceProperties::*getter)(),
      void (SourceProperties::*changedSignal)(PropType*),
      void (RealType::*signal)(Pars...), Target* target,
      TargetSignal targetSignal)
      : properties(properties),
        getter(getter),
        changedSignal(changedSignal),
        signal(signal),
        target(target),
        targetSignal(targetSignal) {
    QObject::connect(properties, &QObject::destroyed, this,
                     &QObject::deleteLater);
    QObject::connect(target, &QObject::destroyed, this, &QObject::deleteLater);

    QObject::connect(
        properties, changedSignal, this, [this](PropType* newValue) {
          disconnect();
          if (!this->properties || !this->target) return;
          if (!newValue) return;
          auto newValueCast = dynamic_cast<RealType*>(newValue);
          if (!newValueCast) {
            qWarning() << "Cast in ForwardSignalFromPropertyNodeHelper "
                          "failed, expected "
                       << typeid(RealType).name() << ", got "
                       << typeid(*newValue).name();
          } else {
            connect(newValueCast);
          }
        });
    auto value = (properties->*getter)();
    if (value) {
      auto valueCast = dynamic_cast<RealType*>(value);
      if (!valueCast) {
        qWarning() << "Initial cast in ForwardSignalFromPropertyNodeHelper "
                      "failed, expected "
                   << typeid(RealType).name() << ", got "
                   << typeid(*value).name();
      } else {
        connect(valueCast);
      }
    }
  }

  ~ForwardSignalFromPropertyNodeHelper() override { disconnect(); }
};

template <typename SourceProperties, typename PropType, typename RealType,
          typename Target, typename TargetSignal, typename... Pars>
class ForwardSignalFromListPropertyNodeHelper : public QObject {
  QPointer<SourceProperties> properties;
  QList<PropType*> (SourceProperties::*getter)();
  void (SourceProperties::*changedSignal)(QList<PropType*>);
  void (RealType::*signal)(Pars...);
  QPointer<Target> target;
  TargetSignal targetSignal;

  QList<QMetaObject::Connection> connections;

  void disconnect() {
    // qDebug() << "disconnect";
    for (const auto& connection : connections) {
      // qDebug() << "disconnect2";
      QObject::disconnect(connection);
    }
    connections.clear();
  }

  void addConnection(RealType* real) {
    // qDebug() << "connect";
    connections << QObject::connect(real, signal, target, targetSignal);
  }

 public:
  ForwardSignalFromListPropertyNodeHelper(
      SourceProperties* properties,
      QList<PropType*> (SourceProperties::*getter)(),
      void (SourceProperties::*changedSignal)(QList<PropType*>),
      void (RealType::*signal)(Pars...), Target* target,
      TargetSignal targetSignal)
      : properties(properties),
        getter(getter),
        changedSignal(changedSignal),
        signal(signal),
        target(target),
        targetSignal(targetSignal) {
    QObject::connect(properties, &QObject::destroyed, this,
                     &QObject::deleteLater);
    QObject::connect(target, &QObject::destroyed, this, &QObject::deleteLater);

    QObject::connect(
        properties, changedSignal, this, [this](QList<PropType*> newValue) {
          disconnect();
          if (!this->properties || !this->target) return;
          for (const auto& entry : newValue) {
            auto newValueCast = dynamic_cast<RealType*>(entry);
            if (!newValueCast) {
              qWarning() << "Cast in ForwardSignalFromPropertyNodeHelper "
                            "failed, expected "
                         << typeid(RealType).name() << ", got "
                         << typeid(*entry).name();
            } else {
              addConnection(newValueCast);
            }
          }
        });
    auto value = (properties->*getter)();
    disconnect();
    for (const auto& entry : value) {
      auto valueCast = dynamic_cast<RealType*>(entry);
      if (!valueCast) {
        qWarning() << "Initial cast in ForwardSignalFromListPropertyNodeHelper "
                      "failed, expected "
                   << typeid(RealType).name() << ", got "
                   << typeid(*entry).name();
      } else {
        addConnection(valueCast);
      }
    }
  }

  ~ForwardSignalFromListPropertyNodeHelper() override { disconnect(); }
};

template <typename RealType, typename Target, typename TargetSignal,
          typename PropertyChangedHandler, typename... Pars>
class ForwardSignalFromPropertyHelper : public QObject {
  QPointer<Node> node;
  QSharedPointer<NodeProperty> property;
  void (RealType::*signal)(Pars...);
  QPointer<Target> target;
  TargetSignal targetSignal;
  PropertyChangedHandler propertyChangedHandler;

  QMetaObject::Connection connection;

  void disconnect() {
    // qDebug() << "disconnect";
    if (connection) {
      // qDebug() << "disconnect2";
      QObject::disconnect(connection);
      connection = QMetaObject::Connection();
    }
  }

  void connect(RealType* real) {
    // qDebug() << "connect";
    connection = QObject::connect(real, signal, target, targetSignal);
  }

 public:
  ForwardSignalFromPropertyHelper(
      Node* node, const QSharedPointer<NodeProperty>& property,
      void (RealType::*signal)(Pars...), Target* target,
      TargetSignal targetSignal,
      const PropertyChangedHandler& propertyChangedHandler)
      : node(node),
        property(property),
        signal(signal),
        target(target),
        targetSignal(targetSignal),
        propertyChangedHandler(propertyChangedHandler) {
    QObject::connect(node, &QObject::destroyed, this, &QObject::deleteLater);
    QObject::connect(target, &QObject::destroyed, this, &QObject::deleteLater);

    if (property->type() != vx::types::NodeReferenceType()) {
      qWarning() << "ForwardSignalFromPropertyHelper got a property whose type "
                    "is not NodeReference";
      return;
    }

    QObject::connect(node, &Node::propertyChanged, this,
                     [this](const QSharedPointer<NodeProperty>& property2,
                            const QVariant& value2) {
                       // TODO: somehow only register for this property?
                       if (property2 != this->property) return;
                       auto newValue = Node::parseVariantNode(value2);

                       disconnect();
                       if (!this->node || !this->target) return;

                       if (newValue) {
                         auto newValueCast = dynamic_cast<RealType*>(newValue);
                         if (!newValueCast) {
                           qWarning()
                               << "Cast in ForwardSignalFromPropertyHelper "
                                  "failed, expected "
                               << typeid(RealType).name() << ", got "
                               << typeid(*newValue).name();
                         } else {
                           connect(newValueCast);
                         }
                       }

                       this->propertyChangedHandler();
                     });
    auto value = Node::parseVariantNode(node->getNodeProperty(property));
    if (value) {
      auto valueCast = dynamic_cast<RealType*>(value);
      if (!valueCast) {
        qWarning() << "Initial cast in ForwardSignalFromPropertyHelper "
                      "failed, expected "
                   << typeid(RealType).name() << ", got "
                   << typeid(*value).name();
      } else {
        connect(valueCast);
      }
    }
  }

  ~ForwardSignalFromPropertyHelper() override { disconnect(); }
};

template <typename T>
struct PropertyHelperGetFunPointerClass {};
template <typename RetType, typename Cls, typename... Pars>
struct PropertyHelperGetFunPointerClass<RetType (Cls::*)(Pars...)> {
  typedef Cls type;
};

/**
 * Forward signal from the property in properties with getter getter and the
 * changed signal changedSignal to the target signal targetSignal.
 */
template <typename SourceProperties, typename PropType, typename RealType,
          typename Target1, typename TargetSignal, typename... Pars>
void forwardSignalFromPropertyNode(
    SourceProperties* properties, PropType* (SourceProperties::*getter)(),
    void (SourceProperties::*changedSignal)(PropType*),
    void (RealType::*signal)(Pars...), Target1* target,
    TargetSignal targetSignal) {
  using Target2 = typename PropertyHelperGetFunPointerClass<TargetSignal>::type;
  new ForwardSignalFromPropertyNodeHelper<SourceProperties, PropType, RealType,
                                          Target2, TargetSignal, Pars...>(
      properties, getter, changedSignal, signal, target, targetSignal);
}

/**
 * Similar to forwardSignalFromPropertyNode(), but also invoke targetSignal
 * when the connection changes. This means that the target signal must not have
 * any arguments.
 */
template <typename SourceProperties, typename PropType, typename RealType,
          typename Target1, typename TargetSignal, typename... Pars>
void forwardSignalFromPropertyNodeOnReconnect(
    SourceProperties* properties, PropType* (SourceProperties::*getter)(),
    void (SourceProperties::*changedSignal)(PropType*),
    void (RealType::*signal)(Pars...), Target1* target,
    TargetSignal targetSignal, bool invokeNow = false) {
  forwardSignalFromPropertyNode(properties, getter, changedSignal, signal,
                                target, targetSignal);
  QObject::connect(properties, changedSignal, target, targetSignal);
  if (invokeNow) {
    QObject obj;
    QObject::connect(&obj, &QObject::destroyed, target, targetSignal);
  }  // obj will be destroyed here
}

/**
 * Forward signal from the property in properties with getter getter and the
 * changed signal changedSignal to the target signal targetSignal.
 */
template <typename SourceProperties, typename PropType, typename RealType,
          typename Target1, typename TargetSignal, typename... Pars>
void forwardSignalFromListPropertyNode(
    SourceProperties* properties,
    QList<PropType*> (SourceProperties::*getter)(),
    void (SourceProperties::*changedSignal)(QList<PropType*>),
    void (RealType::*signal)(Pars...), Target1* target,
    TargetSignal targetSignal) {
  using Target2 = typename PropertyHelperGetFunPointerClass<TargetSignal>::type;
  new ForwardSignalFromListPropertyNodeHelper<
      SourceProperties, PropType, RealType, Target2, TargetSignal, Pars...>(
      properties, getter, changedSignal, signal, target, targetSignal);
}

/**
 * Similar to forwardSignalFromListPropertyNode(), but also invoke
 * targetSignal when the connection changes. This means that the target signal
 * must not have any arguments.
 */
template <typename SourceProperties, typename PropType, typename RealType,
          typename Target1, typename TargetSignal, typename... Pars>
void forwardSignalFromListPropertyNodeOnReconnect(
    SourceProperties* properties,
    QList<PropType*> (SourceProperties::*getter)(),
    void (SourceProperties::*changedSignal)(QList<PropType*>),
    void (RealType::*signal)(Pars...), Target1* target,
    TargetSignal targetSignal, bool invokeNow = false) {
  forwardSignalFromListPropertyNode(properties, getter, changedSignal, signal,
                                    target, targetSignal);
  QObject::connect(properties, changedSignal, target, targetSignal);
  if (invokeNow) {
    QObject obj;
    QObject::connect(&obj, &QObject::destroyed, target, targetSignal);
  }  // obj will be destroyed here
}

/**
 * Forward signal from node referenced by property property to the target
 * signal targetSignal.
 *
 * The property must be of type NodeReference.
 */
template <typename RealType, typename Target1, typename TargetSignal,
          typename... Pars>
void forwardSignalFromProperty(Node* node,
                               const QSharedPointer<NodeProperty>& property,
                               void (RealType::*signal)(Pars...),
                               Target1* target, TargetSignal targetSignal) {
  using Target2 = typename PropertyHelperGetFunPointerClass<TargetSignal>::type;
  auto lambda = []() {};
  new ForwardSignalFromPropertyHelper<
      RealType, Target2, TargetSignal,
      typename std::decay<decltype(lambda)>::type, Pars...>(
      node, property, signal, target, targetSignal, lambda);
}

/**
 * Similar to forwardSignalFromProperty(), but also invoke targetSignal
 * when the connection changes. This means that the target signal must not have
 * any arguments.
 */
template <typename RealType, typename Target1, typename TargetSignal,
          typename... Pars>
void forwardSignalFromPropertyOnReconnect(
    Node* node, const QSharedPointer<NodeProperty>& property,
    void (RealType::*signal)(Pars...), Target1* target,
    TargetSignal targetSignal, bool invokeNow = false) {
  using Target2 = typename PropertyHelperGetFunPointerClass<TargetSignal>::type;
  QPointer<Target2> targetP = target;
  auto lambda = [targetP, targetSignal]() {
    if (!targetP) return;
    QObject obj;
    QObject::connect(&obj, &QObject::destroyed, targetP, targetSignal);
  };  // obj will be destroyed here
  new ForwardSignalFromPropertyHelper<
      RealType, Target2, TargetSignal,
      typename std::decay<decltype(lambda)>::type, Pars...>(
      node, property, signal, target, targetSignal, lambda);
  if (invokeNow) lambda();
}
}  // namespace vx
