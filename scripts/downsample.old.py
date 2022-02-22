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
import os
import numpy as np
import time
import dbus

import voxie

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

dataSet = instance.getVolumeObject()

data = dataSet.getVolumeDataVoxel()


def setTransientParent(widget, voxieWindowID):
    import PyQt5.QtGui
    voxieWindow = None
    if voxieWindowID is not None:
        voxieWindow = PyQt5.QtGui.QWindow.fromWinId(voxieWindowID)
    widget.winId()  # Trigger creation of native window
    window = None
    if 'windowHandle' in dir(widget):  # Available starting with v5.3.2
        window = widget.windowHandle()
    if voxieWindow is not None and window is not None:
        window.setTransientParent(voxieWindow)


def getFactor():
    import PyQt5.QtCore
    import PyQt5.QtWidgets
    app = PyQt5.QtWidgets.QApplication(sys.argv)
    voxieWindowID = instance.dbus_gui.GetMainWindowID()
    # print (voxieWindowID)
    dialog = PyQt5.QtWidgets.QInputDialog()
    dialog.setLabelText(
        'Enter the downsampling factor (will do downsampling with averaging)')
    dialog.setInputMode(PyQt5.QtWidgets.QInputDialog.IntInput)
    dialog.setIntMinimum(2)
    setTransientParent(dialog, voxieWindowID)
    ok = dialog.exec()
    if not ok:
        sys.exit(0)
    return dialog.intValue()


# factor = 2
factor = getFactor()

nameOrig = dataSet.get('DisplayName')

origin = np.array(data.get('Origin'), dtype=np.double)
sizeOrig = np.array(data.get('Size'), dtype='uint64')
spacingOrig = np.array(data.get('Spacing'), dtype=np.double)
print(nameOrig, origin, sizeOrig, spacingOrig)

# TODO: Don't cut away data at the end

name = '%s downsampled by %s' % (nameOrig, factor)
# size = ((int(sizeOrig[0]) + factor - 1) // factor,
#        (int(sizeOrig[1]) + factor - 1) // factor,
#        (int(sizeOrig[2]) + factor - 1) // factor)
size = (int(sizeOrig[0]) // factor,
        int(sizeOrig[1]) // factor,
        int(sizeOrig[2]) // factor)
spacing = spacingOrig * factor

with instance.createClient() as client:
    with data.getDataReadonly() as bufferOld:
        arrayOld = bufferOld.array

        arrayOld2 = arrayOld[:size[0] * factor, :size[1] * factor, :size[2] * factor]

        arrayOld3 = arrayOld2.view()
        arrayOld3.shape = size[0], factor, size[1], factor, size[2], factor

        with instance.createVolumeDataVoxel(client, size, {'Spacing': tuple(spacing), 'Origin': tuple(origin)}) as data, data.getDataWritable() as bufferNew:
            array = bufferNew.array
            array[:] = 0

            for z in range(arrayOld3.shape[4]):
                array[:, :, z] = np.mean(
                    arrayOld3[:, :, :, :, z, :], axis=(1, 3, 4))

            dataSet = instance.createVolumeObject(name, data)

    # slicePlugin = instance.getPlugin('Vis3D')
    # visualizerPrototype = slicePlugin.getMemberDBus('de.uni_stuttgart.Voxie.VisualizerPrototype', 'IsosurfaceMetaVisualizer')
    # visualizerPrototype.Create([dataSet.path], dbus.Array(signature='o'), voxie.emptyOptions)
