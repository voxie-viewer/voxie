// This file was automatically generated by tools/update-debug-options.py
// All changes to this file will be lost

#pragma once

#include <VoxieClient/DebugOption.hpp>

#include <Voxie/Voxie.hpp>

namespace vx {
namespace debug_option {
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_View3DUpdates();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_Vis3D_CreateModifiedSurface();
VOXIECORESHARED_EXPORT vx::DebugOptionBool* Log_Vis3D_MouseTracking();
}  // namespace debug_option

VOXIECORESHARED_EXPORT QList<vx::DebugOption*> getVoxieDebugOptions();
}  // namespace vx
