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

#include <Voxie/Voxie.hpp>

#include <Voxie/Node/NodeKind.hpp>
#include <Voxie/Node/NodeProperty.hpp>

#include <VoxieBackend/DBus/DynamicObject.hpp>

#include <VoxieClient/DBusTypeList.hpp>

#include <QtWidgets/QAction>

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <Voxie/Data/Color.hpp>
#include <Voxie/Data/ColorInterpolator.hpp>
#include <Voxie/Data/ColorizerEntry.hpp>
#include <Voxie/Data/PropertySection.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/PropertyUI.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>
#include <Voxie/Node/Types.hpp>

#include <Voxie/IVoxie.hpp>

#include <QtCore/QDebug>

#include <QtGui/QIcon>

#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

// A pointer which always has to be initialized
template <typename T>
class InitializedPointer {
  T* data;

 public:
  InitializedPointer(T* ptr) : data(ptr) {
    if (!ptr)
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "InitializedPointer set to nullptr");
  }

  T& operator*() const noexcept { return *data; }
  T* operator->() const noexcept { return data; }

  operator T*() const { return data; }
};

#if 1
#define VX_NODE_INITIALIZED_POINTER(T) InitializedPointer<T>
#else
#define VX_NODE_INITIALIZED_POINTER(T) T*
#endif

// VX_NODE_INSTANTIATION is in NodePrototype.hpp

#define VX_NODE_IMPLEMENTATION(name)                                    \
 private:                                                               \
  VX_DECLARE_STRING_CONSTANT_CLASS(NameType, name);                     \
  using PropertiesTypeAlias = ::vx::PropertiesTypeAlias<NameType>;      \
                                                                        \
 public:                                                                \
  static QSharedPointer<vx::NodePrototype> getPrototypeSingleton();     \
  using PropertiesType = PropertiesTypeAlias::PropertiesType;           \
  using PropertiesEntryType = PropertiesTypeAlias::PropertiesEntryType; \
                                                                        \
 private:                                                               \
  VX_NODE_INITIALIZED_POINTER(PropertiesType) properties;

// TODO: Avoid using this?

#define VX_NODE_IMPLEMENTATION_PUB(name)                                \
 private:                                                               \
  VX_DECLARE_STRING_CONSTANT_CLASS(NameType, name);                     \
  using PropertiesTypeAlias = ::vx::PropertiesTypeAlias<NameType>;      \
                                                                        \
 public:                                                                \
  static QSharedPointer<vx::NodePrototype> getPrototypeSingleton();     \
  using PropertiesType = PropertiesTypeAlias::PropertiesType;           \
  using PropertiesEntryType = PropertiesTypeAlias::PropertiesEntryType; \
                                                                        \
  VX_NODE_INITIALIZED_POINTER(PropertiesType) properties;               \
                                                                        \
 private:

namespace vx {
class NodePrototype;
class PropertySection;
class NodeProperty;
class NodeGroup;
}  // namespace vx
namespace vx {

class VOXIECORESHARED_EXPORT Node : public vx::DynamicObject {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  static bool showWarningOnOldObjectNames();

  // Note: Not all the states are fully implemented
  enum class State : uint8_t {
    /**
     * Initial state. Node is not in global node list. Node cannot be used
     * as a value in other node's properties.
     */
    Initial,

    /**
     * Setup state. Node is in global node list. Node can now be used
     * as a value in other node's properties.
     *
     * Node might not yet have all properties set to their correct value.
     */
    Setup,

    /**
     * Normal state. Node can be used normally.
     */
    Normal,

    /**
     * Teardown state. Node is still in global node list. Node cannot be
     * added to other node's properties anymore, but might still be there for
     * some nodes.
     */
    Teardown,

    /**
     * State after teardown. Node is no longer in global node list. Node
     * is not in any other node's properties. The only reason the node is
     * not yet deallocated is because there are some references to it.
     */
    Destroyed,
  };
  static QString stateToString(State state);

 private:
  PropertySection* exportedPropertiesSection = nullptr;
  State state_ = State::Initial;

  bool displayNameIsSetManually = false;
  QString manualDisplayName_;
  QString automaticDisplayName_;

