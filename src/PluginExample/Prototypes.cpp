// This file was automatically generated by tools/update-node-prototypes.py
// All changes to this file will be lost

#include "Prototypes.hpp"

#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/PropertyValueConvertDBus.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>
namespace vx {
inline namespace filter_prop {
TheSphereGeneratorPropertiesEntry::~TheSphereGeneratorPropertiesEntry() {}
TheSphereGeneratorPropertiesEntry::TheSphereGeneratorPropertiesEntry(
    vx::PropType::Seed, qint64 value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Seed",
          QVariant::fromValue<qint64>(
              vx::PropertyValueConvertRaw<qint64, qint64>::toRaw(value_))) {}
TheSphereGeneratorPropertiesEntry::TheSphereGeneratorPropertiesEntry(
    vx::PropType::Size, qint64 value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Size",
          QVariant::fromValue<qint64>(
              vx::PropertyValueConvertRaw<qint64, qint64>::toRaw(value_))) {}
TheSphereGeneratorPropertiesEntry::TheSphereGeneratorPropertiesEntry(
    vx::PropType::Output, vx::Node* value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Output",
          QVariant::fromValue<QDBusObjectPath>(
              vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::toRaw(
                  value_))) {}
TheSphereGeneratorPropertiesBase::~TheSphereGeneratorPropertiesBase() {}
TheSphereGeneratorPropertiesCopy::TheSphereGeneratorPropertiesCopy(
    const QSharedPointer<const QMap<QString, QVariant>>& properties)
    : _properties(properties) {}
qint64 TheSphereGeneratorPropertiesCopy::seed() {
  return vx::PropertyValueConvertRaw<qint64, qint64>::fromRaw(
      vx::Node::parseVariant<qint64>(
          (*_properties)["de.uni_stuttgart.Voxie.Example.Filter."
                         "TheSphereGenerator.Seed"]));
}
qint64 TheSphereGeneratorPropertiesCopy::seedRaw() {
  return vx::Node::parseVariant<qint64>(
      (*_properties)
          ["de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Seed"]);
}
qint64 TheSphereGeneratorPropertiesCopy::size() {
  return vx::PropertyValueConvertRaw<qint64, qint64>::fromRaw(
      vx::Node::parseVariant<qint64>(
          (*_properties)["de.uni_stuttgart.Voxie.Example.Filter."
                         "TheSphereGenerator.Size"]));
}
qint64 TheSphereGeneratorPropertiesCopy::sizeRaw() {
  return vx::Node::parseVariant<qint64>(
      (*_properties)
          ["de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Size"]);
}
vx::Node* TheSphereGeneratorPropertiesCopy::output() {
  return vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
      vx::Node::parseVariant<QDBusObjectPath>(
          (*_properties)["de.uni_stuttgart.Voxie.Output"]));
}
QDBusObjectPath TheSphereGeneratorPropertiesCopy::outputRaw() {
  return vx::Node::parseVariant<QDBusObjectPath>(
      (*_properties)["de.uni_stuttgart.Voxie.Output"]);
}
static const char _prototype_TheSphereGenerator_[] = {
    123, 34,  67,  111, 109, 112, 97,  116, 105, 98,  105, 108, 105, 116, 121,
    78,  97,  109, 101, 115, 34,  58,  32,  91,  34,  100, 101, 46,  117, 110,
    105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120,
    105, 101, 46,  69,  120, 97,  109, 112, 108, 101, 46,  84,  104, 101, 83,
    112, 104, 101, 114, 101, 71,  101, 110, 101, 114, 97,  116, 111, 114, 34,
    93,  44,  32,  34,  68,  101, 115, 99,  114, 105, 112, 116, 105, 111, 110,
    34,  58,  32,  34,  71,  101, 110, 101, 114, 97,  116, 101, 115, 32,  97,
    32,  98,  108, 117, 114, 114, 121, 32,  115, 112, 104, 101, 114, 101, 34,
    44,  32,  34,  68,  105, 115, 112, 108, 97,  121, 78,  97,  109, 101, 34,
    58,  32,  34,  84,  104, 101, 32,  115, 112, 104, 101, 114, 101, 32,  103,
    101, 110, 101, 114, 97,  116, 111, 114, 34,  44,  32,  34,  78,  97,  109,
    101, 34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117,
    116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  69,  120,
    97,  109, 112, 108, 101, 46,  70,  105, 108, 116, 101, 114, 46,  84,  104,
    101, 83,  112, 104, 101, 114, 101, 71,  101, 110, 101, 114, 97,  116, 111,
    114, 34,  44,  32,  34,  78,  111, 100, 101, 75,  105, 110, 100, 34,  58,
    32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103,
    97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  78,  111, 100, 101, 75,
    105, 110, 100, 46,  70,  105, 108, 116, 101, 114, 34,  44,  32,  34,  80,
    114, 111, 112, 101, 114, 116, 105, 101, 115, 34,  58,  32,  123, 34,  100,
    101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116,
    46,  86,  111, 120, 105, 101, 46,  69,  120, 97,  109, 112, 108, 101, 46,
    70,  105, 108, 116, 101, 114, 46,  84,  104, 101, 83,  112, 104, 101, 114,
    101, 71,  101, 110, 101, 114, 97,  116, 111, 114, 46,  83,  101, 101, 100,
    34,  58,  32,  123, 34,  67,  111, 109, 112, 97,  116, 105, 98,  105, 108,
    105, 116, 121, 78,  97,  109, 101, 115, 34,  58,  32,  91,  34,  100, 101,
    46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,
    86,  111, 120, 105, 101, 46,  69,  120, 97,  109, 112, 108, 101, 46,  84,
    104, 101, 83,  112, 104, 101, 114, 101, 71,  101, 110, 101, 114, 97,  116,
    111, 114, 46,  83,  101, 101, 100, 34,  93,  44,  32,  34,  68,  101, 102,
    97,  117, 108, 116, 86,  97,  108, 117, 101, 34,  58,  32,  49,  51,  51,
    55,  44,  32,  34,  68,  105, 115, 112, 108, 97,  121, 78,  97,  109, 101,
    34,  58,  32,  34,  83,  101, 101, 100, 34,  44,  32,  34,  77,  97,  120,
    105, 109, 117, 109, 86,  97,  108, 117, 101, 34,  58,  32,  52,  50,  57,
    52,  57,  54,  55,  50,  57,  53,  44,  32,  34,  77,  105, 110, 105, 109,
    117, 109, 86,  97,  108, 117, 101, 34,  58,  32,  48,  44,  32,  34,  84,
    121, 112, 101, 34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115,
    116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,
    80,  114, 111, 112, 101, 114, 116, 121, 84,  121, 112, 101, 46,  73,  110,
    116, 34,  125, 44,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116,
    117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  69,
    120, 97,  109, 112, 108, 101, 46,  70,  105, 108, 116, 101, 114, 46,  84,
    104, 101, 83,  112, 104, 101, 114, 101, 71,  101, 110, 101, 114, 97,  116,
    111, 114, 46,  83,  105, 122, 101, 34,  58,  32,  123, 34,  67,  111, 109,
    112, 97,  116, 105, 98,  105, 108, 105, 116, 121, 78,  97,  109, 101, 115,
    34,  58,  32,  91,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117,
    116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  69,  120,
    97,  109, 112, 108, 101, 46,  84,  104, 101, 83,  112, 104, 101, 114, 101,
    71,  101, 110, 101, 114, 97,  116, 111, 114, 46,  83,  105, 122, 101, 34,
    93,  44,  32,  34,  68,  101, 102, 97,  117, 108, 116, 86,  97,  108, 117,
    101, 34,  58,  32,  49,  50,  57,  44,  32,  34,  68,  105, 115, 112, 108,
    97,  121, 78,  97,  109, 101, 34,  58,  32,  34,  83,  105, 122, 101, 34,
    44,  32,  34,  77,  97,  120, 105, 109, 117, 109, 86,  97,  108, 117, 101,
    34,  58,  32,  57,  57,  57,  57,  44,  32,  34,  77,  105, 110, 105, 109,
    117, 109, 86,  97,  108, 117, 101, 34,  58,  32,  48,  44,  32,  34,  84,
    121, 112, 101, 34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115,
    116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,
    80,  114, 111, 112, 101, 114, 116, 121, 84,  121, 112, 101, 46,  73,  110,
    116, 34,  125, 44,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116,
    117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  79,
    117, 116, 112, 117, 116, 34,  58,  32,  123, 34,  65,  108, 108, 111, 119,
    101, 100, 78,  111, 100, 101, 80,  114, 111, 116, 111, 116, 121, 112, 101,
    115, 34,  58,  32,  91,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116,
    117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  68,
    97,  116, 97,  46,  86,  111, 108, 117, 109, 101, 34,  93,  44,  32,  34,
    68,  105, 115, 112, 108, 97,  121, 78,  97,  109, 101, 34,  58,  32,  34,
    79,  117, 116, 112, 117, 116, 34,  44,  32,  34,  84,  121, 112, 101, 34,
    58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116,
    103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  80,  114, 111, 112,
    101, 114, 116, 121, 84,  121, 112, 101, 46,  79,  117, 116, 112, 117, 116,
    78,  111, 100, 101, 82,  101, 102, 101, 114, 101, 110, 99,  101, 34,  125,
    125, 44,  32,  34,  82,  117, 110, 70,  105, 108, 116, 101, 114, 69,  110,
    97,  98,  108, 101, 100, 67,  111, 110, 100, 105, 116, 105, 111, 110, 34,
    58,  32,  123, 34,  84,  121, 112, 101, 34,  58,  32,  34,  100, 101, 46,
    117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,
    111, 120, 105, 101, 46,  80,  114, 111, 112, 101, 114, 116, 121, 67,  111,
    110, 100, 105, 116, 105, 111, 110, 46,  84,  114, 117, 101, 34,  125, 125,
    0};
const char* TheSphereGeneratorProperties::_getPrototypeJson() {
  return _prototype_TheSphereGenerator_;
}

TheSphereGeneratorProperties::~TheSphereGeneratorProperties() {}

qint64 TheSphereGeneratorProperties::seed() {
  return vx::PropertyValueConvertRaw<qint64, qint64>::fromRaw(
      _node->getNodePropertyTyped<qint64>(
          "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Seed"));
}
qint64 TheSphereGeneratorProperties::seedRaw() {
  return _node->getNodePropertyTyped<qint64>(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Seed");
}
QSharedPointer<NodeProperty> TheSphereGeneratorProperties::seedProperty() {
  return _node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Seed", false);
}
void TheSphereGeneratorProperties::setSeed(qint64 value) {
  _node->setNodePropertyTyped<qint64>(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Seed",
      vx::PropertyValueConvertRaw<qint64, qint64>::toRaw(value));
}
qint64 TheSphereGeneratorProperties::size() {
  return vx::PropertyValueConvertRaw<qint64, qint64>::fromRaw(
      _node->getNodePropertyTyped<qint64>(
          "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Size"));
}
qint64 TheSphereGeneratorProperties::sizeRaw() {
  return _node->getNodePropertyTyped<qint64>(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Size");
}
QSharedPointer<NodeProperty> TheSphereGeneratorProperties::sizeProperty() {
  return _node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Size", false);
}
void TheSphereGeneratorProperties::setSize(qint64 value) {
  _node->setNodePropertyTyped<qint64>(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Size",
      vx::PropertyValueConvertRaw<qint64, qint64>::toRaw(value));
}
vx::Node* TheSphereGeneratorProperties::output() {
  return vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
      _node->getNodePropertyTyped<QDBusObjectPath>(
          "de.uni_stuttgart.Voxie.Output"));
}
QDBusObjectPath TheSphereGeneratorProperties::outputRaw() {
  return _node->getNodePropertyTyped<QDBusObjectPath>(
      "de.uni_stuttgart.Voxie.Output");
}
QSharedPointer<NodeProperty> TheSphereGeneratorProperties::outputProperty() {
  return _node->prototype()->getProperty("de.uni_stuttgart.Voxie.Output",
                                         false);
}
void TheSphereGeneratorProperties::setOutput(vx::Node* value) {
  _node->setNodePropertyTyped<QDBusObjectPath>(
      "de.uni_stuttgart.Voxie.Output",
      vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::toRaw(value));
}
TheSphereGeneratorProperties::TheSphereGeneratorProperties(vx::Node* parent)
    : QObject(parent) {
  this->_node = parent;
  auto _prop_Seed = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Seed", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_Seed](const QSharedPointer<NodeProperty>& property,
                         const QVariant& value) {
        if (property != _prop_Seed) return;
        qint64 valueCasted;
        try {
          valueCasted = vx::PropertyValueConvertRaw<qint64, qint64>::fromRaw(
              Node::parseVariant<qint64>(value));
        } catch (vx::Exception& e) {
          qCritical() << "Error while parsing property value for event handler "
                         "for property "
                         "\"de.uni_stuttgart.Voxie.Example.Filter."
                         "TheSphereGenerator.Seed\":"
                      << e.what();
          return;
        }
        Q_EMIT this->seedChanged(valueCasted);
      });
  auto _prop_Size = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator.Size", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_Size](const QSharedPointer<NodeProperty>& property,
                         const QVariant& value) {
        if (property != _prop_Size) return;
        qint64 valueCasted;
        try {
          valueCasted = vx::PropertyValueConvertRaw<qint64, qint64>::fromRaw(
              Node::parseVariant<qint64>(value));
        } catch (vx::Exception& e) {
          qCritical() << "Error while parsing property value for event handler "
                         "for property "
                         "\"de.uni_stuttgart.Voxie.Example.Filter."
                         "TheSphereGenerator.Size\":"
                      << e.what();
          return;
        }
        Q_EMIT this->sizeChanged(valueCasted);
      });
  auto _prop_Output = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Output", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_Output](const QSharedPointer<NodeProperty>& property,
                           const QVariant& value) {
        if (property != _prop_Output) return;
        vx::Node* valueCasted;
        try {
          valueCasted =
              vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
                  Node::parseVariant<QDBusObjectPath>(value));
        } catch (vx::Exception& e) {
          qCritical() << "Error while parsing property value for event handler "
                         "for property \"de.uni_stuttgart.Voxie.Output\":"
                      << e.what();
          return;
        }
        Q_EMIT this->outputChanged(valueCasted);
      });
}

}  // namespace filter_prop
inline namespace visualizer_prop {
RandomChartPropertiesEntry::~RandomChartPropertiesEntry() {}
RandomChartPropertiesBase::~RandomChartPropertiesBase() {}
RandomChartPropertiesCopy::RandomChartPropertiesCopy(
    const QSharedPointer<const QMap<QString, QVariant>>& properties)
    : _properties(properties) {}
static const char _prototype_RandomChart_[] = {
    123, 34,  67,  111, 109, 112, 97,  116, 105, 98,  105, 108, 105, 116, 121,
    78,  97,  109, 101, 115, 34,  58,  32,  91,  34,  100, 101, 46,  117, 110,
    105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120,
    105, 101, 46,  69,  120, 97,  109, 112, 108, 101, 86,  105, 115, 117, 97,
    108, 105, 122, 101, 114, 34,  93,  44,  32,  34,  68,  101, 115, 99,  114,
    105, 112, 116, 105, 111, 110, 34,  58,  32,  34,  68,  105, 115, 112, 108,
    97,  121, 32,  115, 111, 109, 101, 32,  114, 97,  110, 100, 111, 109, 32,
    99,  104, 97,  114, 116, 115, 34,  44,  32,  34,  68,  105, 115, 112, 108,
    97,  121, 78,  97,  109, 101, 34,  58,  32,  34,  69,  120, 97,  109, 112,
    108, 101, 32,  86,  105, 115, 117, 97,  108, 105, 122, 101, 114, 34,  44,
    32,  34,  73,  99,  111, 110, 34,  58,  32,  34,  58,  47,  105, 99,  111,
    110, 115, 47,  101, 113, 117, 97,  108, 105, 122, 101, 114, 46,  112, 110,
    103, 34,  44,  32,  34,  78,  97,  109, 101, 34,  58,  32,  34,  100, 101,
    46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,
    86,  111, 120, 105, 101, 46,  69,  120, 97,  109, 112, 108, 101, 46,  86,
    105, 115, 117, 97,  108, 105, 122, 101, 114, 46,  82,  97,  110, 100, 111,
    109, 67,  104, 97,  114, 116, 34,  44,  32,  34,  78,  111, 100, 101, 75,
    105, 110, 100, 34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115,
    116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,
    78,  111, 100, 101, 75,  105, 110, 100, 46,  86,  105, 115, 117, 97,  108,
    105, 122, 101, 114, 34,  44,  32,  34,  80,  114, 111, 112, 101, 114, 116,
    105, 101, 115, 34,  58,  32,  123, 125, 125, 0};
const char* RandomChartProperties::_getPrototypeJson() {
  return _prototype_RandomChart_;
}

RandomChartProperties::~RandomChartProperties() {}

RandomChartProperties::RandomChartProperties(vx::Node* parent)
    : QObject(parent) {
  this->_node = parent;
}

}  // namespace visualizer_prop
}  // namespace vx
