// This file was automatically generated by tools/update-debug-options.py
// All changes to this file will be lost

#pragma once

#include <VoxieClient/DebugOption.hpp>

namespace vx {
namespace debug_option {
vx::DebugOptionBool* CMark_VerifyNodeDeepClone();
vx::DebugOptionBool* Log_FocusChanges();
vx::DebugOptionBool* Log_HelpPageCache();
vx::DebugOptionBool* Log_QtEvents();
}  // namespace debug_option

QList<vx::DebugOption*> getMainDebugOptions();
}  // namespace vx
