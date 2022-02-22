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
import numpy as np

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + args.voxie_action)

with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo(
        'de.uni_stuttgart.Voxie.VolumeData')
    tableData = op.GetInputData('de.uni_stuttgart.Voxie.Filter.ExtractLabelObject.Table').CastTo(
        'de.uni_stuttgart.Voxie.TableData')
    outputPath = op.Properties['de.uni_stuttgart.Voxie.Output'].getValue('o')

    inputDataVoxel = inputData.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    columns = tableData.Columns
    colNr = None
    for i in range(len(columns)):
        name = columns[i][0]
        ty = columns[i][1]
        if name == 'LabelID':
            # print (ty)
            tyObj = context.makeObject(context.bus, context.busName, ty, [
                                       'de.uni_stuttgart.Voxie.PropertyType'])
            if tyObj.Name != 'de.uni_stuttgart.Voxie.PropertyType.Int':
                raise Exception('LabelID column does not have type Int')
            if colNr is not None:
                raise Exception('Got multiple LabelID columns')
            colNr = i
    if colNr is None:
        raise Exception('Got no LabelID column')

    op.SetProgress(0.1)
    labelList = []
    # TODO: Get in multiple steps to avoid problems with DBus maximum message
    # size
    for row in tableData.GetRows(0, 2**64 - 1):
        rowData = row[1]
        colData = rowData[colNr].getValue('x')  # Int is 'x'
        # print (colData)
        labelList.append(colData)
    labelList = np.array(labelList)
    op.SetProgress(0.3)

    with inputDataVoxel.GetBufferReadonly() as inputArray:
        with op.GetOutputVolumeDataVoxelLike(outputPath, inputDataVoxel) as data:
            with data.CreateUpdate() as update, data.GetBufferWritable(update) as outputBuffer:
                # TODO: Progress / cancellation?
                # zCount = data[:].shape[2]
                # for z in range(0, zCount):
                #    op.ThrowIfCancelled()
                #    ...
                #    op.SetProgress((z + 1) / zCount)

                # Entry is False when voxel belongs to label in label_list,
                # otherwise True.
                op.SetProgress(0.5)
                mask = np.logical_not(np.isin(inputArray, labelList))
                op.SetProgress(0.7)
                result = np.ma.masked_where(mask, inputArray)
                op.SetProgress(0.85)
                # Replace masked voxels with 0 label.
                outputBuffer[:] = np.ma.MaskedArray(
                    result, fill_value=0).filled()
                op.SetProgress(1.0)

                version = update.Finish()

            result = {}
            result[outputPath] = {
                'Data': voxie.Variant('o', data._objectPath),
                'DataVersion': voxie.Variant('o', version._objectPath),
            }
            op.Finish(result)
            version._referenceCountingObject.destroy()
