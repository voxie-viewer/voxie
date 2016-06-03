#pragma once

#include <QtCore/qglobal.h>

#if defined(VOXIECORE_LIBRARY)
#  define VOXIECORESHARED_EXPORT Q_DECL_EXPORT
#else
#  define VOXIECORESHARED_EXPORT Q_DECL_IMPORT
#endif

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
