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

import voxie
import tifffile

import numpy as np

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

# if args.voxie_action != 'Export' and args.voxie_action != 'Import':
if args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))


if args.voxie_action == 'Import':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
        filename = op.Filename

        with tifffile.TiffFile(filename) as tif:
            pageCount = len(tif.pages)
            p0 = tif.pages[0]
            dtype = p0.dtype
            arrayShape = p0.shape + (pageCount,)
            print(dtype, arrayShape)

            # TODO
            spacing = 1e-6
            scaling = 1e6

            gridSpacing = (spacing, spacing, spacing)
            volumeOrigin = (-gridSpacing[0] / 2, -gridSpacing[1] / 2, -gridSpacing[2] / 2)
            if scaling is None:
                memType = dtype
            else:
                memType = np.dtype('f4')
            if memType.kind == 'f':
                ty = ('float', memType.itemsize * 8, 'native')
            elif memType.kind == 'i':
                ty = ('int', memType.itemsize * 8, 'native')
            elif memType.kind == 'u':
                ty = ('uint', memType.itemsize * 8, 'native')
            else:
                raise Exception('Unknown dtype kind: {!r}'.format(memType.kind))

            with instance.CreateVolumeDataVoxel(arrayShape, ty, volumeOrigin, gridSpacing) as resultData:
                with resultData.CreateUpdate() as update, resultData.GetBufferWritable(update) as buffer:
                    outData = buffer[()]

                    op.ThrowIfCancelled()
                    for z in range(pageCount):
                        if scaling is None:
                            outData[:, :, z] = tif.pages[z].asarray()
                        else:
                            outData[:, :, z] = tif.pages[z].asarray() * scaling

                        op.ThrowIfCancelled()
                        op.SetProgress((z + 1) / pageCount)

                    version = update.Finish()
                with version:
                    op.Finish(resultData, version)
else:
    raise Exception()

context.client.destroy()
