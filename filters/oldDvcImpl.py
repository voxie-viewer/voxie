#!/usr/bin/python3
#
# Copyright (c) 2014-2022 The Voxie Authors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

"""dvc.py: An implementation of Digital Volume Correlation."""

__author__ = "Robin Kindler,Pouya Shahidi"
__copyright__ = "Copyright 2020, Germany"


import os
import cv2
import math
import pprint
import numpy as np
from PIL import Image
from scipy.ndimage import sobel
from multiprocessing import Pool
from scipy.optimize import curve_fit
from scipy.signal import correlate as correlateSignal
from scipy.ndimage.filters import uniform_filter


class Subvolume(object):
    # A subvolume class storing the absorption value data as 3D array
    # besides the split index referring to the position within the
    # original unsplitted volume.

    def __init__(self, absorptionValues):
        self.aValueArray = absorptionValues

    def setAbsorptionArray(self, v):
        self.aValueArray = v

    def getAbsorptionArray(self):
        return self.aValueArray

    def setSplitIndex(self, splitIndex):
        self.siVector = splitIndex

    def getSplitIndex(self):
        return self.siVector


def splitVolume(volume, windowShape, strideShape):
    """
    DVC Preprocessing step.
    Divides a voxel volume into subvolumes.

    :param volume: voxel volume data
    :param windowShape: tupel containing the shape of correlation window (x,y,z)
    :param strideShape: tupel containing the stride sizes (x,y,z)
    :return: Numpy Array of subvolumes of v. Subvolumes have the size as product of windowSize components
    """
    x, y, z = windowShape
    strideX, strideY, strideZ = strideShape

    vShape = volume.shape  # v is a numpy.ndarray
    amountOfSubvolumesX = math.ceil((vShape[0] - x) / strideX)
    amountOfSubvolumesY = math.ceil((vShape[1] - y) / strideY)
    amountOfSubvolumesZ = math.ceil((vShape[2] - z) / strideZ)

    overlapIndexX = vShape[0] / amountOfSubvolumesX
    overlapIndexY = vShape[1] / amountOfSubvolumesY
    overlapIndexZ = vShape[2] / amountOfSubvolumesZ

    # First step: Create a prototype of a 3D list to fill it later with
    # subvolumes
    subvolumeList = np.empty(
        (amountOfSubvolumesX, amountOfSubvolumesY, amountOfSubvolumesZ), dtype=object)

    # Second step: calculate the subvolumes and place them in the subvolumeList
    for i in range(0, amountOfSubvolumesX):
        for j in range(0, amountOfSubvolumesY):
            for k in range(0, amountOfSubvolumesZ):
                # the starting index in direction X where the actual subvolume
                # begins
                splitIndexX = math.floor(i * overlapIndexX)
                splitIndexY = math.floor(j * overlapIndexY)
                splitIndexZ = math.floor(k * overlapIndexZ)
                subvolume = volume[splitIndexX:splitIndexX + x, splitIndexY:splitIndexY +
                                   y, splitIndexZ:splitIndexZ + z]    # calculate the subvolume

                # save subvolume as object
                svObject = Subvolume(subvolume)
                splitIndex = (splitIndexX, splitIndexY, splitIndexZ)
                # save the split index position to be able to merge the
                # subvolumes later
                svObject.setSplitIndex(splitIndex)

                # save the calculated subvolume in subvolume list at position
                # i,j,k
                subvolumeList[i, j, k] = svObject

    return subvolumeList  # return the subvolume list


# (old) unoptimized normalized cross correlation (first try)
def normxcorr(template, image):
    meanTemplate = np.mean(template)
    stdTemplate = np.std(template)
    template = template - meanTemplate
    templateShape = template.shape
    result = np.zeros(np.subtract(image.shape, templateShape) + 1)
    for i in range(np.shape(result)[0]):
        for j in range(np.shape(result)[1]):
            for k in range(np.shape(result)[2]):
                imageUnderTemplate = image[i:i + templateShape[0],
                                           j:j + templateShape[1], k:k + templateShape[2]]
                meanImageUnderTemplate = np.mean(imageUnderTemplate)
                stdProdImageTemplate = np.std(imageUnderTemplate) * stdTemplate
                imageUnderTemplate = imageUnderTemplate - meanImageUnderTemplate
                result[i, j, k] = np.sum(np.multiply(
                    imageUnderTemplate, template)) / stdProdImageTemplate
    return result

# Efficiently calculate mean and standard deviation using sum of squares
# idea: https://stackoverflow.com/a/18422519


