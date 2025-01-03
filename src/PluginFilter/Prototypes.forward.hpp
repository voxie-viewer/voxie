// This file was automatically generated by tools/update-node-prototypes.py
// All changes to this file will be lost

#pragma once

#include <VoxieClient/StringConstant.hpp>

namespace vx {
inline namespace filter_prop {
static inline auto ColorizeLabeledSurfaceProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.Filter.ColorizeLabeledSurface");
}
using ColorizeLabeledSurfaceProperties_name =
    decltype(ColorizeLabeledSurfaceProperties_name_function());
class ColorizeLabeledSurfacePropertiesEntry;
class ColorizeLabeledSurfacePropertiesBase;
class ColorizeLabeledSurfacePropertiesCopy;
class ColorizeLabeledSurfaceProperties;
}  // namespace filter_prop
inline namespace filter_prop {
static inline auto ColorizeSurfaceFromAttributeProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.Filter.ColorizeSurfaceFromAttribute");
}
using ColorizeSurfaceFromAttributeProperties_name =
    decltype(ColorizeSurfaceFromAttributeProperties_name_function());
class ColorizeSurfaceFromAttributePropertiesEntry;
class ColorizeSurfaceFromAttributePropertiesBase;
class ColorizeSurfaceFromAttributePropertiesCopy;
class ColorizeSurfaceFromAttributeProperties;
}  // namespace filter_prop
inline namespace filter_prop {
static inline auto CreateSurfaceProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.Filter.CreateSurface");
}
using CreateSurfaceProperties_name =
    decltype(CreateSurfaceProperties_name_function());
class CreateSurfacePropertiesEntry;
class CreateSurfacePropertiesBase;
class CreateSurfacePropertiesCopy;
class CreateSurfaceProperties;
}  // namespace filter_prop
inline namespace filter_prop {
static inline auto TableFilterProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.Filter.TableFilter");
}
using TableFilterProperties_name =
    decltype(TableFilterProperties_name_function());
class TableFilterPropertiesEntry;
class TableFilterPropertiesBase;
class TableFilterPropertiesCopy;
class TableFilterProperties;
}  // namespace filter_prop
template <typename S>
struct PropertiesTypeAlias;
template <>
struct PropertiesTypeAlias<
    ::vx::filter_prop::ColorizeLabeledSurfaceProperties_name> {
  using PropertiesType = ::vx::filter_prop::ColorizeLabeledSurfaceProperties;
  using PropertiesEntryType =
      ::vx::filter_prop::ColorizeLabeledSurfacePropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::filter_prop::ColorizeSurfaceFromAttributeProperties_name> {
  using PropertiesType =
      ::vx::filter_prop::ColorizeSurfaceFromAttributeProperties;
  using PropertiesEntryType =
      ::vx::filter_prop::ColorizeSurfaceFromAttributePropertiesEntry;
};
template <>
struct PropertiesTypeAlias<::vx::filter_prop::CreateSurfaceProperties_name> {
  using PropertiesType = ::vx::filter_prop::CreateSurfaceProperties;
  using PropertiesEntryType = ::vx::filter_prop::CreateSurfacePropertiesEntry;
};
template <>
struct PropertiesTypeAlias<::vx::filter_prop::TableFilterProperties_name> {
  using PropertiesType = ::vx::filter_prop::TableFilterProperties;
  using PropertiesEntryType = ::vx::filter_prop::TableFilterPropertiesEntry;
};
}  // namespace vx
