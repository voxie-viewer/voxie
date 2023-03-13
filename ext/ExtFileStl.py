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
import sys
import struct
import collections
import dataclasses
import typing

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'Export' and args.voxie_action != 'Import':
    raise Exception('Invalid operation: ' + args.voxie_action)

if args.voxie_action == 'Export':
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationExport']).ClaimOperationAndCatch() as op:
        raise Exception('TODO: Implement exporter')
else:
    with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationImport']).ClaimOperationAndCatch() as op:
        filename = op.Filename

        properties = op.Properties
        unifyVertices = properties['de.uni_stuttgart.Voxie.FileFormat.Surface.Stl.Import.UnifyVertices'].getValue('b')
        # print('Unify vertices: ' + str(unifyVertices))

        dt = np.dtype([
            # ('normal1', '<f'), ('normal2', '<f'), ('normal3', '<f'),
            # ('v1_1', '<f'), ('v1_2', '<f'), ('v1_3', '<f'),
            # ('v2_1', '<f'), ('v2_2', '<f'), ('v2_3', '<f'),
            # ('v3_1', '<f'), ('v3_2', '<f'), ('v3_3', '<f'),
            ('normal', '<3f'),
            ('v1', '<3f'), ('v2', '<3f'), ('v3', '<3f'),
            ('attribute_byte_count', '<u2'),
        ])

        with open(filename, 'rb') as file:
            header = file.read(80)
            if header.startswith(b'solid'):
                raise Exception('ASCII STL files not supported')

            count = struct.unpack('<I', file.read(4))[0]
            print('Got {} triangles'.format(count), flush=True)

            op.SetProgress(0.02)

            entries = np.fromfile(file, dtype=dt, count=count)
            if entries.shape[0] != count:
                raise Exception('Expected {} entries, got {}'.format(count, entries.shape[0]))

        op.SetProgress(0.5)

        vertices = np.empty((count * 3, 3), dtype=np.float32)
        vertices[::3] = entries['v1']
        vertices[1::3] = entries['v2']
        vertices[2::3] = entries['v3']
        del entries

        triangles = np.empty((count, 3), dtype=np.uint32)
        triangles[:, 0] = np.arange(0, count * 3, 3, dtype=np.uint32)
        triangles[:, 1] = np.arange(0, count * 3, 3, dtype=np.uint32) + 1
        triangles[:, 2] = np.arange(0, count * 3, 3, dtype=np.uint32) + 2

        op.SetProgress(0.55)

        # Unify vertices
        if unifyVertices:
            vertices, translate = np.unique(vertices, axis=0, return_inverse=True)
            op.SetProgress(0.9)
            triangles = translate[triangles]

        op.SetProgress(0.95)

        srf2t = instance.CreateSurfaceDataTriangleIndexed(
            triangles.shape[0], 0, None, True)
        with srf2t.CreateUpdate() as update:
            buffer = voxie.Buffer(srf2t.GetTrianglesWritable(update), 2, True)
            buffer[:] = triangles
            update.Finish()

        op.SetProgress(0.97)

        srf2 = instance.CreateSurfaceDataTriangleIndexed(
            triangles.shape[0], vertices.shape[0], srf2t, False)
        with srf2.CreateUpdate() as update:
            buffer = voxie.Buffer(srf2.GetVerticesWritable(update), 2, True)
            buffer[:] = vertices
            version = update.Finish()
        with version:
            op.Finish(srf2, version)

context.client.destroy()