def windowMeanStd(image, templateShape):
    origin = tuple(int(-x / 2) for x in templateShape)
    mean = uniform_filter(image, templateShape, mode='constant', origin=origin)
    meansqrd = uniform_filter(
        cv2.pow(image, 2), templateShape, mode='constant', origin=origin)
    return mean[:-templateShape[0] + 1, :-templateShape[1] + 1, :-templateShape[2] + 1],\
        (cv2.sqrt(cv2.subtract(meansqrd, cv2.pow(mean, 2))))[
        :-templateShape[0] + 1, :-templateShape[1] + 1, :-templateShape[2] + 1]


def fastNormXCorr(template, image, stdvTheshold=0):
    # skip blank areas
    templateStdev = np.std(template)
    if templateStdev <= stdvTheshold:
        outputshape = np.subtract(image.shape, template.shape) + 1
        return np.full(np.prod(outputshape), np.nan).reshape(outputshape)

    # normalize template
    templateMean = np.mean(template)
    template = template - templateMean
    template = template / templateStdev
    templateShape = template.shape
    sumOfTemplate = np.sum(template)

    # calculate mean and standard deviation
    mean, stdev = windowMeanStd(image, templateShape)

    # use fft to correlate without normalizing
    corr = correlateSignal(image, template, mode="valid", method="fft")

    # normalize the result using mean and standard deviation
    corr = cv2.divide(corr, stdev)
    corr = cv2.scaleAdd(mean, -sumOfTemplate, corr)

    return corr


