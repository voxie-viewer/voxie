#pragma once

#include <Voxie/lib/CL/cl.hpp>

/**
 * @brief The polygonPoint struct is for gpu cause gpu can't use QPointF.
 */
struct polygonPoint
{
    cl_float  x;
    cl_float  y;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
