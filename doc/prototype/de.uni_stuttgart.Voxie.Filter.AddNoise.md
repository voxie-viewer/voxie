## Constant Sigma

The same sigma is used for each data point.

## Exponential Scaling Sigma

Noise needs to be measured at high (Max Mean) and low (Min Mean) voxel values.
These values are used to fit an exponential function a * exp(b * x).
The sigma is calculated seperatly for each data point by plugging in the voxel value in the exponential function.
The sigma value of the exponential is capped between "Min Sigma" and "Max Sigma" and the resulting value can be scaled with "Sigma scaling"
