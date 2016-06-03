#!/usr/bin/python3

import sys
import os
import numpy
import time
import scipy.misc

import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

slice = instance.getSlice(args)

dataSet = slice.getDataSet()

data = dataSet.getFilteredData()

with instance.createClient() as client:
    pixSize = 100e-6
    slicethick = pixSize / 2
    #slicethick = pixSize * 10

    plane = slice.dbus_properties.Get('de.uni_stuttgart.Voxie.Slice', 'Plane')
    origin = numpy.array(plane[0])
    orientation = plane[1]
    rotation = voxie.Rotation (orientation)

    size = numpy.array(data.get('Size'), dtype='uint64')
    dorigin = numpy.array(data.get('Origin'), dtype=numpy.double)
    dspacing = numpy.array(data.get('Spacing'), dtype=numpy.double)
    posmin = posmax = None
    for corner in [[0, 0, 0], [0, 0, 1], [0, 1, 0], [0, 1, 1], [1, 0, 0], [1, 0, 1], [1, 1, 0], [1, 1, 1]]:
        cpos = numpy.array(corner,dtype=numpy.double) * size
        cpos = rotation * (dorigin + dspacing * cpos)
        if posmin is None:
            posmin = cpos
        if posmax is None:
            posmax = cpos
        posmin = numpy.minimum (posmin, cpos)
        posmax = numpy.maximum (posmax, cpos)
        #print (cpos)
    #print (posmin)
    #print (posmax)

    posmin[2] = 0
    posmax[2] = 0.004
#    posmin[0] = posmin[1] = 0
#    posmax[0] = posmax[1] = 0.01

    width = int ((posmax[0] - posmin[0]) / pixSize + 1)
    height = int ((posmax[1] - posmin[1]) / pixSize + 1)
    with instance.createImage(client, (width, height)) as image, image.getDataReadonly() as buffer:
        oarray = buffer.array
    
        avg = []
    
        options = {}
        options['Interpolation'] = 'NearestNeighbor'
        options['Interpolation'] = 'Linear'
    
        #pos0 = (posmin[0], posmin[1], (rotation.inverse * origin)[2])
        print (int ((posmax[2] - posmin[2]) / slicethick + 1))
        #for i in range (int ((posmax[2] - posmin[2]) / slicethick + 1)):
        for i in [int ((posmax[2] - posmin[2]) / slicethick + 1)//2]:
            print(i)
            pos0 = (posmin[0], posmin[1], posmin[2] + slicethick * i)
    
            #print (rotation)
            #print (rotation.inverse)
            #print (pos0)
            pos = rotation * pos0
            data.dbus.ExtractSlice (pos, orientation, (width, height), (pixSize, pixSize), image.path, options)
        
            image.dbus.UpdateBuffer({})
        
            array = oarray
        
            array = numpy.array(array)
            array[numpy.isnan(array)] = 0
    
            cutoff=40
            array[array>cutoff] = cutoff
            avg.append (numpy.mean (array))
    
            #print (i)
            #continue
        
            #array = array[::-1, ::-1]
            array = array.transpose()
        
            #print (array)
            #print (array.shape)
            #print (array.strides)
            array = array[:,:,numpy.newaxis] * numpy.ones(4)[numpy.newaxis,numpy.newaxis,:]
            array[:,:,3] = (1 - numpy.isnan(oarray).transpose()) * 40
            fn = '/tmp/out-%04d.png' % i
            scipy.misc.toimage(array, cmin = 0.0, cmax = 40.0).save(fn)
            #print (array[1,0:100])
    
    os.system('mdbus2 ' + instance.bus_name + ' | grep Image')

print (avg)

os.system('mdbus2 ' + instance.bus_name + ' | grep Client')

os.system('ls -l /proc/$PPID/fd/')
