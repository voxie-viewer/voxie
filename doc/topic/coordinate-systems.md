Coordinate systems in Voxie
===========================

In Voxie 5 different coordinate systems are in use: Global, Object, Voxel, Plane and Pixel.

Transformation / Projection -order:
-----------------------------------

All transformations/Projections in the base-classes are in the following two orders:

```Voxel->Object->Global```

```Pixel->Plane->Global```
  
All following **transformations** in the bullet points are in this order.

Global
------
- **description**: Global coordinate system in which voxie operates.
- **origin**: Arbitrary point.
- **dimension**: 3D
- **coordinate-units**: meter [m]

Object
------
- **description**: Compared to the Globale coordinate system it is shifted by ObjectOrigin and rotated by ObjectOrientation.
- **transformations**: Shift, Rotation
- **origin**: Often object center but can be chosen arbitrarily.
- **dimension**: 3D
- **coordinate-units**: meter [m]

Voxel
-----
- **description**: Coordinate system which is used for storing the data, only valid for regular grid. (0, 0, 0) is the lower left corner of the volume, (nx, ny, nz) is the upper right corner. The coordinate system is shifted by VoxelOrigin, rotated by VoxelOrientation and scaled by the pixel size [pix/meter].
- **transformations**: Shift, Rotation, Scale
- **origin**: Lower left corner of the volume-dataset
- **dimension**: 3D
- **coordinate-units**: voxel_indices [uint]:
    -  E.g. a volume with voxel size (60, 55, 10) contains 60\*55\*10 = 33000 voxels.

Plane
-----
- **description**: Compared to the Globale coordinate system it is shifted by PlanePosition and rotated by PlaneOrientation. In this system the plane lies in the xy-plane with z = 0 & the z-coordinate is the distane to the plane.
- **transformations**: Shift, Rotation
- **origin**: Crosshair set in a SliceVisualizer
- **dimension**: 3D
- **coordinate-units**: meter [m]

Pixel
----------
- **description**: Compared to the Plane coordinate system it is shifted by PixelOrigin and scaled by the pixel size. Before a point can be converted to the pixel coordinate system, it has to be projected onto the plane.
- **transformations**: Shift, Scaling
- **origin**: Bottom left corner of the rectangle (Pixel-plane)
- **dimension**: 2D
- **coordinate-units**: pixel [uint]