  QPointer<NodeGroup> parentNodeGroup_;

  QList<QSharedPointer<NodeProperty>> exportedProperties_;

  QList<QPointer<PropertySection>> sections;

  QList<QWidget*> propertySections_;
  int defaultSectionPosition_ = -1;

  QMap<QString, QVariant>
      propertyValues_;  // For properties without custom getter/setter

  // List of nodes which have properties pointing to this node as an input
  QList<QPointer<Node>> referencingNodesAsInput;

  // List of nodes which have properties pointing to this node as an output
  QList<QPointer<Node>> referencingNodesAsOutput;

  QSharedPointer<NodePrototype> prototype_;

  QList<QAction*> contextMenuActions_;

  void removeChildNode(Node* obj, bool emitSignalsForThis,
                       bool emitSignalsForObj);
  void removeChildNode(Node* obj, int index, bool emitSignalsForThis,
                       bool emitSignalsForObj);

  void setNodeProperty(const QSharedPointer<NodeProperty>& property,
                       const QVariant& value, bool emitSignalsForThis);
  void setNodeProperty(NodeProperty* property, const QVariant& value,
                       bool emitSignalsForThis);
  void setNodeProperty(const QString& key, const QVariant& value,
                       bool emitSignalsForThis);

 public:
  explicit Node(const QString& type,
                const QSharedPointer<NodePrototype>& prototype);
  ~Node() override;

  virtual QWidget* getCustomPropertySectionContent(const QString& name);

  // QSharedPointer<NodePrototype> getPrototypeSingleton();

  State state() const { return state_; }

  QList<Node*> parentNodes();
  QList<Node*> childNodes();
  void addChildNode(Node* obj,
                    int slot = 0);  // TODO: should be moved somewhere
                                    // else (does GUI stuff now)
  void removeChildNode(Node* obj, int index = -1);

  /**
   * Return all properties, including custom storage ones.
   */
  QMap<QString, QVariant> propertyValues();

  /**
   * @brief Returns the name of this filter node (e.g. for displaying it in
   * the gui).
   * @return The human readable name.
   */
  QString displayName() const;

  /**
   * @brief destroy This methods destroys the given node instance
   */
  void destroy();

  const QString automaticDisplayName() const { return automaticDisplayName_; }
  std::tuple<bool, QString> manualDisplayName() const {
    return std::make_tuple(displayNameIsSetManually, manualDisplayName_);
  }

  /**
   * @brief setDisplayName Sets the automatic displayName of the Node
   */
  void setAutomaticDisplayName(const QString& name);

  /**
   * @brief setDisplayName Sets the manual displayName of the Node
   */
  void setManualDisplayName(const std::tuple<bool, QString>& name);

  QPointer<NodeGroup> parentNodeGroup() const;
  void setParentNodeGroup(const QPointer<NodeGroup> nodeGroup);

  QList<QSharedPointer<NodeProperty>> exportedProperties() const;
  void setExportedProperties(
      const QList<QSharedPointer<NodeProperty>>& properties);

  QIcon icon();

  QList<QWidget*> propertySections() const { return propertySections_; }
  void addPropertySection(QWidget* section, int position = -1,
                          bool isInitiallyExpanded = true);

  // TODO: should this be moved to NodePrototype?
  const QList<QAction*>& contextMenuActions() const {
    return contextMenuActions_;
  }
  void addContextMenuAction(QAction* action);

  /**
   * @brief isAllowedChild returns true if the parameter NodeKind is a valid
   * child of the Node.
   */
  virtual bool isAllowedChild(NodeKind node) = 0;

  /**
   * @brief isAllowedParent returns true if the parameter NodeKind is a valid
   * parent of the Node.
   */
  virtual bool isAllowedParent(NodeKind node) = 0;

  /**
   * @brief isCreatableChild returns true if the parameter NodeKind is a
   * Node creatable by the user. Some NodeKinds can't be created by user but
   * by scripts, like FilterNode -> DataNode.
   */
  virtual bool isCreatableChild(NodeKind node);

  /**
   * @brief hasAncestor returns true if parameter node is an ancestor of the
   * Node.
   */
  bool hasAncestor(Node* node);

