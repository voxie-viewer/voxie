## Marching Cubes 33 by D. Vega et al.

This Marching Cubes 33 implementation was created by D. Vega et al. See the paper from which this code was taken [here](http://jcgt.org/published/0008/03/01/paper.pdf), see the code from this paper [here](http://jcgt.org/published/0008/03/01/marching_cubes_33.zip).\
The following files are copied from the original implementation and partly modified (removed some methods and the normal calculation and refactored some identifiers):
- marchingcubes.h
- MC33_LookupTable.h
- marchingcubes.cpp
- grid3d.cpp
- surface.cpp
- helper.cpp (This file only contains implemented methods extracted from marchingcubes.h as them being there led to compile problems. It is not part of the original implementation but contains no own code.)

The original code was published under the [MIT license](https://opensource.org/licenses/MIT), see LICENSE.txt .
