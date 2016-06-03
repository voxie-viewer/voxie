#!/usr/bin/python3

import numpy
import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

slice = instance.getSlice(args)

dataSet = slice.getDataSet()

data = dataSet.getOriginalData()

#array = data.getDataReadonly().array
array = data.getDataWritable().array
print (numpy.mean(array))
print (numpy.mean(array[43:86,43:86,43:86]))
#array[43:86,43:86,43:86] = 0
array[43:86,43:86,43:86] = 1 - array[43:86,43:86,43:86]
print (numpy.mean(array[43:86,43:86,43:86]))
data.dbus.UpdateFromBuffer({})
