// This file was automatically generated by tools/update-node-prototypes.py
// All changes to this file will be lost

#include "Prototypes.hpp"

#include <Voxie/Node/NodeNodeProperty.hpp>
#include <Voxie/Node/NodePrototype.hpp>
#include <Voxie/Node/PropertyValueConvertDBus.hpp>
#include <Voxie/Node/PropertyValueConvertRaw.hpp>
namespace vx {
inline namespace filter_prop {
FitPlanePropertiesEntry::~FitPlanePropertiesEntry() {}
FitPlanePropertiesEntry::FitPlanePropertiesEntry(
    vx::PropType::GeometricPrimitive, vx::Node* value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.GeometricPrimitive",
          QVariant::fromValue<QDBusObjectPath>(
              vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::toRaw(
                  value_))) {}
FitPlanePropertiesEntry::FitPlanePropertiesEntry(vx::PropType::MaximumDistance,
                                                 double value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance",
          QVariant::fromValue<double>(
              vx::PropertyValueConvertRaw<double, double>::toRaw(value_))) {}
FitPlanePropertiesEntry::FitPlanePropertiesEntry(vx::PropType::Point1,
                                                 quint64 value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.Point1",
          QVariant::fromValue<quint64>(
              vx::PropertyValueConvertRaw<quint64, quint64>::toRaw(value_))) {}
FitPlanePropertiesEntry::FitPlanePropertiesEntry(vx::PropType::Point2,
                                                 quint64 value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.Point2",
          QVariant::fromValue<quint64>(
              vx::PropertyValueConvertRaw<quint64, quint64>::toRaw(value_))) {}
FitPlanePropertiesEntry::FitPlanePropertiesEntry(vx::PropType::Point3,
                                                 quint64 value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.Point3",
          QVariant::fromValue<quint64>(
              vx::PropertyValueConvertRaw<quint64, quint64>::toRaw(value_))) {}
FitPlanePropertiesEntry::FitPlanePropertiesEntry(vx::PropType::Surface,
                                                 vx::Node* value_)
    : vx::PropertiesEntryBase(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.Surface",
          QVariant::fromValue<QDBusObjectPath>(
              vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::toRaw(
                  value_))) {}
FitPlanePropertiesBase::~FitPlanePropertiesBase() {}
FitPlanePropertiesCopy::FitPlanePropertiesCopy(
    const QSharedPointer<const QMap<QString, QVariant>>& properties)
    : _properties(properties) {}
QDBusObjectPath FitPlanePropertiesCopy::geometricPrimitiveRaw() {
  return vx::Node::parseVariant<QDBusObjectPath>(
      (*_properties)
          ["de.uni_stuttgart.Voxie.Filter.FitPlane.GeometricPrimitive"]);
}
double FitPlanePropertiesCopy::maximumDistance() {
  return vx::PropertyValueConvertRaw<double, double>::fromRaw(
      vx::Node::parseVariant<double>(
          (*_properties)
              ["de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance"]));
}
double FitPlanePropertiesCopy::maximumDistanceRaw() {
  return vx::Node::parseVariant<double>(
      (*_properties)["de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance"]);
}
quint64 FitPlanePropertiesCopy::point1() {
  return vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
      vx::Node::parseVariant<quint64>(
          (*_properties)["de.uni_stuttgart.Voxie.Filter.FitPlane.Point1"]));
}
quint64 FitPlanePropertiesCopy::point1Raw() {
  return vx::Node::parseVariant<quint64>(
      (*_properties)["de.uni_stuttgart.Voxie.Filter.FitPlane.Point1"]);
}
quint64 FitPlanePropertiesCopy::point2() {
  return vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
      vx::Node::parseVariant<quint64>(
          (*_properties)["de.uni_stuttgart.Voxie.Filter.FitPlane.Point2"]));
}
quint64 FitPlanePropertiesCopy::point2Raw() {
  return vx::Node::parseVariant<quint64>(
      (*_properties)["de.uni_stuttgart.Voxie.Filter.FitPlane.Point2"]);
}
quint64 FitPlanePropertiesCopy::point3() {
  return vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
      vx::Node::parseVariant<quint64>(
          (*_properties)["de.uni_stuttgart.Voxie.Filter.FitPlane.Point3"]));
}
quint64 FitPlanePropertiesCopy::point3Raw() {
  return vx::Node::parseVariant<quint64>(
      (*_properties)["de.uni_stuttgart.Voxie.Filter.FitPlane.Point3"]);
}
QDBusObjectPath FitPlanePropertiesCopy::surfaceRaw() {
  return vx::Node::parseVariant<QDBusObjectPath>(
      (*_properties)["de.uni_stuttgart.Voxie.Filter.FitPlane.Surface"]);
}
static const char _prototype_FitPlane_[] = {
    123, 34,  67,  111, 109, 112, 97,  116, 105, 98,  105, 108, 105, 116, 121,
    78,  97,  109, 101, 115, 34,  58,  32,  91,  34,  100, 101, 46,  117, 110,
    105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120,
    105, 101, 46,  70,  105, 116, 80,  108, 97,  110, 101, 34,  93,  44,  32,
    34,  68,  101, 115, 99,  114, 105, 112, 116, 105, 111, 110, 34,  58,  32,
    34,  70,  105, 116, 32,  97,  32,  112, 108, 97,  110, 101, 32,  116, 111,
    32,  97,  32,  115, 117, 114, 102, 97,  99,  101, 34,  44,  32,  34,  68,
    105, 115, 112, 108, 97,  121, 78,  97,  109, 101, 34,  58,  32,  34,  70,
    105, 116, 32,  115, 117, 114, 102, 97,  99,  101, 32,  116, 111, 32,  112,
    108, 97,  110, 101, 34,  44,  32,  34,  78,  97,  109, 101, 34,  58,  32,
    34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,
    114, 116, 46,  86,  111, 120, 105, 101, 46,  70,  105, 108, 116, 101, 114,
    46,  70,  105, 116, 80,  108, 97,  110, 101, 34,  44,  32,  34,  78,  111,
    100, 101, 75,  105, 110, 100, 34,  58,  32,  34,  100, 101, 46,  117, 110,
    105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120,
    105, 101, 46,  78,  111, 100, 101, 75,  105, 110, 100, 46,  70,  105, 108,
    116, 101, 114, 34,  44,  32,  34,  80,  114, 111, 112, 101, 114, 116, 105,
    101, 115, 34,  58,  32,  123, 34,  100, 101, 46,  117, 110, 105, 95,  115,
    116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,
    70,  105, 108, 116, 101, 114, 46,  70,  105, 116, 80,  108, 97,  110, 101,
    46,  71,  101, 111, 109, 101, 116, 114, 105, 99,  80,  114, 105, 109, 105,
    116, 105, 118, 101, 34,  58,  32,  123, 34,  65,  108, 108, 111, 119, 101,
    100, 78,  111, 100, 101, 80,  114, 111, 116, 111, 116, 121, 112, 101, 115,
    34,  58,  32,  91,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117,
    116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  68,  97,
    116, 97,  46,  71,  101, 111, 109, 101, 116, 114, 105, 99,  80,  114, 105,
    109, 105, 116, 105, 118, 101, 34,  93,  44,  32,  34,  67,  111, 109, 112,
    97,  116, 105, 98,  105, 108, 105, 116, 121, 78,  97,  109, 101, 115, 34,
    58,  32,  91,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116,
    116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  70,  105, 116,
    80,  108, 97,  110, 101, 46,  71,  101, 111, 109, 101, 116, 114, 105, 99,
    80,  114, 105, 109, 105, 116, 105, 118, 101, 34,  93,  44,  32,  34,  68,
    105, 115, 112, 108, 97,  121, 78,  97,  109, 101, 34,  58,  32,  34,  71,
    101, 111, 109, 101, 116, 114, 105, 99,  32,  112, 114, 105, 109, 105, 116,
    105, 118, 101, 34,  44,  32,  34,  84,  121, 112, 101, 34,  58,  32,  34,
    100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114,
    116, 46,  86,  111, 120, 105, 101, 46,  80,  114, 111, 112, 101, 114, 116,
    121, 84,  121, 112, 101, 46,  78,  111, 100, 101, 82,  101, 102, 101, 114,
    101, 110, 99,  101, 34,  125, 44,  32,  34,  100, 101, 46,  117, 110, 105,
    95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105,
    101, 46,  70,  105, 108, 116, 101, 114, 46,  70,  105, 116, 80,  108, 97,
    110, 101, 46,  77,  97,  120, 105, 109, 117, 109, 68,  105, 115, 116, 97,
    110, 99,  101, 34,  58,  32,  123, 34,  67,  111, 109, 112, 97,  116, 105,
    98,  105, 108, 105, 116, 121, 78,  97,  109, 101, 115, 34,  58,  32,  91,
    34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,
    114, 116, 46,  86,  111, 120, 105, 101, 46,  70,  105, 116, 80,  108, 97,
    110, 101, 46,  77,  97,  120, 105, 109, 117, 109, 68,  105, 115, 116, 97,
    110, 99,  101, 34,  93,  44,  32,  34,  68,  101, 102, 97,  117, 108, 116,
    86,  97,  108, 117, 101, 34,  58,  32,  48,  46,  48,  48,  49,  44,  32,
    34,  68,  105, 115, 112, 108, 97,  121, 78,  97,  109, 101, 34,  58,  32,
    34,  77,  97,  120, 105, 109, 117, 109, 32,  100, 105, 115, 116, 97,  110,
    99,  101, 34,  44,  32,  34,  84,  121, 112, 101, 34,  58,  32,  34,  100,
    101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116,
    46,  86,  111, 120, 105, 101, 46,  80,  114, 111, 112, 101, 114, 116, 121,
    84,  121, 112, 101, 46,  70,  108, 111, 97,  116, 34,  125, 44,  32,  34,
    100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114,
    116, 46,  86,  111, 120, 105, 101, 46,  70,  105, 108, 116, 101, 114, 46,
    70,  105, 116, 80,  108, 97,  110, 101, 46,  80,  111, 105, 110, 116, 49,
    34,  58,  32,  123, 34,  65,  108, 108, 111, 119, 101, 100, 80,  114, 105,
    109, 105, 116, 105, 118, 101, 115, 34,  58,  32,  91,  34,  100, 101, 46,
    117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,
    111, 120, 105, 101, 46,  71,  101, 111, 109, 101, 116, 114, 105, 99,  80,
    114, 105, 109, 105, 116, 105, 118, 101, 46,  80,  111, 105, 110, 116, 34,
    93,  44,  32,  34,  67,  111, 109, 112, 97,  116, 105, 98,  105, 108, 105,
    116, 121, 78,  97,  109, 101, 115, 34,  58,  32,  91,  34,  100, 101, 46,
    117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,
    111, 120, 105, 101, 46,  70,  105, 116, 80,  108, 97,  110, 101, 46,  80,
    111, 105, 110, 116, 49,  34,  93,  44,  32,  34,  68,  105, 115, 112, 108,
    97,  121, 78,  97,  109, 101, 34,  58,  32,  34,  70,  105, 114, 115, 116,
    32,  80,  111, 105, 110, 116, 34,  44,  32,  34,  80,  97,  114, 101, 110,
    116, 80,  114, 111, 112, 101, 114, 116, 121, 34,  58,  32,  34,  100, 101,
    46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,
    86,  111, 120, 105, 101, 46,  70,  105, 108, 116, 101, 114, 46,  70,  105,
    116, 80,  108, 97,  110, 101, 46,  71,  101, 111, 109, 101, 116, 114, 105,
    99,  80,  114, 105, 109, 105, 116, 105, 118, 101, 34,  44,  32,  34,  84,
    121, 112, 101, 34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115,
    116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,
    80,  114, 111, 112, 101, 114, 116, 121, 84,  121, 112, 101, 46,  71,  101,
    111, 109, 101, 116, 114, 105, 99,  80,  114, 105, 109, 105, 116, 105, 118,
    101, 34,  125, 44,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116,
    117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  70,
    105, 108, 116, 101, 114, 46,  70,  105, 116, 80,  108, 97,  110, 101, 46,
    80,  111, 105, 110, 116, 50,  34,  58,  32,  123, 34,  65,  108, 108, 111,
    119, 101, 100, 80,  114, 105, 109, 105, 116, 105, 118, 101, 115, 34,  58,
    32,  91,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116,
    103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  71,  101, 111, 109,
    101, 116, 114, 105, 99,  80,  114, 105, 109, 105, 116, 105, 118, 101, 46,
    80,  111, 105, 110, 116, 34,  93,  44,  32,  34,  67,  111, 109, 112, 97,
    116, 105, 98,  105, 108, 105, 116, 121, 78,  97,  109, 101, 115, 34,  58,
    32,  91,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116,
    103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  70,  105, 116, 80,
    108, 97,  110, 101, 46,  80,  111, 105, 110, 116, 50,  34,  93,  44,  32,
    34,  68,  105, 115, 112, 108, 97,  121, 78,  97,  109, 101, 34,  58,  32,
    34,  83,  101, 99,  111, 110, 100, 32,  80,  111, 105, 110, 116, 34,  44,
    32,  34,  80,  97,  114, 101, 110, 116, 80,  114, 111, 112, 101, 114, 116,
    121, 34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117,
    116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  70,  105,
    108, 116, 101, 114, 46,  70,  105, 116, 80,  108, 97,  110, 101, 46,  71,
    101, 111, 109, 101, 116, 114, 105, 99,  80,  114, 105, 109, 105, 116, 105,
    118, 101, 34,  44,  32,  34,  84,  121, 112, 101, 34,  58,  32,  34,  100,
    101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116,
    46,  86,  111, 120, 105, 101, 46,  80,  114, 111, 112, 101, 114, 116, 121,
    84,  121, 112, 101, 46,  71,  101, 111, 109, 101, 116, 114, 105, 99,  80,
    114, 105, 109, 105, 116, 105, 118, 101, 34,  125, 44,  32,  34,  100, 101,
    46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,
    86,  111, 120, 105, 101, 46,  70,  105, 108, 116, 101, 114, 46,  70,  105,
    116, 80,  108, 97,  110, 101, 46,  80,  111, 105, 110, 116, 51,  34,  58,
    32,  123, 34,  65,  108, 108, 111, 119, 101, 100, 80,  114, 105, 109, 105,
    116, 105, 118, 101, 115, 34,  58,  32,  91,  34,  100, 101, 46,  117, 110,
    105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120,
    105, 101, 46,  71,  101, 111, 109, 101, 116, 114, 105, 99,  80,  114, 105,
    109, 105, 116, 105, 118, 101, 46,  80,  111, 105, 110, 116, 34,  93,  44,
    32,  34,  67,  111, 109, 112, 97,  116, 105, 98,  105, 108, 105, 116, 121,
    78,  97,  109, 101, 115, 34,  58,  32,  91,  34,  100, 101, 46,  117, 110,
    105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120,
    105, 101, 46,  70,  105, 116, 80,  108, 97,  110, 101, 46,  80,  111, 105,
    110, 116, 51,  34,  93,  44,  32,  34,  68,  105, 115, 112, 108, 97,  121,
    78,  97,  109, 101, 34,  58,  32,  34,  84,  104, 105, 114, 100, 32,  80,
    111, 105, 110, 116, 34,  44,  32,  34,  80,  97,  114, 101, 110, 116, 80,
    114, 111, 112, 101, 114, 116, 121, 34,  58,  32,  34,  100, 101, 46,  117,
    110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111,
    120, 105, 101, 46,  70,  105, 108, 116, 101, 114, 46,  70,  105, 116, 80,
    108, 97,  110, 101, 46,  71,  101, 111, 109, 101, 116, 114, 105, 99,  80,
    114, 105, 109, 105, 116, 105, 118, 101, 34,  44,  32,  34,  84,  121, 112,
    101, 34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117,
    116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  80,  114,
    111, 112, 101, 114, 116, 121, 84,  121, 112, 101, 46,  71,  101, 111, 109,
    101, 116, 114, 105, 99,  80,  114, 105, 109, 105, 116, 105, 118, 101, 34,
    125, 44,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116,
    116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  70,  105, 108,
    116, 101, 114, 46,  70,  105, 116, 80,  108, 97,  110, 101, 46,  83,  117,
    114, 102, 97,  99,  101, 34,  58,  32,  123, 34,  65,  108, 108, 111, 119,
    101, 100, 78,  111, 100, 101, 80,  114, 111, 116, 111, 116, 121, 112, 101,
    115, 34,  58,  32,  91,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116,
    117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  68,
    97,  116, 97,  46,  83,  117, 114, 102, 97,  99,  101, 34,  93,  44,  32,
    34,  67,  111, 109, 112, 97,  116, 105, 98,  105, 108, 105, 116, 121, 78,
    97,  109, 101, 115, 34,  58,  32,  91,  34,  100, 101, 46,  117, 110, 105,
    95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,  111, 120, 105,
    101, 46,  70,  105, 116, 80,  108, 97,  110, 101, 46,  83,  117, 114, 102,
    97,  99,  101, 34,  93,  44,  32,  34,  68,  105, 115, 112, 108, 97,  121,
    78,  97,  109, 101, 34,  58,  32,  34,  83,  117, 114, 102, 97,  99,  101,
    34,  44,  32,  34,  84,  121, 112, 101, 34,  58,  32,  34,  100, 101, 46,
    117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,  86,
    111, 120, 105, 101, 46,  80,  114, 111, 112, 101, 114, 116, 121, 84,  121,
    112, 101, 46,  78,  111, 100, 101, 82,  101, 102, 101, 114, 101, 110, 99,
    101, 34,  125, 125, 44,  32,  34,  82,  117, 110, 70,  105, 108, 116, 101,
    114, 69,  110, 97,  98,  108, 101, 100, 67,  111, 110, 100, 105, 116, 105,
    111, 110, 34,  58,  32,  123, 34,  67,  111, 110, 100, 105, 116, 105, 111,
    110, 115, 34,  58,  32,  91,  123, 34,  67,  111, 110, 100, 105, 116, 105,
    111, 110, 34,  58,  32,  123, 34,  80,  114, 111, 112, 101, 114, 116, 121,
    34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116,
    116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  70,  105, 108,
    116, 101, 114, 46,  70,  105, 116, 80,  108, 97,  110, 101, 46,  83,  117,
    114, 102, 97,  99,  101, 34,  44,  32,  34,  84,  121, 112, 101, 34,  58,
    32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103,
    97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  80,  114, 111, 112, 101,
    114, 116, 121, 67,  111, 110, 100, 105, 116, 105, 111, 110, 46,  73,  115,
    69,  109, 112, 116, 121, 34,  125, 44,  32,  34,  84,  121, 112, 101, 34,
    58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116,
    103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  80,  114, 111, 112,
    101, 114, 116, 121, 67,  111, 110, 100, 105, 116, 105, 111, 110, 46,  78,
    111, 116, 34,  125, 44,  32,  123, 34,  67,  111, 110, 100, 105, 116, 105,
    111, 110, 34,  58,  32,  123, 34,  80,  114, 111, 112, 101, 114, 116, 121,
    34,  58,  32,  34,  100, 101, 46,  117, 110, 105, 95,  115, 116, 117, 116,
    116, 103, 97,  114, 116, 46,  86,  111, 120, 105, 101, 46,  70,  105, 108,
    116, 101, 114, 46,  70,  105, 116, 80,  108, 97,  110, 101, 46,  71,  101,
    111, 109, 101, 116, 114, 105, 99,  80,  114, 105, 109, 105, 116, 105, 118,
    101, 34,  44,  32,  34,  84,  121, 112, 101, 34,  58,  32,  34,  100, 101,
    46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,
    86,  111, 120, 105, 101, 46,  80,  114, 111, 112, 101, 114, 116, 121, 67,
    111, 110, 100, 105, 116, 105, 111, 110, 46,  73,  115, 69,  109, 112, 116,
    121, 34,  125, 44,  32,  34,  84,  121, 112, 101, 34,  58,  32,  34,  100,
    101, 46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116,
    46,  86,  111, 120, 105, 101, 46,  80,  114, 111, 112, 101, 114, 116, 121,
    67,  111, 110, 100, 105, 116, 105, 111, 110, 46,  78,  111, 116, 34,  125,
    93,  44,  32,  34,  84,  79,  68,  79,  34,  58,  32,  34,  65,  100, 100,
    32,  99,  111, 110, 100, 105, 116, 105, 111, 110, 32,  116, 104, 97,  116,
    32,  112, 111, 105, 110, 116, 115, 32,  97,  114, 101, 32,  115, 101, 116,
    63,  34,  44,  32,  34,  84,  121, 112, 101, 34,  58,  32,  34,  100, 101,
    46,  117, 110, 105, 95,  115, 116, 117, 116, 116, 103, 97,  114, 116, 46,
    86,  111, 120, 105, 101, 46,  80,  114, 111, 112, 101, 114, 116, 121, 67,
    111, 110, 100, 105, 116, 105, 111, 110, 46,  65,  110, 100, 34,  125, 44,
    32,  34,  84,  114, 111, 118, 101, 67,  108, 97,  115, 115, 105, 102, 105,
    101, 114, 115, 34,  58,  32,  91,  34,  68,  101, 118, 101, 108, 111, 112,
    109, 101, 110, 116, 32,  83,  116, 97,  116, 117, 115, 32,  58,  58,  32,
    52,  32,  45,  32,  66,  101, 116, 97,  34,  93,  125, 0};
