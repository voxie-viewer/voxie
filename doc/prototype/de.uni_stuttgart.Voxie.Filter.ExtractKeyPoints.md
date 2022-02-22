## Description

This filter implements the keypoint descriptor "Intrinsic Shape Signatures".
It computes the scatter matrix for each point and uses the eigenvalues to detect keypoints.

### SearchRadius

Support radius to calculate the scatter matrix.
If SearchRadius is set to zero, it will be calculated automatically.

### MinNeighbors

The required minimum number of points within the SearchRadius.

### Gamma21 and Gamma32

The eigenvalues of the scatter matrix in decreasing order are denoted as e1, e2, e3.
Points whose ratio between two successive eigenvalues is below a threshold are retained.
e2/e1 < Gamma21 and e3/e2 < Gamma32. Small parameter values will result in less keypoints.

### MaxRadius
A point is only considered a keypoint if it has the highest e3 value within the neighborhood detemined by MaxRadius.
If MaxRadius is set to zero, it will be calculated automatically.
