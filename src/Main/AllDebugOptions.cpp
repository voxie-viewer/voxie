#include "AllDebugOptions.hpp"

#include <Main/DebugOptions.hpp>
#include <Voxie/DebugOptions.hpp>
#include <VoxieBackend/DebugOptions.hpp>

static QList<vx::DebugOption*> getAllDebugOptions() {
  QList<vx::DebugOption*> result;
  result << vx::getMainDebugOptions();
  result << vx::getVoxieDebugOptions();
  result << vx::getVoxieBackendDebugOptions();
  return result;
}

QSharedPointer<QList<vx::DebugOption*>> vx::allDebugOptions() {
  static QSharedPointer<QList<vx::DebugOption*>> cache =
      createQSharedPointer<QList<vx::DebugOption*>>(
          std::move(getAllDebugOptions()));
  return cache;
}
