// This file was automatically generated by tools/update-debug-options.py
// All changes to this file will be lost

#include "DebugOptions.hpp"

namespace vx {
namespace debug_option_impl {
vx::DebugOptionBool Log_FocusChanges_option("Log.FocusChanges");
vx::DebugOptionBool Log_HelpPageCache_option("Log.HelpPageCache");
vx::DebugOptionBool Log_QtEvents_option("Log.QtEvents");
}  // namespace debug_option_impl
}  // namespace vx

vx::DebugOptionBool* vx::debug_option::Log_FocusChanges() {
  return &vx::debug_option_impl::Log_FocusChanges_option;
}
vx::DebugOptionBool* vx::debug_option::Log_HelpPageCache() {
  return &vx::debug_option_impl::Log_HelpPageCache_option;
}
vx::DebugOptionBool* vx::debug_option::Log_QtEvents() {
  return &vx::debug_option_impl::Log_QtEvents_option;
}

QList<vx::DebugOption*> vx::getMainDebugOptions() {
  return {
      vx::debug_option::Log_FocusChanges(),
      vx::debug_option::Log_HelpPageCache(),
      vx::debug_option::Log_QtEvents(),
  };
}