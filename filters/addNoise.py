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
import dbus

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    inputPath = properties['de.uni_stuttgart.Voxie.Input'].getValue('o')
    if inputPath == dbus.ObjectPath('/'):
        raise Exception('No input volume object connected')
    inputProperties = pars[inputPath]['Properties'].getValue('a{sv}')
    inputDataPath = pars[inputPath]['Data'].getValue('o')
    inputData = context.makeObject(context.bus, context.busName, inputDataPath, [
                                   'de.uni_stuttgart.Voxie.VolumeDataVoxel'])
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    sigma = properties['de.uni_stuttgart.Voxie.Filter.AddNoise.Sigma'].getValue('d')
    minSigma = properties['de.uni_stuttgart.Voxie.Filter.AddNoise.MinSigma'].getValue('d')
    maxSigma = properties['de.uni_stuttgart.Voxie.Filter.AddNoise.MaxSigma'].getValue('d')
    minMean = properties['de.uni_stuttgart.Voxie.Filter.AddNoise.MinMean'].getValue('d')
    maxMean = properties['de.uni_stuttgart.Voxie.Filter.AddNoise.MaxMean'].getValue('d')
    scaling = properties['de.uni_stuttgart.Voxie.Filter.AddNoise.Scaling'].getValue('d')
    method = properties['de.uni_stuttgart.Voxie.Filter.AddNoise.Method'].getValue('s')

    if (minMean - maxMean) == 0:
        b = 0
    else:
        b = (np.log(minSigma) - np.log(maxSigma)) / (minMean - maxMean)
    a = minSigma / np.exp(b * minMean)

    origin = inputData.VolumeOrigin
    sizeOrig = inputData.ArrayShape
    spacingOrig = np.array(inputData.GridSpacing)

    dataType = ('float', 32, 'native')  # TODO?
    with instance.CreateVolumeDataVoxel(sizeOrig, dataType, origin, spacingOrig) as data:
        with data.CreateUpdate() as update, data.GetBufferWritable(update) as buffer:
            zCount = sizeOrig[2]
            if method == 'de.uni_stuttgart.Voxie.Filter.AddNoise.Method.ConstantSigma':
                for z in range(0, zCount):
                    op.ThrowIfCancelled()
                    noise = np.random.normal(0, sigma, sizeOrig[0:2])
                    buffer[:, :, z] = inputData[:, :, z] + noise[:]
                    op.SetProgress((z + 1) / zCount)
            elif method == 'de.uni_stuttgart.Voxie.Filter.AddNoise.Method.ExpSigma':
                if maxSigma < minSigma:
                    raise Exception('Min sigma must be < max sigma')
                for z in range(0, zCount):
                    op.ThrowIfCancelled()
                    sigmaSlice = np.copy(inputData[:, :, z])
                    sigmaSlice[sigmaSlice > maxSigma] = maxSigma
                    sigmaSlice[sigmaSlice < minSigma] = minSigma
                    sigmaSlice = a * np.exp(b * sigmaSlice)
                    noise = scaling * np.random.normal(0, sigmaSlice)
                    buffer[:, :, z] = inputData[:, :, z] + noise[:]
                    op.SetProgress((z + 1) / zCount)
            else:
                raise Exception('Unknown threshold method: ' + repr(method))
            version = update.Finish()

        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', data._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