  /**
   * @brief hasParent returns true only if parameter node is the parent of the
   * Node.
   */
  bool hasParent(Node* node);

  /**
   * @brief hasParent returns true only if parameter node is the child of the
   * Node.
   */
  bool hasChild(Node* node);

  NodeKind nodeKind();

  // Note: This is called getNodeProperty()/setNodeProperty() instead of
  // getProperty()/setProperty() to prevent QObject::setProperty() from being
  // hidden

  QVariant getNodeProperty(const QSharedPointer<NodeProperty>& property);
  QVariant getNodeProperty(const QString& key);

  void setNodeProperty(const QSharedPointer<NodeProperty>& property,
                       const QVariant& value);
  void setNodeProperty(const QString& key, const QVariant& value);

  template <typename T>
  typename T::QtType getNodeProperty(const NodePropertyTyped<T>& property) {
    return getNodePropertyTyped<typename T::QtType>(property.property());
  }
  template <typename T>
  void setNodeProperty(const NodePropertyTyped<T>& property,
                       const typename T::QtType& value) {
    setNodePropertyTyped<typename T::QtType>(property.property(), value);
  }

  /**
   * Set multiple properties at once.
   *
   * In some cases the property setters have to be called in a certain order,
   * this function will make sure they will be called in the right order.
   */
  void setNodeProperties(const QMap<QString, QVariant>& values);

  /**
   * @brief Returns a dynamically-typed data node associated with the
   * specified property. This function serves as a general data-passing
   * mechanism to display contextual data in a property UI widget.
   *
   * As an example, see Colorizer, which obtains a HistogramProvider node
   * from SliceVisualizer.
   *
   * @param propertyName The fully qualified name of the property making the
   * request.
   * @return A null-pointer by default.
   */
  virtual QSharedPointer<QObject> getPropertyUIData(QString propertyName);

 protected:
  /**
   * @brief GetNodeProperty getter for the single fields of this node which
   * should be availbale to external scripts (especially for save and loading
   * functionality) Throws a Exception if a field is queried which is
   * not available. Make sure to call super method when overwriting this
   * function.
   * @param key string identiifier for the field to return
   */
  virtual QVariant getNodePropertyCustom(QString key);

  /**
   * @brief SetNodeProperty setter for the single fields of this node which
   * should be availbale to external scripts (especially for save and loading
   * functionality) Throws a Exception if a field is queried which is
   * not available or the corresponding type does not fit to the field. Make
   * sure to call super method when overwriting this function.
   * @param key string identiifier for the field to return
   * @param value new value for the corresponding field
   */
  virtual void setNodePropertyCustom(QString key, QVariant value);

  void emitCustomPropertyChanged(const QSharedPointer<NodeProperty>& property);

 private:
  void initializeReal();

  // bool to switch GraphWidget visiblity for the object and it's graph node
  bool graphVisibility = true;
  // variable to give objects custom ui via specific QWidgets which will be
  // displayed in the voxie sidepanel
  QWidget* customUi = nullptr;

 public:
  void initialize() override;

  void addNodePropertyUIToSection(Node* self, PropertySection* section,
                                  const QSharedPointer<NodeProperty>& property,
                                  const QJsonObject& json);

  // Should only be called by NodePrototype class
  void setStateToSetup();
  void setStateToNormal();

  void setCustomUi(QWidget* widget) { this->customUi = widget; }
  QWidget* getCustomUi() const { return this->customUi; }
  void setGraphVisibility(bool visible) { this->graphVisibility = visible; }
  bool getGraphVisibility() const { return this->graphVisibility; }

  void parentNodeGroupStateChanged(State state);

 protected:
  // TODO: Somehow also allow delaying some stuff when SetProperties() is
  // called?
  /**
   * This function will be called after all properties have been set.
   */
  virtual void setupFinished();

 public:
  const QSharedPointer<NodePrototype>& prototype() { return prototype_; }

