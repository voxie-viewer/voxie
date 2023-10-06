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
import os
import tarfile
import codecs

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    filterPath = op.FilterObject
    pars = op.Parameters
    properties = pars[filterPath._objectPath]['Properties'].getValue('a{sv}')
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    TAR = './lib/C_MC33/Closed_Surfaces.tar.xz'
    # TAR = './lib/C_MC33/MarchingCubes_cases.tar.xz'

    DIR = 'Closed_Surfaces/Grids'
    # DIR = 'MarchingCubes_cases/Grids'

    tar = tarfile.open(TAR, 'r')

    # volAmount = len([name for name in os.listdir(
    #     DIR) if ".raw" in name and os.path.isfile(os.path.join(DIR, name))])
    volAmount = len([f for f in tar if f.name.endswith(".raw") and f.isfile()])

    size = 0
    littleEndian = False
    # with open(os.path.join(DIR, next(name for name in os.listdir(DIR) if ".nhdr" in name)), 'r') as nhdrFile:
    with codecs.getreader('utf-8')(tar.extractfile(next(f for f in tar if f.name.endswith(".nhdr")))) as nhdrFile:
        for line in nhdrFile:
            if "sizes: " in line:
                # Assuming all volume data are cubic
                size = int(line.split(' ')[1])
            if "endian: " in line and "little" in line.split(' ')[1]:
                littleEndian = True

    # Amount of volumes in each dimension
    dimensionAmount = int(np.ceil(volAmount**(1 / 3.0)))

    arrayShape = (dimensionAmount * 2 * size, dimensionAmount *
                  2 * size, dimensionAmount * 2 * size)
    volumeOrigin = (0.0, 0.0, 0.0)
    volumeSpacing = (0.5, 0.5, 0.5)
    with instance.CreateVolumeDataVoxel(arrayShape, ('float', 32, 'native'), volumeOrigin, volumeSpacing) as vol:
        with vol.CreateUpdate() as update:
            buffer = vol.GetBufferWritable(update)
            buffer[:] = 0

            # fileNames = [n for n in os.listdir(DIR) if '.raw' in n]
            fileNames = [f for f in tar if f.name.endswith(".raw") and f.isfile()]
            for i, name in enumerate(fileNames):
                xPos = 2 * size * (i % dimensionAmount)
                yPos = 2 * size * \
                    (i % (dimensionAmount ** 2) // dimensionAmount)
                zPos = 2 * size * (i // (dimensionAmount ** 2))

                data = []
                if littleEndian:
                    # data = np.fromfile(os.path.join(DIR, name), dtype="<f4")
                    data = np.frombuffer(tar.extractfile(name).read(), dtype="<f4")
                else:
                    # data = np.fromfile(os.path.join(DIR, name), dtype=">f4")
                    data = np.frombuffer(tar.extractfile(name).read(), dtype=">f4")
                data = data.reshape(size, size, size)

                # 1 - data because 1=air in original data
                buffer[xPos:xPos + size, yPos:yPos +
                       size, zPos:zPos + size] = 1 - data

                # Progressbar (only update every percent)
                if i % (len(fileNames) / 100) == 0:
                    op.SetProgress(i / len(fileNames))

            version = update.Finish()

        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', vol._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        op.Finish(result)
