#pragma once

#include <Voxie/filtermask/polygonPoint.hpp>

#include <QtCore/QPointF>
#include <QtCore/QVector>

/**
 * @brief The polygonData struct contains cpuCoords and gpuCoords.
 */

struct polygonData
{
	QVector<QPointF> cpuCoords;
	QVector<polygonPoint> gpuCoords;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