  template <typename T>
  T getNodePropertyTyped(const QString& key) {
    auto value = getNodeProperty(key);
    // TODO: Should canConvert or equality of types be used here?
    return parseVariant<T>(value);
    /*
    if (!value.canConvert<T>())
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          "Cannot convert property '" + key + "' to correct type");
    return value.value<T>();
    */
  }
  template <typename T>
  T getNodePropertyTyped(const QSharedPointer<NodeProperty>& property) {
    auto value = getNodeProperty(property);
    // TODO: Should canConvert or equality of types be used here?
    return parseVariant<T>(value);
    /*
    if (!value.canConvert<T>())
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.Error",
          "Cannot convert property '" + key + "' to correct type");
    return value.value<T>();
    */
  }
  template <typename T>
  void setNodePropertyTyped(const QSharedPointer<NodeProperty>& property,
                            const T& value) {
    setNodeProperty(property, QVariant::fromValue(value));
  }
  template <typename T>
  void setNodePropertyTyped(const QString& key, const T& value) {
    setNodeProperty(key, QVariant::fromValue(value));
  }

 Q_SIGNALS:
  void stateChanged(State newState);

  void displayNameChanged(const QString& newName);
  void propertySectionAdded(QWidget* section);

  void exportedPropertiesChanged(
      const QList<QSharedPointer<NodeProperty>>& exportedProperties);

  void parentNodeGroupChanged(NodeGroup* newNodeGroup);

  /**
   * @brief parentChanged This signal is emitted when a parent is added to or
   * removed from the current node.
   * @param parent The added/removed parent.
   */
  void parentChanged(Node* parent);

  /**
   * @brief childChanged This signal is emitted when a child is added to or
   * removed from the current node.
   * @param child The added/removed child.
   */
  void childChanged(Node* child);

  /**
   * @brief propertyChanged This signal is emitted when a property value is
   * changed.
   * @param property The property which was changed
   * @param value The new value of the property
   */
  void propertyChanged(const QSharedPointer<NodeProperty>& property,
                       const QVariant& value);

 public:
  static QVariant propertyDBusToRaw(
      // TODO: This should be a shared pointer (and NodeProperty should
      // inherit from RefCountedObject)
      // const QSharedPointer<NodePrototype>& prototype,
      NodePrototype* prototype, const QSharedPointer<NodeProperty>& property,
      const QDBusVariant& value);

  template <typename T>
  static T parseVariant(const QVariant& variant) {
    if (variant.userType() != qMetaTypeId<T>())
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      QString() + "Got invalid type: Expected " +
                          QMetaType::typeName(qMetaTypeId<T>()) + ", got " +
                          QMetaType::typeName(variant.userType()));
    return variant.value<T>();
  }

  // TODO: Return shared pointers?
  static Node* parseVariantNode(const QVariant& variant);
  static QList<Node*> parseVariantNodeList(const QVariant& variant);

  static QVariant getVariant(Node* obj);
  static QVariant getVariant(const QList<Node*>& objs);
};

namespace Prop {}
namespace PropType {}
class VOXIECORESHARED_EXPORT PropTypeBase {};
class VOXIECORESHARED_EXPORT PropertiesEntryBase {
  QString name_;
  QVariant value_;

 public:
  PropertiesEntryBase(const QString& name, const QVariant& value);
  ~PropertiesEntryBase();

  const QString& name() const { return name_; }
  const QVariant& value() const { return value_; }
};

template <typename T>
QSharedPointer<T> createNode(
    std::initializer_list<typename T::PropertiesEntryType> properties = {}) {
  static_assert(
      std::is_base_of<PropertiesEntryBase,
                      typename T::PropertiesEntryType>::value,
      "T::PropertiesEntryType must have base vx::PropertiesEntryBase");

  QMap<QString, QVariant> propertiesRes;
  for (const auto& entry : properties) {
    const PropertiesEntryBase& entry2 = entry;
    const auto& name = entry2.name();

    // With some more template magic this probably could be detected statically,
    // but that's probably too much effort
    if (propertiesRes.contains(name))
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InternalError",
          "Got property name multiple times in createNode<>(): " + name);

    propertiesRes[name] = entry2.value();
  }
  auto res = qSharedPointerDynamicCast<T>(
      T::getPrototypeSingleton()->create(propertiesRes, {}, {}));
  if (!res)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Cannot cast result of NodePrototype::create()");
  return res;
}
}  // namespace vx
