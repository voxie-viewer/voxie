#!/usr/bin/python3

import sys
import os
import numpy
import numpy as np
import time
import scipy.misc
import subprocess
import argparse

import voxie

parser = argparse.ArgumentParser()
parser.add_argument('--voxie-bus-address')
parser.add_argument('--voxie-bus-name')
parser.add_argument('--voxie-slice-object')

parser.add_argument('--rotation')
parser.add_argument('--minpos')
parser.add_argument('--maxpos')
parser.add_argument('--maxval')
parser.add_argument('--fps')
parser.add_argument('--slice-factor')
parser.add_argument('--output-file')

args = parser.parse_args()

instance = voxie.Voxie(args)

slice = instance.getSlice(args)

dataSet = slice.getDataSet()

data = dataSet.getFilteredData()

with instance.createClient() as client:
    pixSize = 100e-6
    slicethick = pixSize / 10
    #slicethick = pixSize * 10
    if args.slice_factor is not None:
        slicethick = pixSize / float(args.slice_factor)

    voxArray = data.getDataWritable().array
    valMin = numpy.min (voxArray)
    valMax = numpy.max (voxArray)

    if args.maxval is not None:
        valMax = float(args.maxval)

    if args.rotation is not None:
        orientation = np.fromstring(args.rotation, dtype=float, sep=',')
        if orientation.shape != (4,):
            raise Exception('Invalid --rotation value: ' + str(orientation))
    else:
        plane = slice.dbus_properties.Get('de.uni_stuttgart.Voxie.Slice', 'Plane')
        #origin = numpy.array(plane[0])
        orientation = plane[1]
    rotation = voxie.Rotation (orientation)

    size = numpy.array(data.get('Size'), dtype='uint64')
    dorigin = numpy.array(data.get('Origin'), dtype=numpy.double)
    dspacing = numpy.array(data.get('Spacing'), dtype=numpy.double)
    posmin = posmax = None
    for corner in [[0, 0, 0], [0, 0, 1], [0, 1, 0], [0, 1, 1], [1, 0, 0], [1, 0, 1], [1, 1, 0], [1, 1, 1]]:
        cpos = numpy.array(corner,dtype=numpy.double) * size
        cpos = rotation.inverse * (dorigin + dspacing * cpos)
        if posmin is None:
            posmin = cpos
        if posmax is None:
            posmax = cpos
        posmin = numpy.minimum (posmin, cpos)
        posmax = numpy.maximum (posmax, cpos)

    #print ('posmin[2] = %f, posmax[2] = %f' % (posmin[2], posmax[2]))
    if args.minpos is not None:
        posmin[2] = float(args.minpos)
    if args.maxpos is not None:
        posmax[2] = float(args.maxpos)
    #print ('posmin[2] = %f, posmax[2] = %f' % (posmin[2], posmax[2]))

    width = int ((posmax[0] - posmin[0]) / pixSize + 1)
    height = int ((posmax[1] - posmin[1]) / pixSize + 1)
    with instance.createImage(client, (width, height)) as image, image.getDataReadonly() as buffer:
        oarray = buffer.array
    
        options = {}
        options['Interpolation'] = 'Linear'

        fps = 10
        if args.fps is not None:
            fps = float(args.fps)
        outputFile = args.output_file
        process = subprocess.Popen(['ffmpeg', '-v', 'quiet', '-f', 'rawvideo', '-vcodec', 'rawvideo', '-s', '%dx%d' % (width, height), '-r', '%f' % fps, '-pix_fmt', 'rgb24', '-i', '-', '-codec', 'mjpeg', '-qscale', '1', '-y', outputFile], stdin = subprocess.PIPE)

        for i in range (int ((posmax[2] - posmin[2]) / slicethick + 1)):
            pos0 = (posmin[0], posmin[1], posmin[2] + slicethick * i)
    
            pos = rotation * pos0
            data.dbus.ExtractSlice (pos, orientation, (width, height), (pixSize, pixSize), image.path, options)
        
            image.dbus.UpdateBuffer({})
        
            array = oarray
        
            array = numpy.array(array)
            array[numpy.isnan(array)] = 0
    
            array = array[:,::-1]
            array = array.transpose()

            array = array[:,:,numpy.newaxis] * numpy.ones(3)[numpy.newaxis,numpy.newaxis,:]

            array = array[:,:,0:3]
            array = array * (255.0 / valMax)
            array = np.fmin (255, np.fmax (0, array))
            array = np.array(array, dtype=np.uint8, order='C')
            process.stdin.write(array.data)
