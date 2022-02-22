## Description

Utilized the Fourier Transform to align 2 volume data sets.

## 1. Step: Rotation Axis

Calculate fft of both volumes. Take the difference of the magnitude spectra and search for the rotation axis ("zero line")

## 2. Step: Rotation Angle

Sample a circle perpendicular to rotation axis in both magnitude spectras. Find rotation angle using cross correlation.

## 3. Step: Translation

Undo rotation and calculate translation using cross correlation.
