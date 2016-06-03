#!/usr/bin/python3

import numpy

import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

slice = instance.getSlice(args)

with instance.createClient() as client:
    plane = slice.dbus_properties.Get('de.uni_stuttgart.Voxie.Slice', 'Plane')
    origin = numpy.array(plane[0])
    orientation = plane[1]
    rotation = voxie.Rotation (orientation)

    originPlane = rotation.inverse * origin

    print (originPlane[2])
