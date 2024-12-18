// This file was automatically generated by tools/update-node-prototypes.py
// All changes to this file will be lost

#pragma once

#include <VoxieClient/StringConstant.hpp>

namespace vx {
inline namespace filter_prop {
static inline auto SegmentationProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.Filter.Segmentation");
}
using SegmentationProperties_name =
    decltype(SegmentationProperties_name_function());
class SegmentationPropertiesEntry;
class SegmentationPropertiesBase;
class SegmentationPropertiesCopy;
class SegmentationProperties;
}  // namespace filter_prop
inline namespace segmentationstep_prop {
static inline auto AssignmentStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.AssignmentStep");
}
using AssignmentStepProperties_name =
    decltype(AssignmentStepProperties_name_function());
class AssignmentStepPropertiesEntry;
class AssignmentStepPropertiesBase;
class AssignmentStepPropertiesCopy;
class AssignmentStepProperties;
}  // namespace segmentationstep_prop
inline namespace segmentationstep_prop {
static inline auto BrushSelectionStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep");
}
using BrushSelectionStepProperties_name =
    decltype(BrushSelectionStepProperties_name_function());
class BrushSelectionStepPropertiesEntry;
class BrushSelectionStepPropertiesBase;
class BrushSelectionStepPropertiesCopy;
class BrushSelectionStepProperties;
}  // namespace segmentationstep_prop
inline namespace segmentationstep_prop {
static inline auto LassoSelectionStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.LassoSelectionStep");
}
using LassoSelectionStepProperties_name =
    decltype(LassoSelectionStepProperties_name_function());
class LassoSelectionStepPropertiesEntry;
class LassoSelectionStepPropertiesBase;
class LassoSelectionStepPropertiesCopy;
class LassoSelectionStepProperties;
}  // namespace segmentationstep_prop
inline namespace segmentationstep_prop {
static inline auto ManualSelectionStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.ManualSelectionStep");
}
using ManualSelectionStepProperties_name =
    decltype(ManualSelectionStepProperties_name_function());
class ManualSelectionStepPropertiesEntry;
class ManualSelectionStepPropertiesBase;
class ManualSelectionStepPropertiesCopy;
class ManualSelectionStepProperties;
}  // namespace segmentationstep_prop
inline namespace segmentationstep_prop {
static inline auto MetaStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.MetaStep");
}
using MetaStepProperties_name = decltype(MetaStepProperties_name_function());
class MetaStepPropertiesEntry;
class MetaStepPropertiesBase;
class MetaStepPropertiesCopy;
class MetaStepProperties;
}  // namespace segmentationstep_prop
inline namespace segmentationstep_prop {
static inline auto MultiThresholdStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.MultiThresholdStep");
}
using MultiThresholdStepProperties_name =
    decltype(MultiThresholdStepProperties_name_function());
class MultiThresholdStepPropertiesEntry;
class MultiThresholdStepPropertiesBase;
class MultiThresholdStepPropertiesCopy;
class MultiThresholdStepProperties;
}  // namespace segmentationstep_prop
inline namespace segmentationstep_prop {
static inline auto RemoveLabelStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.RemoveLabelStep");
}
using RemoveLabelStepProperties_name =
    decltype(RemoveLabelStepProperties_name_function());
class RemoveLabelStepPropertiesEntry;
class RemoveLabelStepPropertiesBase;
class RemoveLabelStepPropertiesCopy;
class RemoveLabelStepProperties;
}  // namespace segmentationstep_prop
inline namespace segmentationstep_prop {
static inline auto SubtractStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.SubtractStep");
}
using SubtractStepProperties_name =
    decltype(SubtractStepProperties_name_function());
class SubtractStepPropertiesEntry;
class SubtractStepPropertiesBase;
class SubtractStepPropertiesCopy;
class SubtractStepProperties;
}  // namespace segmentationstep_prop
inline namespace segmentationstep_prop {
static inline auto ThresholdSelectionStepProperties_name_function() {
  return VX_GET_STRING_CONSTANT_VALUE(
      "de.uni_stuttgart.Voxie.SegmentationStep.ThresholdSelectionStep");
}
using ThresholdSelectionStepProperties_name =
    decltype(ThresholdSelectionStepProperties_name_function());
class ThresholdSelectionStepPropertiesEntry;
class ThresholdSelectionStepPropertiesBase;
class ThresholdSelectionStepPropertiesCopy;
class ThresholdSelectionStepProperties;
}  // namespace segmentationstep_prop
template <typename S>
struct PropertiesTypeAlias;
template <>
struct PropertiesTypeAlias<::vx::filter_prop::SegmentationProperties_name> {
  using PropertiesType = ::vx::filter_prop::SegmentationProperties;
  using PropertiesEntryType = ::vx::filter_prop::SegmentationPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::AssignmentStepProperties_name> {
  using PropertiesType = ::vx::segmentationstep_prop::AssignmentStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::AssignmentStepPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::BrushSelectionStepProperties_name> {
  using PropertiesType =
      ::vx::segmentationstep_prop::BrushSelectionStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::BrushSelectionStepPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::LassoSelectionStepProperties_name> {
  using PropertiesType =
      ::vx::segmentationstep_prop::LassoSelectionStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::LassoSelectionStepPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::ManualSelectionStepProperties_name> {
  using PropertiesType =
      ::vx::segmentationstep_prop::ManualSelectionStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::ManualSelectionStepPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::MetaStepProperties_name> {
  using PropertiesType = ::vx::segmentationstep_prop::MetaStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::MetaStepPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::MultiThresholdStepProperties_name> {
  using PropertiesType =
      ::vx::segmentationstep_prop::MultiThresholdStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::MultiThresholdStepPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::RemoveLabelStepProperties_name> {
  using PropertiesType = ::vx::segmentationstep_prop::RemoveLabelStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::RemoveLabelStepPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::SubtractStepProperties_name> {
  using PropertiesType = ::vx::segmentationstep_prop::SubtractStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::SubtractStepPropertiesEntry;
};
template <>
struct PropertiesTypeAlias<
    ::vx::segmentationstep_prop::ThresholdSelectionStepProperties_name> {
  using PropertiesType =
      ::vx::segmentationstep_prop::ThresholdSelectionStepProperties;
  using PropertiesEntryType =
      ::vx::segmentationstep_prop::ThresholdSelectionStepPropertiesEntry;
};
}  // namespace vx
