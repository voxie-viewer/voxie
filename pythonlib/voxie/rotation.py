import numpy
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
import math


class Quaternion(object):
    def __init__(self, value):
        self.value = numpy.array(value, dtype=numpy.double)

    def __mul__(self, other):
        if isinstance(other, Quaternion):
            a = self.a * other.a - self.b * other.b - self.c * other.c - self.d * other.d
            b = self.a * other.b + self.b * other.a + self.c * other.d - self.d * other.c
            c = self.a * other.c - self.b * other.d + self.c * other.a + self.d * other.b
            d = self.a * other.d + self.b * other.c - self.c * other.b + self.d * other.a
            return Quaternion((a, b, c, d))
        else:
            return Quaternion(self.value * other)

    def absolute(self):
        return numpy.sqrt(numpy.sum(self.value * self.value))

    def conjugate(self):
        return Quaternion((self.a, -self.b, -self.c, -self.d))

    def __str__(self):
        return '(%s, %s, %s, %s)' % (self.a, self.b, self.c, self.d)

    a = property(lambda self: self.value[0])
    b = property(lambda self: self.value[1])
    c = property(lambda self: self.value[2])
    d = property(lambda self: self.value[3])


class Rotation(object):
    def __init__(self, quaternion):
        if not isinstance(quaternion, Quaternion):
            quaternion = Quaternion(quaternion)
        absolute = quaternion.absolute()
        if absolute < 0.99 or absolute > 1.01:
            raise Exception(
                'Absolute value of quaternion %s (%s) is outside range 0.99-1.01' %
                (quaternion, absolute))
        self.quaternion = quaternion * (1 / absolute)

    def __mul__(self, other):
        if isinstance(other, Rotation):
            return Rotation(self.quaternion * other.quaternion)
        else:
            (x, y, z) = other
            res = self.quaternion * \
                Quaternion((0, x, y, z)) * self.quaternion.conjugate()
            return res.value[1], res.value[2], res.value[3]

    @property
    def inverse(self):
        return Rotation(self.quaternion.conjugate())

    def __str__(self):
        return 'Rotation%s' % (self.quaternion)

    def fromAxisAngle(axis, rad):
        sin = math.sin(rad / 2)
        cos = math.cos(rad / 2)
        axis = numpy.array(axis)
        axis = axis / numpy.sqrt(numpy.sum(axis * axis))
        (x, y, z) = axis
        return Rotation((cos, x * sin, y * sin, z * sin))

    def fromAxisAngleDeg(axis, deg):
        return Rotation.fromAxisAngle(axis, deg * math.pi / 180)
