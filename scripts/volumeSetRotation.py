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

import numpy as np
import voxie

import sys

# TODO: What happens if this is not a pure rotation matrix?


def rotMatToQuat(mat):
    # https://stackoverflow.com/questions/32208838/rotation-matrix-to-quaternion-equivalence
    # get the real part of the quaternion first
    r = np.math.sqrt(float(1) + mat[0, 0] + mat[1, 1] + mat[2, 2]) * 0.5
    i = (mat[2, 1] - mat[1, 2]) / (4 * r)
    j = (mat[0, 2] - mat[2, 0]) / (4 * r)
    k = (mat[1, 0] - mat[0, 1]) / (4 * r)
    return tuple(map(float, (r, i, j, k)))


parser = voxie.parser

parser.add_argument('--quaternion')
parser.add_argument('--matrix4')
parser.add_argument('--matrix4-ref')

args = parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

obj = instance.Gui.SelectedObjects[0]
# oldRot = obj.GetProperty('de.uni_stuttgart.Voxie.MovableDataNode.Rotation').getValue('(dddd)')
# print (fov)

if args.quaternion is not None:
    rot = tuple(map(float, args.quaternion.split(' ')))
    obj.SetProperty('de.uni_stuttgart.Voxie.MovableDataNode.Rotation',
                    voxie.Variant('(dddd)', rot))
elif args.matrix4 is not None:
    refPos = np.array([0, 0, 0])
    if args.matrix4_ref is not None:
        matrix_ref = tuple(map(float, args.matrix4_ref.split(' ')))
        if len(matrix_ref) != 16:
            raise Exception(
                'Invalid number of ref matrix values: ' + repr(len(matrix_ref)))
        matrix_ref = np.array(matrix_ref).reshape((4, 4), order='F')
        if np.any(matrix_ref[3] != [0, 0, 0, 1]):
            raise Exception(
                'Last line of ref matrix != [0, 0, 0, 1]: ' + repr(matrix_ref[3]))
        if np.any(matrix_ref[0:3, 0:3] != [[1, 0, 0], [0, 1, 0], [0, 0, 1]]):
            raise Exception('ref matrix has rotation: ' +
                            repr(matrix_ref[0:3, 0:3]))
        refPos = matrix_ref[0:3, 3]
        # print (refPos)

    matrix = tuple(map(float, args.matrix4.split(' ')))
    if len(matrix) != 16:
        raise Exception('Invalid number of matrix values: ' +
                        repr(len(matrix)))
    matrix = np.array(matrix).reshape((4, 4), order='F')
    if np.any(matrix[3] != [0, 0, 0, 1]):
        raise Exception(
            'Last line of matrix != [0, 0, 0, 1]: ' + repr(matrix[3]))
    rotMat = matrix[0:3, 0:3]
    trans = matrix[0:3, 3]
    # print (trans)
    # print (rotMat)
    rot = rotMatToQuat(rotMat)
    # TODO: ?
    # Seems like VGStudio ignores the origin of volumes anyway
    trans = voxie.Rotation(rot).inverse * (trans)
    # print (rot)
    # print (trans)

    trans = tuple(-np.array(trans))
    rot = tuple(map(float, voxie.Rotation(rot).inverse.quaternion.value))
    # print (rot)
    # print (trans)

    # b = np.array((0.00025778799317777157, -0.0016201899852603674, 0.0007827000226825476))
    # print (trans / b)

    # TODO: Determine translation

    obj.SetProperty('de.uni_stuttgart.Voxie.MovableDataNode.Rotation',
                    voxie.Variant('(dddd)', rot))
else:
    print('No rotation given')
