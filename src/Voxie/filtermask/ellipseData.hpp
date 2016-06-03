#pragma once

#include <Voxie/lib/CL/cl.hpp>

/**
 * @brief The ellipseData struct contains the values of the inverted TransformationMatrix.
 */
struct ellipseData
{
    cl_float scaleX = 1;
    cl_float scaleY = 1;
    cl_float dx = 0;
    cl_float dy = 0;
    cl_float angleX = 0;
    cl_float angleY = 0;
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
