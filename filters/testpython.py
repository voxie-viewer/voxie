import numpy as np
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
import time

from scipy.ndimage.filters import uniform_filter


def windowMeanStd(arr, templateShape):
    origin = tuple(int(-x / 2) for x in templateShape)
    mean = uniform_filter(arr, templateShape, mode='constant', origin=origin)
    meansqrd = uniform_filter(arr * arr, templateShape,
                              mode='constant', origin=origin)
    return mean[:-templateShape[0] + 1, :-templateShape[1] + 1, :-templateShape[2] + 1],\
        ((meansqrd - mean * mean)**.5)[:-templateShape[0] +
                                       1, :-templateShape[1] + 1, :-templateShape[2] + 1]


def std(image, templateShape):
    result = np.zeros(np.subtract(image.shape, templateShape) + 1)
    for i in range(np.shape(result)[0]):
        for j in range(np.shape(result)[1]):
            for k in range(np.shape(result)[2]):
                result[i][j][k] = np.std(
                    image[i:i + templateShape[0], j:j + templateShape[1], k:k + templateShape[2]])
    return result


def mean(image, templateShape):
    result = np.zeros(np.subtract(image.shape, templateShape) + 1)
    for i in range(np.shape(result)[0]):
        for j in range(np.shape(result)[1]):
            for k in range(np.shape(result)[2]):
                result[i][j][k] = np.mean(
                    image[i:i + templateShape[0], j:j + templateShape[1], k:k + templateShape[2]])
    return result


def window_mean(arr, templateShape):
    origin = tuple(int(-ti / 2) for ti in templateShape)
    c1 = uniform_filter(arr, templateShape, mode='constant', origin=origin)
    return (c1)[:-templateShape[0] + 1, :-
                templateShape[1] + 1, :-templateShape[2] + 1]


templateShape = (8, 8, 8)
a = np.random.rand(12, 12, 12)


print("sliding window")
print(windowMeanStd(a, templateShape))
