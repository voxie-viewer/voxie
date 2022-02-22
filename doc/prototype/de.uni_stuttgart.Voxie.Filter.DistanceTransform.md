## Description

Performs a distance transform on the dataset, computing the radius of the largest sphere **centered** on each voxel that does not intersect a zero-valued voxel.

This function was implemented with the exact euclidean distance transform from **scipy.ndimage** . 

```outputBuffer.array[:] = ndimage.distance_transform_edt(inputArray[:])```

For more informations see [Distance Transform with ndimage](https://docs.scipy.org/doc/scipy-0.14.0/reference/generated/scipy.ndimage.morphology.distance_transform_edt.html).


### Negate Output
Performs a distance transform on the dataset, computing the radius of the largest sphere **containing** each voxel that does not intersect a zero-valued voxel.

```Threshold method not yet implemented```







