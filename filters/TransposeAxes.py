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

import sys

if len(sys.argv) == 1:
    import sys
    import json
    entries = {}
    pos = 0
    # import itertools; list(itertools.permutations([0, 1, 2]))
    rots = [(0, 1, 2), (0, 2, 1), (1, 0, 2), (1, 2, 0), (2, 0, 1), (2, 1, 0)]
    rotsImprop = [0, 1, 1, 0, 0, 1]
    for rot in range(6):
        for mirror_n in range(8):
            pos = pos + 1
            mirror = [
                mirror_n & 4 != 0,
                mirror_n & 2 != 0,
                mirror_n & 1 != 0,
            ]
            isImprop = (rotsImprop[rot] + mirror[0] + mirror[1] + mirror[2]) % 2 != 0
            impropInd = ''
            if isImprop:
                impropInd = ' (improper)'
            elif rot == 0 and mirror_n == 0:
                impropInd = ' (identity)'
            name = 'de.uni_stuttgart.Voxie.Filter.TransposeAxes.Transformation.'
            transformed = ''
            revTransformedParts = [None, None, None]
            for i in range(3):
                r = rots[rot][i]
                revTransformedPart = ''
                if i:
                    name += '_'
                    transformed += ' '
                if mirror[i]:
                    name += 'N'
                    transformed += '-'
                    revTransformedPart += '-'
                else:
                    transformed += ' '
                    revTransformedPart += ' '
                name += chr(88 + r)
                transformed += chr(88 + r)
                revTransformedPart += chr(88 + i)
                revTransformedParts[r] = revTransformedPart
            name += '_to_X_Y_Z'
            revTransformed = ' '.join(revTransformedParts)
            if rot == 0 and mirror_n == 0:
                dn = '{}/{}  {} -> XYZ{}'.format(rot, mirror_n, transformed, impropInd)
            else:
                dn = '{}/{}  {} -> XYZ  |  XYZ -> {}{}'.format(rot, mirror_n, transformed, revTransformed, impropInd)
            if name in entries:
                raise Exception("name in entries: " + repr(name))
            entries[name] = {
                "DisplayName": dn,
                "UIPosition": pos,
            }
    json.dump(entries, sys.stdout, indent=4, allow_nan=False, sort_keys=True, ensure_ascii=False)
    print()
    sys.exit(0)

import numpy as np
import voxie
import dbus

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    # print (pars)
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    # print (properties)
    inputPath = properties['de.uni_stuttgart.Voxie.Input'].getValue('o')
    if inputPath == dbus.ObjectPath('/'):
        raise Exception('No input volume object connected')
    inputProperties = pars[inputPath]['Properties'].getValue('a{sv}')
    inputDataPath = pars[inputPath]['Data'].getValue('o')
    inputData = context.makeObject(context.bus, context.busName, inputDataPath, ['de.uni_stuttgart.Voxie.VolumeDataVoxel'])
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    transformation = properties['de.uni_stuttgart.Voxie.Filter.TransposeAxes.Transformation'].getValue('s')

    prefix = 'de.uni_stuttgart.Voxie.Filter.TransposeAxes.Transformation.'
    suffix = '_to_X_Y_Z'
    if not transformation.startswith(prefix):
        raise Exception('Invalid transformation ' + repr(transformation))
    transformation2 = transformation[len(prefix):]
    if not transformation2.endswith(suffix):
        raise Exception('Invalid transformation ' + repr(transformation))
    transformation2 = transformation2[:-len(suffix)]

    out_str = transformation2.split('_')
    if len(out_str) != 3:
        raise Exception('Invalid transformation ' + repr(transformation))

    rot = [None, None, None]
    mirror = [None, None, None]
    f = [False, False, False]
    for i in range(3):
        s = out_str[i]
        if s.startswith('N'):
            s = s[1:]
            mirror[i] = True
        else:
            mirror[i] = False
        if s == 'X':
            rot[i] = 0
        elif s == 'Y':
            rot[i] = 1
        elif s == 'Z':
            rot[i] = 2
        else:
            raise Exception('Invalid transformation ' + repr(transformation))
        if f[rot[i]]:
            raise Exception('Invalid transformation ' + repr(transformation))
        f[rot[i]] = True

    inputOrigin = inputData.VolumeOrigin
    inputShape = inputData.ArrayShape
    inputSpacing = inputData.GridSpacing
    dataType = inputData.DataType

    outputOrigin = [None, None, None]
    outputShape = [None, None, None]
    outputSpacing = [None, None, None]
    for i in range(3):
        r = rot[i]
        m = mirror[i]
        outputShape[i] = inputShape[r]
        outputSpacing[i] = inputSpacing[r]
        if not m:
            outputOrigin[i] = inputOrigin[r]
        else:
            outputOrigin[i] = -inputOrigin[r] - inputShape[r] * inputSpacing[r]

    transposed = inputData[()].transpose(rot)
    if mirror[0]:
        transposed = transposed[::-1, :, :]
    if mirror[1]:
        transposed = transposed[:, ::-1, :]
    if mirror[2]:
        transposed = transposed[:, :, ::-1]

    with instance.CreateVolumeDataVoxel(outputShape, dataType, outputOrigin, outputSpacing) as data:
        with data.CreateUpdate() as update, data.GetBufferWritable(update) as buffer:
            zCount = outputShape[2]
            for z in range(0, zCount):
                op.ThrowIfCancelled()
                buffer[:, :, z] = transposed[:, :, z]
                op.SetProgress((z + 1) / zCount)
            version = update.Finish()

        with version:
            result = {}
            result[outputPath] = {
                'Data': voxie.Variant('o', data._objectPath),
                'DataVersion': voxie.Variant('o', version._objectPath),
            }
            op.Finish(result)

context.client.destroy()
