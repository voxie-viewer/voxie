// This file was automatically generated by tools/update-node-prototypes.py
// All changes to this file will be lost

#pragma once

#include <VoxieClient/StringConstant.hpp>

namespace vx {
inline namespace filter_prop {
static inline auto TheSphereGeneratorProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.Example.Filter.TheSphereGenerator");
}
using TheSphereGeneratorProperties_name =
    decltype(TheSphereGeneratorProperties_name_function());
class TheSphereGeneratorPropertiesEntry;
class TheSphereGeneratorPropertiesBase;
class TheSphereGeneratorPropertiesCopy;
class TheSphereGeneratorProperties;
}  // namespace filter_prop
inline namespace visualizer_prop {
static inline auto RandomChartProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.Example.Visualizer.RandomChart");
}
using RandomChartProperties_name =
    decltype(RandomChartProperties_name_function());
class RandomChartPropertiesEntry;
class RandomChartPropertiesBase;
class RandomChartPropertiesCopy;
class RandomChartProperties;
}  // namespace visualizer_prop
template <typename S>
struct PropertiesTypeAlias;
template <>
struct PropertiesTypeAlias<
    ::vx::filter_prop::TheSphereGeneratorProperties_name> {
  using PropertiesType = ::vx::filter_prop::TheSphereGeneratorProperties;
  using PropertiesEntryType =
      ::vx::filter_prop::TheSphereGeneratorPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<::vx::visualizer_prop::RandomChartProperties_name> {
  using PropertiesType = ::vx::visualizer_prop::RandomChartProperties;
  using PropertiesEntryType = ::vx::visualizer_prop::RandomChartPropertiesEntry;
};
}  // namespace vx
