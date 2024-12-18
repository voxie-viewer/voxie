// This file was automatically generated by tools/update-debug-options.py
// All changes to this file will be lost

#pragma once

#include <VoxieClient/DebugOption.hpp>

#include <Voxie/Voxie.hpp>

namespace vx {
namespace debug_option {
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_HighDPI();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_NodeNameLineEdit();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_Properties_UI();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_Segmentation_IterateVoxels();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_TrackNodeLifecycle();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_View3DUpdates();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_Vis_Keyboard();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_Vis_Mouse();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_Vis3D_CreateModifiedSurface();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_Vis3D_MouseTracking();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_VisSlice_BrushSelection();
VOXIECORESHARED_EXPORT vx::DebugOptionBool*
Log_Workaround_CoalesceOpenGLResize();
VOXIECORESHARED_EXPORT vx::DebugOptionFloat*
VisSlice_BrushSelection_MinDistance();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Workaround_CoalesceOpenGLResize();
}  // namespace debug_option

VOXIECORESHARED_EXPORT QList<vx::DebugOption*> getVoxieDebugOptions();
}  // namespace vx
