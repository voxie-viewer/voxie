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

# Requires NBT (https://github.com/twoolie/NBT), tested with commit
# b06dd6cc8117d2788da1d8416e642d58bad45762

import numpy as np
import voxie
import sys
import os


class BlockIDManager:
    def __init__(self, block_ids):
        self.offset = 1

        self.idToName = {}
        self.nameToID = {}
        self.nextID = self.offset
        self.usedIDs = set()
        # return
        for id in block_ids:
            name = block_ids[id]
            id = id + self.offset
            self.idToName[id] = name
            self.nameToID[name] = id
            self.nextID = max(self.nextID, id + 1)

    def getID(self, name):
        if name is None:
            id = 0
            self.usedIDs.add(id)
            return id
        if name.startswith('minecraft:'):
            name = name[len('minecraft:'):]
        if name in self.nameToID:
            id = self.nameToID[name]
            self.usedIDs.add(id)
            return id
        if name.startswith('unknown_'):
            id = int(name[len('unknown_'):]) + self.offset
            if id not in self.idToName:
                self.idToName[id] = name
                self.nameToID[name] = id
                self.usedIDs.add(id)
                return id
        id = self.nextID
        while id in self.idToName:
            id = id + 1
        self.nextID = id + 1
        self.idToName[id] = name
        self.nameToID[name] = id
        print('Unknown block name {name!r}, assigning ID {id}'.format(
            name=name, id=id), flush=True)
        self.usedIDs.add(id)
        return id

    def addToTable(self, labelData, labelUpdate):
        if 0 in self.usedIDs:
            row = [
                voxie.Variant('x', 0),
                voxie.Variant('s', '-'),
            ]
            labelData.AddRow(labelUpdate, row)
        ids = list(self.idToName)
        ids.sort()
        for id in ids:
            if id in self.usedIDs:
                row = [
                    voxie.Variant('x', id),
                    voxie.Variant('s', self.idToName[id]),
                ]
                labelData.AddRow(labelUpdate, row)


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
    outputPath = properties['de.uni_stuttgart.Voxie.Output'].getValue('o')
    outputLabelPath = properties['de.uni_stuttgart.Voxie.Filter.ImportMinecraftLevel.OutputLabels'].getValue(
        'o')
    filename = properties['de.uni_stuttgart.Voxie.Filter.ImportMinecraftLevel.FileName'].getValue(
        's')
    # print ('Filename: ' + repr(filename))

    # sys.path.append(os.path.join(os.path.dirname(
    #     __file__), 'importMinecraftLevel.nbt'))
    import nbt
    blockManager = BlockIDManager(nbt.chunk.block_ids)

    world = nbt.world.WorldFolder(filename)
    bb = world.get_boundingbox()
    print('Bounding box: ' + repr(bb), flush=True)
    sizeChunks = (bb.maxx - bb.minx + 1, 16, bb.maxz - bb.minz + 1)
    originChunks = (bb.minx, 0, bb.minz)
    size = tuple(map(lambda i: i * 16, sizeChunks))
    origin = tuple(map(lambda i: i * 16, originChunks))

    # dataType = ('uint', 16, 'native') # There seems to be some problem with
    # the slice visualizer for this type
    dataType = ('float', 32, 'native')

    columns = [
        ('LabelID', instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.PropertyType',
                                                     'de.uni_stuttgart.Voxie.PropertyType.Int').CastTo('de.uni_stuttgart.Voxie.PropertyType'), 'Label ID', {}, {}),
        ('Name', instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.PropertyType',
                                                  'de.uni_stuttgart.Voxie.PropertyType.String').CastTo('de.uni_stuttgart.Voxie.PropertyType'), 'Name', {}, {}),
    ]
    with instance.CreateVolumeDataVoxel(size, dataType, origin, (1, 1, 1)) as data, instance.CreateTableData(columns) as labelData:
        with data.CreateUpdate() as update, labelData.CreateUpdate() as labelUpdate:
            buffer = data.GetBufferWritable(update)
            buffer[:] = 0
            t = world.chunk_count()
            i = 0
            for chunk in world.iter_chunks():
                # break
                op.ThrowIfCancelled()
                xc, zc = chunk.get_coords()
                # print (xc, zc, flush=True)
                # buffer[(xc*16)+origin[0] : (xc*16)+origin[0]+16, :, (zc*16)+origin[2] : (zc*16)+origin[2]+16] = 1

                # Use chunk.get_max_height() to speed up import
                # ymax = 256
                ymax = chunk.get_max_height() + 1
                # print(xc, zc, ymax)
                for zi in range(16):
                    for xi in range(16):
                        for yi in range(ymax):
                            x = (xc - originChunks[0]) * 16 + xi
                            y = yi
                            z = (zc - originChunks[2]) * 16 + zi
                            block_name = chunk.get_block(xi, yi, zi)
                            # print (x, y, z, block_name)
                            if x < 0 or y < 0 or z < 0:
                                raise Exception(
                                    'x < 0 or y < 0 or z < 0: {x} {y} {z}'.format(x=x, y=y, z=z))
                            buffer[x, y, z] = blockManager.getID(block_name)
                i += 1
                op.SetProgress(i / t)
                # if i > 500:
                #    break

            blockManager.addToTable(labelData, labelUpdate)

            version = update.Finish()
            labelVersion = labelUpdate.Finish()

        result = {}
        result[outputPath] = {
            'Data': voxie.Variant('o', data._objectPath),
            'DataVersion': voxie.Variant('o', version._objectPath),
        }
        result[outputLabelPath] = {
            'Data': voxie.Variant('o', labelData._objectPath),
            'DataVersion': voxie.Variant('o', labelVersion._objectPath),
        }
        op.Finish(result)
