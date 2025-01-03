// This file was automatically generated by tools/update-debug-options.py
// All changes to this file will be lost

#include "DebugOptions.hpp"

namespace vx {
namespace debug_option_impl {
vx::DebugOptionBool Log_HighDPI_option("Log.HighDPI");
vx::DebugOptionBool Log_NodeNameLineEdit_option("Log.NodeNameLineEdit");
vx::DebugOptionBool Log_Properties_UI_option("Log.Properties.UI");
vx::DebugOptionBool Log_Segmentation_IterateVoxels_option(
    "Log.Segmentation.IterateVoxels");
vx::DebugOptionBool Log_TrackNodeLifecycle_option("Log.TrackNodeLifecycle");
vx::DebugOptionBool Log_View3DUpdates_option("Log.View3DUpdates");
vx::DebugOptionBool Log_Vis_Keyboard_option("Log.Vis.Keyboard");
vx::DebugOptionBool Log_Vis_Mouse_option("Log.Vis.Mouse");
vx::DebugOptionBool Log_Vis3D_CreateModifiedSurface_option(
    "Log.Vis3D.CreateModifiedSurface");
vx::DebugOptionBool Log_Vis3D_MouseTracking_option("Log.Vis3D.MouseTracking");
vx::DebugOptionBool Log_VisSlice_BrushSelection_option(
    "Log.VisSlice.BrushSelection");
vx::DebugOptionBool Log_Workaround_CoalesceOpenGLResize_option(
    "Log.Workaround.CoalesceOpenGLResize");
vx::DebugOptionFloat VisSlice_BrushSelection_MinDistance_option(
    "VisSlice.BrushSelection.MinDistance", 1);
vx::DebugOptionBool Workaround_CoalesceOpenGLResize_option(
    "Workaround.CoalesceOpenGLResize", true);
}  // namespace debug_option_impl
}  // namespace vx

vx::DebugOptionBool* vx::debug_option::Log_HighDPI() {
  return &vx::debug_option_impl::Log_HighDPI_option;
}
vx::DebugOptionBool* vx::debug_option::Log_NodeNameLineEdit() {
  return &vx::debug_option_impl::Log_NodeNameLineEdit_option;
}
vx::DebugOptionBool* vx::debug_option::Log_Properties_UI() {
  return &vx::debug_option_impl::Log_Properties_UI_option;
}
vx::DebugOptionBool* vx::debug_option::Log_Segmentation_IterateVoxels() {
  return &vx::debug_option_impl::Log_Segmentation_IterateVoxels_option;
}
vx::DebugOptionBool* vx::debug_option::Log_TrackNodeLifecycle() {
  return &vx::debug_option_impl::Log_TrackNodeLifecycle_option;
}
vx::DebugOptionBool* vx::debug_option::Log_View3DUpdates() {
  return &vx::debug_option_impl::Log_View3DUpdates_option;
}
vx::DebugOptionBool* vx::debug_option::Log_Vis_Keyboard() {
  return &vx::debug_option_impl::Log_Vis_Keyboard_option;
}
vx::DebugOptionBool* vx::debug_option::Log_Vis_Mouse() {
  return &vx::debug_option_impl::Log_Vis_Mouse_option;
}
vx::DebugOptionBool* vx::debug_option::Log_Vis3D_CreateModifiedSurface() {
  return &vx::debug_option_impl::Log_Vis3D_CreateModifiedSurface_option;
}
vx::DebugOptionBool* vx::debug_option::Log_Vis3D_MouseTracking() {
  return &vx::debug_option_impl::Log_Vis3D_MouseTracking_option;
}
vx::DebugOptionBool* vx::debug_option::Log_VisSlice_BrushSelection() {
  return &vx::debug_option_impl::Log_VisSlice_BrushSelection_option;
}
vx::DebugOptionBool* vx::debug_option::Log_Workaround_CoalesceOpenGLResize() {
  return &vx::debug_option_impl::Log_Workaround_CoalesceOpenGLResize_option;
}
vx::DebugOptionFloat* vx::debug_option::VisSlice_BrushSelection_MinDistance() {
  return &vx::debug_option_impl::VisSlice_BrushSelection_MinDistance_option;
}
vx::DebugOptionBool* vx::debug_option::Workaround_CoalesceOpenGLResize() {
  return &vx::debug_option_impl::Workaround_CoalesceOpenGLResize_option;
}

QList<vx::DebugOption*> vx::getVoxieDebugOptions() {
  return {
      vx::debug_option::Log_HighDPI(),
      vx::debug_option::Log_NodeNameLineEdit(),
      vx::debug_option::Log_Properties_UI(),
      vx::debug_option::Log_Segmentation_IterateVoxels(),
      vx::debug_option::Log_TrackNodeLifecycle(),
      vx::debug_option::Log_View3DUpdates(),
      vx::debug_option::Log_Vis_Keyboard(),
      vx::debug_option::Log_Vis_Mouse(),
      vx::debug_option::Log_Vis3D_CreateModifiedSurface(),
      vx::debug_option::Log_Vis3D_MouseTracking(),
      vx::debug_option::Log_VisSlice_BrushSelection(),
      vx::debug_option::Log_Workaround_CoalesceOpenGLResize(),
      vx::debug_option::VisSlice_BrushSelection_MinDistance(),
      vx::debug_option::Workaround_CoalesceOpenGLResize(),
  };
}
