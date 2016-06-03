#!/usr/bin/python3

import numpy
import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

slice = instance.getSlice(args)

dataSet = slice.getDataSet()

data = dataSet.getFilteredData()

array = data.getDataReadonly().array

avg = numpy.mean(array, dtype = numpy.double)
count = numpy.product(array.shape)

str = "The average of %d values is %f" % (count, avg)

print (str)