const char* FitPlaneProperties::_getPrototypeJson() {
  return _prototype_FitPlane_;
}

FitPlaneProperties::~FitPlaneProperties() {}

vx::Node* FitPlaneProperties::geometricPrimitive() {
  return vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
      _node->getNodePropertyTyped<QDBusObjectPath>(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.GeometricPrimitive"));
}
QDBusObjectPath FitPlaneProperties::geometricPrimitiveRaw() {
  return _node->getNodePropertyTyped<QDBusObjectPath>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.GeometricPrimitive");
}
QSharedPointer<NodeProperty> FitPlaneProperties::geometricPrimitiveProperty() {
  return FitPlaneProperties::getNodePrototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.GeometricPrimitive", false);
}
NodePropertyTyped<vx::types::NodeReference>
FitPlaneProperties::geometricPrimitivePropertyTyped() {
  return NodePropertyTyped<vx::types::NodeReference>(
      geometricPrimitiveProperty());
}
NodeNodeProperty FitPlaneProperties::geometricPrimitiveInstance() {
  return NodeNodeProperty(_node, geometricPrimitiveProperty());
}
void FitPlaneProperties::setGeometricPrimitive(vx::Node* value) {
  _node->setNodePropertyTyped<QDBusObjectPath>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.GeometricPrimitive",
      vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::toRaw(value));
}
double FitPlaneProperties::maximumDistance() {
  return vx::PropertyValueConvertRaw<double, double>::fromRaw(
      _node->getNodePropertyTyped<double>(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance"));
}
double FitPlaneProperties::maximumDistanceRaw() {
  return _node->getNodePropertyTyped<double>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance");
}
QSharedPointer<NodeProperty> FitPlaneProperties::maximumDistanceProperty() {
  return FitPlaneProperties::getNodePrototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance", false);
}
NodePropertyTyped<vx::types::Float>
FitPlaneProperties::maximumDistancePropertyTyped() {
  return NodePropertyTyped<vx::types::Float>(maximumDistanceProperty());
}
NodeNodeProperty FitPlaneProperties::maximumDistanceInstance() {
  return NodeNodeProperty(_node, maximumDistanceProperty());
}
void FitPlaneProperties::setMaximumDistance(double value) {
  _node->setNodePropertyTyped<double>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance",
      vx::PropertyValueConvertRaw<double, double>::toRaw(value));
}
quint64 FitPlaneProperties::point1() {
  return vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
      _node->getNodePropertyTyped<quint64>(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.Point1"));
}
quint64 FitPlaneProperties::point1Raw() {
  return _node->getNodePropertyTyped<quint64>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point1");
}
QSharedPointer<NodeProperty> FitPlaneProperties::point1Property() {
  return FitPlaneProperties::getNodePrototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point1", false);
}
NodePropertyTyped<vx::types::GeometricPrimitive>
FitPlaneProperties::point1PropertyTyped() {
  return NodePropertyTyped<vx::types::GeometricPrimitive>(point1Property());
}
NodeNodeProperty FitPlaneProperties::point1Instance() {
  return NodeNodeProperty(_node, point1Property());
}
void FitPlaneProperties::setPoint1(quint64 value) {
  _node->setNodePropertyTyped<quint64>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point1",
      vx::PropertyValueConvertRaw<quint64, quint64>::toRaw(value));
}
quint64 FitPlaneProperties::point2() {
  return vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
      _node->getNodePropertyTyped<quint64>(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.Point2"));
}
quint64 FitPlaneProperties::point2Raw() {
  return _node->getNodePropertyTyped<quint64>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point2");
}
QSharedPointer<NodeProperty> FitPlaneProperties::point2Property() {
  return FitPlaneProperties::getNodePrototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point2", false);
}
NodePropertyTyped<vx::types::GeometricPrimitive>
FitPlaneProperties::point2PropertyTyped() {
  return NodePropertyTyped<vx::types::GeometricPrimitive>(point2Property());
}
NodeNodeProperty FitPlaneProperties::point2Instance() {
  return NodeNodeProperty(_node, point2Property());
}
void FitPlaneProperties::setPoint2(quint64 value) {
  _node->setNodePropertyTyped<quint64>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point2",
      vx::PropertyValueConvertRaw<quint64, quint64>::toRaw(value));
}
quint64 FitPlaneProperties::point3() {
  return vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
      _node->getNodePropertyTyped<quint64>(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.Point3"));
}
quint64 FitPlaneProperties::point3Raw() {
  return _node->getNodePropertyTyped<quint64>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point3");
}
QSharedPointer<NodeProperty> FitPlaneProperties::point3Property() {
  return FitPlaneProperties::getNodePrototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point3", false);
}
NodePropertyTyped<vx::types::GeometricPrimitive>
FitPlaneProperties::point3PropertyTyped() {
  return NodePropertyTyped<vx::types::GeometricPrimitive>(point3Property());
}
NodeNodeProperty FitPlaneProperties::point3Instance() {
  return NodeNodeProperty(_node, point3Property());
}
void FitPlaneProperties::setPoint3(quint64 value) {
  _node->setNodePropertyTyped<quint64>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point3",
      vx::PropertyValueConvertRaw<quint64, quint64>::toRaw(value));
}
vx::Node* FitPlaneProperties::surface() {
  return vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
      _node->getNodePropertyTyped<QDBusObjectPath>(
          "de.uni_stuttgart.Voxie.Filter.FitPlane.Surface"));
}
QDBusObjectPath FitPlaneProperties::surfaceRaw() {
  return _node->getNodePropertyTyped<QDBusObjectPath>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Surface");
}
QSharedPointer<NodeProperty> FitPlaneProperties::surfaceProperty() {
  return FitPlaneProperties::getNodePrototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Surface", false);
}
NodePropertyTyped<vx::types::NodeReference>
FitPlaneProperties::surfacePropertyTyped() {
  return NodePropertyTyped<vx::types::NodeReference>(surfaceProperty());
}
NodeNodeProperty FitPlaneProperties::surfaceInstance() {
  return NodeNodeProperty(_node, surfaceProperty());
}
void FitPlaneProperties::setSurface(vx::Node* value) {
  _node->setNodePropertyTyped<QDBusObjectPath>(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Surface",
      vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::toRaw(value));
}
FitPlaneProperties::FitPlaneProperties(vx::Node* parent) : QObject(parent) {
  this->_node = parent;
  auto _prop_GeometricPrimitive = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.GeometricPrimitive", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_GeometricPrimitive](
          const QSharedPointer<NodeProperty>& property, const QVariant& value) {
        if (property != _prop_GeometricPrimitive) return;
        vx::Node* valueCasted;
        try {
          valueCasted =
              vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
                  Node::parseVariant<QDBusObjectPath>(value));
        } catch (vx::Exception& e) {
          qCritical() << "Error while parsing property value for event handler "
                         "for property "
                         "\"de.uni_stuttgart.Voxie.Filter.FitPlane."
                         "GeometricPrimitive\":"
                      << e.what();
          return;
        }
        Q_EMIT this->geometricPrimitiveChanged(valueCasted);
      });
  auto _prop_MaximumDistance = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_MaximumDistance](
          const QSharedPointer<NodeProperty>& property, const QVariant& value) {
        if (property != _prop_MaximumDistance) return;
        double valueCasted;
        try {
          valueCasted = vx::PropertyValueConvertRaw<double, double>::fromRaw(
              Node::parseVariant<double>(value));
        } catch (vx::Exception& e) {
          qCritical()
              << "Error while parsing property value for event handler for "
                 "property "
                 "\"de.uni_stuttgart.Voxie.Filter.FitPlane.MaximumDistance\":"
              << e.what();
          return;
        }
        Q_EMIT this->maximumDistanceChanged(valueCasted);
      });
  auto _prop_Point1 = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point1", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_Point1](const QSharedPointer<NodeProperty>& property,
                           const QVariant& value) {
        if (property != _prop_Point1) return;
        quint64 valueCasted;
        try {
          valueCasted = vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
              Node::parseVariant<quint64>(value));
        } catch (vx::Exception& e) {
          qCritical()
              << "Error while parsing property value for event handler for "
                 "property \"de.uni_stuttgart.Voxie.Filter.FitPlane.Point1\":"
              << e.what();
          return;
        }
        Q_EMIT this->point1Changed(valueCasted);
      });
  auto _prop_Point2 = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point2", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_Point2](const QSharedPointer<NodeProperty>& property,
                           const QVariant& value) {
        if (property != _prop_Point2) return;
        quint64 valueCasted;
        try {
          valueCasted = vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
              Node::parseVariant<quint64>(value));
        } catch (vx::Exception& e) {
          qCritical()
              << "Error while parsing property value for event handler for "
                 "property \"de.uni_stuttgart.Voxie.Filter.FitPlane.Point2\":"
              << e.what();
          return;
        }
        Q_EMIT this->point2Changed(valueCasted);
      });
  auto _prop_Point3 = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Point3", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_Point3](const QSharedPointer<NodeProperty>& property,
                           const QVariant& value) {
        if (property != _prop_Point3) return;
        quint64 valueCasted;
        try {
          valueCasted = vx::PropertyValueConvertRaw<quint64, quint64>::fromRaw(
              Node::parseVariant<quint64>(value));
        } catch (vx::Exception& e) {
          qCritical()
              << "Error while parsing property value for event handler for "
                 "property \"de.uni_stuttgart.Voxie.Filter.FitPlane.Point3\":"
              << e.what();
          return;
        }
        Q_EMIT this->point3Changed(valueCasted);
      });
  auto _prop_Surface = this->_node->prototype()->getProperty(
      "de.uni_stuttgart.Voxie.Filter.FitPlane.Surface", false);
  QObject::connect(
      this->_node, &vx::Node::propertyChanged, this,
      [this, _prop_Surface](const QSharedPointer<NodeProperty>& property,
                            const QVariant& value) {
        if (property != _prop_Surface) return;
        vx::Node* valueCasted;
        try {
          valueCasted =
              vx::PropertyValueConvertRaw<QDBusObjectPath, vx::Node*>::fromRaw(
                  Node::parseVariant<QDBusObjectPath>(value));
        } catch (vx::Exception& e) {
          qCritical()
              << "Error while parsing property value for event handler for "
                 "property \"de.uni_stuttgart.Voxie.Filter.FitPlane.Surface\":"
              << e.what();
          return;
        }
        Q_EMIT this->surfaceChanged(valueCasted);
      });
}

}  // namespace filter_prop
}  // namespace vx
