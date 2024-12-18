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

import voxie
from voxie.table import *

import numpy
import dbus
import os
import concurrent.futures
import math
from scipy import ndimage, nan, inf
from skimage import measure as skmeasure

# marching_cubes_lewiner was removed in 0.19
# https://scikit-image.org/docs/stable/release_notes/release_0.19.html
try:
    from skimage.measure import marching_cubes
except ImportError:
    from skimage.measure import marching_cubes_lewiner as marching_cubes

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')

    labelPath = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.LabelVolume'].getValue(
        'o')
    if labelPath == dbus.ObjectPath('/'):
        raise Exception('No label volume specified')
    labelProperties = pars[labelPath]['Properties'].getValue('a{sv}')
    labelDataPath = pars[labelPath]['Data'].getValue('o')
    labelDataObject = context.makeObject(context.bus, context.busName, labelDataPath, [
                                         'de.uni_stuttgart.Voxie.Data']).CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')
    labelData = labelDataObject[:]
    spacing = labelDataObject.GridSpacing
    origin = labelDataObject.VolumeOrigin
    cubicMetersPerVoxel = spacing[0] * spacing[1] * spacing[2]

    def voxelLengthToMeters(coords):
        return [coords[i] * spacing[i] for i in range(3)]

    def voxelCoordsToMeters(coords):
        return [coords[i] * spacing[i] - origin[i] for i in range(3)]

    volumePath = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.Volume'].getValue(
        'o')
    if volumePath == dbus.ObjectPath('/'):
        print('Information: Without the original VolumeObject not all values can be calculated!')
        volumeData = None
    else:
        volumeProperties = pars[volumePath]['Properties'].getValue('a{sv}')
        volumeDataPath = pars[volumePath]['Data'].getValue('o')
        volumeData = context.makeObject(context.bus, context.busName, volumeDataPath, [
                                        'de.uni_stuttgart.Voxie.Data']).CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')[:]

    # Ensure that volume data has the same dimensions as the label data
    if volumeData is not None and volumeData.shape != labelData.shape:
        print('Warning: Volume object dimensions do not match up with Label object dimensions. Values that depend on the voxel data values will not be calculated.')
        volumeData = None

    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    # Define columns for table
    columns = [
        TableColumn.int('LabelID', 'Label ID'),
        TableColumn.int('NumberOfVoxels', 'Voxel count'),
        TableColumn.float('Volume', 'Volume', 'm^3'),
        TableColumn.bbox3('BoundingBox', 'Bounding box', 'm'),
        TableColumn.vec3('CenterOfMass', 'Center of mass', 'm'),
    ]

    # Obtain thresholds
    thresholdAll = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.Threshold'].getValue(
        'x')

    enableSurfaceArea = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.EnableSurfaceArea'].getValue(
        'b')
    thresholdSurfaceArea = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.ThresholdSurfaceArea'].getValue('x') \
        if enableSurfaceArea else 0

    enableInscribedSphereRadius = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.EnableInscribedSphereRadius'].getValue(
        'b')
    thresholdInscribedSphereRadius = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.ThresholdInscribedSphereRadius'].getValue('x') \
        if enableInscribedSphereRadius else 0

    # Obtain inclusion flags
    includeBorders = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.IncludeBorders'].getValue(
        'b')
    includeBackground = properties['de.uni_stuttgart.Voxie.Filter.ConnectedComponentAnalysis.IncludeBackground'].getValue(
        'b')

    # Show additional columns when volume dataset is available
    if volumeData is not None:
        columns += [
            TableColumn.vec3('WeightedCenterOfMass',
                             'Weighted center of mass', 'm'),
            TableColumn.float('SumOfValues', 'Sum of values'),
            TableColumn.float('Average', 'Average'),
            TableColumn.float('Minimum', 'Minimum'),
            TableColumn.float('Maximum', 'Maximum'),
            TableColumn.float('Median', 'Median'),
            TableColumn.float('Variance', 'Variance'),
            TableColumn.float('StandardDeviation', 'Standard deviation'),
        ]

    if enableSurfaceArea:
        columns += [
            TableColumn.float('SurfaceArea', 'Surface area', 'm^2'),
            TableColumn.float('Sphericity', 'Sphericity'),
        ]

    if enableInscribedSphereRadius:
        columns += [
            TableColumn.float('InscribedSphereRadius',
                              'Inscribed sphere radius', 'm'),
        ]

    columns += [
        TableColumn.float('EdgeDistance', 'Edge distance', 'm'),
    ]

    with instance.CreateTableData(createColumnDefinition(instance, columns)) as resultData:
        with resultData.CreateUpdate() as update:

            # Initialize progress variables
            completedSliceCount = 0
            totalSliceCount = labelData.shape[0]

            completedRegionCount = 0
            totalRegionCount = 1

            def updateProgress():
                # Estimate that 10% of the time is spent in find_objects, and
                # 90% in region metric computation
                op.SetProgress(0.1 * (completedSliceCount / totalSliceCount) +
                               0.9 * (completedRegionCount / totalRegionCount))

            # Flatten label data to enable voxel counting
            flatLabelData = labelData.reshape(-1)
            labelVoxelCounts = numpy.bincount(flatLabelData)
            if volumeData is not None:
                labelVoxelSums = numpy.bincount(
                    flatLabelData, weights=volumeData.reshape(-1))

            # Initialize region list
            regions = {}

            def mergeRegions(region1, region2):
                return (slice(min(region1[0].start, region2[0].start), max(region1[0].stop, region2[0].stop)),
                        slice(min(region1[1].start, region2[1].start), max(
                            region1[1].stop, region2[1].stop)),
                        slice(min(region1[2].start, region2[2].start), max(region1[2].stop, region2[2].stop)))

            def findObjects(labels):
                # Shift label indices by 1 if 'include background' is enabled
                # to include 0-labels
                return ndimage.find_objects(
                    labels + 1 if includeBackground else labels)

            # Use multiple threads to find bounding boxes
            with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
                futureMap = {executor.submit(
                    findObjects, labelData[i]): i for i in range(labelData.shape[0])}
                for future in concurrent.futures.as_completed(futureMap):
                    # Obtain region list from slice
                    sliceIndex = futureMap[future]
                    for regionIndex, region in enumerate(future.result()):
                        if region is not None:
                            # Re-add X-coordinate (sliceIndex) to slice tuple
                            region = (slice(sliceIndex, sliceIndex + 1),
                                      region[0], region[1])
                            if regionIndex in regions:
                                regions[regionIndex] = mergeRegions(
                                    regions[regionIndex], region)
                            else:
                                regions[regionIndex] = region

                    # Update filter progress
                    completedSliceCount += 1
                    updateProgress()

            totalRegionCount = max(len(regions), 1)

            def processRegion(labelID, slices):
                # Determine general region statistics based on previous
                # 'bincounts'
                voxelCount = labelVoxelCounts[labelID]
                totalVolume = float(voxelCount) * cubicMetersPerVoxel
                voxelSum = labelVoxelSums[labelID] if volumeData is not None else -1
                voxelAverage = voxelSum / voxelCount if volumeData is not None else -1

                # Compute bounding box from region slices
                boundingBox = numpy.transpose(
                    [(slice.start, slice.stop - 1) for slice in slices])
                boundingBox = [voxelCoordsToMeters(
                    boundingBox[0]), voxelCoordsToMeters(boundingBox[1])]

                # Compute the distance of the region to the nearest edge
                edgeDistance = numpy.amin(voxelLengthToMeters(
                    [min(slices[i].start, labelData.shape[i] - slices[i].stop) for i in range(3)]))

                # Exit early if voxel is above threshold or touching the border
                # if border inclusion is disabled
                if voxelCount > thresholdAll or (
                        not includeBorders and edgeDistance == 0):
                    # Return minimal data
                    return {
                        'LabelID': labelID,
                        'NumberOfVoxels': voxelCount,
                        'Volume': totalVolume,
                        'BoundingBox': boundingBox,
                        'EdgeDistance': edgeDistance,
                    }

                # Obtain sliced sub-cuboid of the label volume containing only
                # the current region of interest
                slicePosition = [slice.start for slice in slices]
                slicedLabelData = labelData[slices]
                maskedLabelData = (slicedLabelData == labelID)

                # Compute unweighted mean of matching voxel positions by passing in an array of 1s as the weight
                # array. Afterwards, add slice position to local position to
                # correct for bounding box slicing
                centerOfMass = voxelCoordsToMeters(numpy.asarray(ndimage.center_of_mass(
                    input=numpy.ones(slicedLabelData.shape),
                    labels=slicedLabelData,
                    index=labelID)) + slicePosition)

                if volumeData is not None:
                    slicedVolumeData = volumeData[slices]

                    # Compute various statistical values for the region
                    minimum = ndimage.minimum(
                        input=slicedVolumeData, labels=slicedLabelData, index=labelID)
                    maximum = ndimage.maximum(
                        input=slicedVolumeData, labels=slicedLabelData, index=labelID)
                    median = ndimage.median(
                        input=slicedVolumeData, labels=slicedLabelData, index=labelID)
                    variance = ndimage.variance(
                        input=slicedVolumeData, labels=slicedLabelData, index=labelID)
                    standardDeviation = math.sqrt(variance)

                    # Compute weighted mean of matching voxel positions
                    weightedCenterOfMass = voxelCoordsToMeters(numpy.asarray(ndimage.center_of_mass(
                        input=slicedVolumeData,
                        labels=slicedLabelData,
                        index=labelID)) + slicePosition)
                else:
                    weightedCenterOfMass = [nan, nan, nan]
                    minimum = nan
                    maximum = nan
                    median = nan
                    variance = nan
                    standardDeviation = nan

                if voxelCount < thresholdSurfaceArea:
                    # Compute the area of the region mask's isosurface
                    # (generated using Marching Cubes)
                    surfaceVertices, surfaceFaces, surfaceNormals, surfaceValues = marching_cubes(
                        numpy.pad(maskedLabelData, mode='constant', pad_width=1, constant_values=0), level=0.001,
                        spacing=spacing)
                    surfaceArea = skmeasure.mesh_surface_area(
                        surfaceVertices, surfaceFaces)

                    # Compute the radius of a sphere with the same volume as the region. The region's
                    # sphericity is the ratio of this sphere's surface area to
                    # the surface area of the region.
                    sphereRadius = ((3. * totalVolume) /
                                    (4. * numpy.pi)) ** (1. / 3.)
                    sphereSurfaceArea = 4. * numpy.pi * sphereRadius * sphereRadius
                    sphericity = sphereSurfaceArea / surfaceArea
                else:
                    surfaceArea = nan
                    sphericity = nan

                if voxelCount < thresholdInscribedSphereRadius:
                    # Compute the radius of the largest sphere fitting into the
                    # region
                    inscribedSphereRadius = numpy.amax(
                        ndimage.distance_transform_edt(maskedLabelData, sampling=spacing))
                else:
                    inscribedSphereRadius = nan

                # Return region data as row dict
                return {
                    'LabelID': labelID,
                    'NumberOfVoxels': voxelCount,
                    'Volume': totalVolume,
                    'BoundingBox': boundingBox,
                    'CenterOfMass': centerOfMass,
                    'WeightedCenterOfMass': weightedCenterOfMass,
                    'SumOfValues': voxelSum,
                    'Average': voxelAverage,
                    'Minimum': minimum,
                    'Maximum': maximum,
                    'Median': median,
                    'Variance': variance,
                    'StandardDeviation': standardDeviation,
                    'SurfaceArea': surfaceArea,
                    'InscribedSphereRadius': inscribedSphereRadius,
                    'Sphericity': sphericity,
                    'EdgeDistance': edgeDistance,
                }

            with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as executor:
                futureMap = {}
                for _, (index, slices) in enumerate(regions.items()):
                    if slices is not None:
                        labelID = index if includeBackground else index + 1
                        futureMap[executor.submit(
                            processRegion, labelID, slices)] = labelID
                for future in concurrent.futures.as_completed(futureMap):
                    # Update filter progress
                    completedRegionCount += 1
                    updateProgress()

                    # Add row to table
                    resultData.AddRow(update, createRow(
                        columns, future.result()))

            version = update.Finish()

        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', resultData._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
