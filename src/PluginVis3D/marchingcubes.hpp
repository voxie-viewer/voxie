#pragma once

#include <QtGui/QVector3D>

struct TRIANGLE
{
   QVector3D p[3];
};

struct GRIDCELL
{
   QVector3D p[8];
   double val[8];
};

/*
   Linearly interpolate the position where an isosurface cuts
   an edge between two vertices, each with their own scalar value
*/
QVector3D VertexInterp(double isolevel, const QVector3D &p1, const QVector3D &p2, double valp1, double valp2);


/*
   Given a grid cell and an isolevel, calculate the triangular
   facets required to represent the isosurface through the cell.
   Return the number of triangular facets, the array "triangles"
   will be loaded up with the vertices at most 5 triangular facets.
    0 will be returned if the grid cell is either totally above
   of totally below the isolevel.
*/
int Polygonise(const GRIDCELL &grid, double isolevel, TRIANGLE *triangles, bool invert);

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