def correlate(subvolumeList, volume, roiShape, op,
              stdvTheshold=0, fitWindowSize=0):
    """
    Correlares a splitted voxel volume with an unsplitted voxel volume via Fourier and returns an array of displacement vectors.
    Internally: Calculates a related Region of Interest (ROI) for each element of the subvolume List and normalizes the ROI and the element. Second step is the correlation.

    :param subvolumeList: Reference voxel volume as preprocessed list of subvolumes via the method splitVolume.
    :param volume: Modified voxel volume as volume data array.
    :param roiShape: Shape (Size) of Region of interest. The lower the higher the performance. Big ROIs can cause false correlations (especially on symmetrical or otherwise similar objects).
    :return: 3D Numpy Array of displacement vector objects (objects containing displacement vector and position of the split index of the related subvolume)
    """
    s = subvolumeList.shape
    # size of voxel volume data
    amountOfSubvolumesX = s[0]
    amountOfSubvolumesY = s[1]
    amountOfSubvolumesZ = s[2]

    # Create a 3D dummy list to fill it later with the displacement vectors.
    # 4th axis will contain 3 element for displacement vector, correlation
    # max, 3 element for split index
    vectorList = np.empty(
        (amountOfSubvolumesX, amountOfSubvolumesY, amountOfSubvolumesZ, 7))

    s = subvolumeList.shape
    vShape = volume.shape

    # size of voxel volume data
    amountOfSubvolumesX = s[0]
    amountOfSubvolumesY = s[1]
    amountOfSubvolumesZ = s[2]

    # todo better parameter naming
    def fittingFunction(xdata, ox, oy, oz, ow, a, b, c):
        return (xdata[0] - ox)**2 / a + (xdata[1] - oy)**2 / \
            b + (xdata[2] - oz)**2 / c + ow

    xdata = np.concatenate([a.flatten()[np.newaxis, :] for a in np.meshgrid(
        np.arange(fitWindowSize) - 1, np.arange(fitWindowSize) - 1, np.arange(fitWindowSize) - 1, indexing='ij')])

    bounds = ((-(fitWindowSize // 2), -(fitWindowSize // 2), -(fitWindowSize // 2), 0, -np.inf, -np.inf, -np.inf),
              (fitWindowSize // 2, fitWindowSize // 2, fitWindowSize // 2, np.inf, 0, 0, 0))

    progressStep = 0.5 / amountOfSubvolumesX
    currentProgress = 0
    # for each subvolume
    for i in range(amountOfSubvolumesX):
        for j in range(amountOfSubvolumesY):
            for k in range(amountOfSubvolumesZ):
                # get the related subvolume data
                subvolume = subvolumeList[i, j, k].getAbsorptionArray()
                splitIndex = subvolumeList[i, j, k].getSplitIndex()

                # volumeROI calculation (the bigger volume to correlate the subvolume with).
                # (ROI is centered in subvolume and gets cut off in the border areas).

                # calculate lower and upper bounds for x axis (lx, hx)
                lx = splitIndex[0] - (roiShape[0] / 2) + \
                    (subvolume.shape[0] / 2)
                hx = splitIndex[0] + (roiShape[0] / 2) + \
                    (subvolume.shape[0] / 2)
                lx = int(lx)
                hx = int(hx)
                if lx < 0:
                    lx = 0
                if hx > vShape[0]:
                    hx = vShape[0]

                # calculate lower and upper bounds for y axis (ly, hy)
                ly = splitIndex[1] - (roiShape[1] / 2) + \
                    (subvolume.shape[1] / 2)
                hy = splitIndex[1] + (roiShape[1] / 2) + \
                    (subvolume.shape[1] / 2)
                ly = int(ly)
                hy = int(hy)
                if ly < 0:
                    ly = 0
                if hy > vShape[1]:
                    hy = vShape[1]

                # calculate lower and upper bounds for z axis (lz, hz)
                lz = splitIndex[2] - (roiShape[2] / 2) + \
                    (subvolume.shape[2] / 2)
                hz = splitIndex[2] + (roiShape[2] / 2) + \
                    (subvolume.shape[2] / 2)
                lz = int(lz)
                hz = int(hz)
                if lz < 0:
                    lz = 0
                if hz > vShape[2]:
                    hz = vShape[2]

                # build ROI out of volume
                volumeROI = volume[lx:hx, ly:hy, lz:hz]

                # raise exception if ROI size is too small
                if volumeROI.shape[0] <= subvolume.shape[0] or volumeROI.shape[
                        1] <= subvolume.shape[1] or volumeROI.shape[2] <= subvolume.shape[2]:
                    print("ROI shape: ", volumeROI.shape,
                          ", Subvolume shape: ", subvolume.shape)
                    raise ValueError(
                        "DVC error: ROI size too small. Raise the ROI size.")

                # 3D Correlation of actual Subvolume of v1 and actual ROI of v2
                corr = fastNormXCorr(subvolume, volumeROI,
                                     stdvTheshold=stdvTheshold)

                # find the index of the max. correlation value. This is the
                # position within the subvolume
                result = np.unravel_index(np.argmax(corr), corr.shape)
                corrMax = corr[result]

                # fit a 3d curve to get subpixel maximum
                # todo handle edge cases
                # todo optimize?
                if not (fitWindowSize < 3 or result[0] >= corr.shape[0] - fitWindowSize // 2 or result[1] >= corr.shape[1] - fitWindowSize // 2 or
                        result[2] >= corr.shape[2] - fitWindowSize // 2 or result[0] < fitWindowSize // 2 or result[1] < fitWindowSize // 2 or result[2] < fitWindowSize // 2):
                    try:
                        ydata = corr[result[0] - fitWindowSize // 2:result[0] + fitWindowSize // 2 + 1, result[1] - fitWindowSize //
                                     2:result[1] + fitWindowSize // 2 + 1, result[2] - fitWindowSize // 2:result[2] + fitWindowSize // 2 + 1]
                        popt, pcov = curve_fit(fittingFunction, xdata, ydata.flatten(), p0=[
                            0, 0, 0, corrMax, -1, -1, -1], bounds=bounds)
                        result = (np.array(popt[0:3]) + result)
                        corrMax = popt[3]
                    except Exception as e:
                        print("could not curve fit: ", e)
                # Transform the result to the index within v1 by considering the lower and upper bouds (the offset).
                # index position in v2 = result + offset of roi
                matchIndex = (result[0] + lx, result[1] + ly, result[2] + lz)

                # Calculate actual displacement vecor
                displacementX = matchIndex[0] - splitIndex[0]
                displacementY = matchIndex[1] - splitIndex[1]
                displacementZ = matchIndex[2] - splitIndex[2]
                dVec = [displacementX, displacementY, displacementZ, corrMax]
                # build displacement vector object to be able to save all data
                # (splitIndex and dVec) in a numpy.ndarray
                dVec.extend(splitIndex)
                vectorList[i, j, k] = dVec
        op.ThrowIfCancelled()
        currentProgress += progressStep
        op.SetProgress(currentProgress)
    # Return the np.ndarray of displacement vector objects.
    return vectorList


def mapDisplacement(subvolumeDisplacementArray, v1, windowShape):
    """
    Rebuilds one big voxel volume out of voxel (sub)volume array.
    :param subvolumeDisplacementArray: the array with subvolumes' displacement objects
    :param v1: voxel volume to map the displacementArray on
    :param windowShape: tupel containing the shape of correlation window (x,y,z)
    """
    x, y, z = windowShape

    # size of voxel volume data
    amountOfSubvolumesX = subvolumeDisplacementArray.shape[0]
    amountOfSubvolumesY = subvolumeDisplacementArray.shape[1]
    amountOfSubvolumesZ = subvolumeDisplacementArray.shape[2]

    # prototype of displacement array
    displacementArray = np.empty(v1.shape + (4,))
    for i in range(amountOfSubvolumesX):
        for j in range(amountOfSubvolumesY):
            for k in range(amountOfSubvolumesZ):
                s = subvolumeDisplacementArray[i, j, k, 4:]
                value = subvolumeDisplacementArray[i, j, k, 0:4]
                displacementArray[s[0]:s[0] + x, s[1]:s[1] + y, s[2]:s[2] + z] = value
                # print(value.getValues())
    return displacementArray


def dvc(v1, v2, windowShape, roiShape, op, strideShape, fitWindowSize,
        mapDisplayement=True, verbose=False, stdvThreshold=0):
    """
    Performs the Digital Volume Correlation for two voxel volumes.

    :param v1: voxel volume V1
    :param v2: voxel volume V2
    :param windowShape: tupel containing the shape of correlation window (x,y,z)
    :param op: This voxie operation
    :param strideShape: tupel containing the stride sizes (x,y,z)
    :param fitWindowSize: size of the curve fitting window for correlation results
    :param roiShape: Shape of Region of Interest
    """
    # Handle some errors
    if v1.shape != v2.shape:
        raise ValueError(
            "DVC error: Voxel volumes V1 and V2 must have the same shape.")

    # DVC step one: Preprocessing
    #############################

    py_spy_path = os.environ.get('DVC_PYSPY_PATH', None)
    import subprocess
    pid = os.getpid()

    filename = 'profile_dvc_old.svg'

    subprocess.Popen([py_spy_path, 'record',
                      '--output', filename,
                      '--pid', str(pid),
                      '--rate', '100'])

    # Split v1
    subvolumeListV1 = splitVolume(v1, windowShape, strideShape)

    # DVC step two: Fourier based correlation
    ########################################

    # Calculate ROI, normalize values and correlate each subvolume of v1 with ROI of v2.
    # Returns the resulting displacement vectors it in a numpy 3d array.
    try:
        localDisplacementArrayAndSplitIndices = correlate(
            subvolumeListV1, v2, roiShape, op, stdvThreshold, fitWindowSize)
    except Exception as e:
        print("Unable to calculate correlation: ", e)
        return None

    # Displaying results: Print displacementArray
    if verbose:
        for p in range(0, subvolumeListV1.shape[0]):
            for q in range(0, subvolumeListV1.shape[1]):
                for v in range(0, subvolumeListV1.shape[2]):
                    print("Undervolume index within V1: ",
                          localDisplacementArrayAndSplitIndices[p, q, v, 4:])
                    print("Associated displacement vector: ",
                          localDisplacementArrayAndSplitIndices[p, q, v, :3])
                    print("Correlation's Max Value: ",
                          localDisplacementArrayAndSplitIndices[p, q, v, 3])
                    print("---")

    # DVC step three: Mapping of displacementArray
    ###########################################
    # map displacement vectors on volume v1 as result of the algorithm
    if mapDisplayement:
        result = mapDisplacement(
            localDisplacementArrayAndSplitIndices, v1, windowShape)
    else:
        result = localDisplacementArrayAndSplitIndices[:, :, :, :4]

    return result


def strainTensor(correlationResult):

    x = correlationResult[:, :, :, 0]
    y = correlationResult[:, :, :, 1]
    z = correlationResult[:, :, :, 2]

    dudX = sobel(x, axis=0, mode='constant')
    dvdX = sobel(x, axis=1, mode='constant')
    dwdX = sobel(x, axis=2, mode='constant')
    dudY = sobel(y, axis=0, mode='constant')
    dvdY = sobel(y, axis=1, mode='constant')
    dwdY = sobel(y, axis=2, mode='constant')
    dudZ = sobel(z, axis=0, mode='constant')
    dvdZ = sobel(z, axis=1, mode='constant')
    dwdZ = sobel(z, axis=2, mode='constant')

    exx = dudX + (cv2.pow(dudX, 2) + cv2.pow(dvdX, 2) + cv2.pow(dwdX, 2)) / 2
    eyy = dvdY + (cv2.pow(dudY, 2) + cv2.pow(dvdY, 2) + cv2.pow(dwdY, 2)) / 2
    ezz = dwdZ + (cv2.pow(dudZ, 2) + cv2.pow(dvdZ, 2) + cv2.pow(dwdZ, 2)) / 2

    exy = (dudY + dvdX) / 2 + (cv2.multiply(dudX, dudY) +
                               cv2.multiply(dvdX, dvdY) + cv2.multiply(dwdX, dwdY)) / 2
    exz = (dudZ + dwdX) / 2 + (cv2.multiply(dudX, dudZ) +
                               cv2.multiply(dvdX, dvdZ) + cv2.multiply(dwdX, dwdZ)) / 2
    eyz = (dwdY + dvdZ) / 2 + (cv2.multiply(dudZ, dudY) +
                               cv2.multiply(dvdZ, dvdY) + cv2.multiply(dwdZ, dwdY)) / 2
    return exx, eyy, ezz, exy, exz, eyz
