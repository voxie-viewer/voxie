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
import numpy
import time
import scipy.misc
import numpy as np

import voxie

import PyQt5.QtCore
import PyQt5.QtWidgets

app = PyQt5.QtWidgets.QApplication(sys.argv)

# PyQt5.QtWidgets.QMessageBox.information(None, "foo", "bar")

instance = voxie.instanceFromArgs()

voxieWindowID = instance.Gui.GetMainWindowID()
# print (voxieWindowID)


def setTransientParent(widget):
    voxieWindow = None
    if voxieWindowID is not None:
        voxieWindow = PyQt5.QtGui.QWindow.fromWinId(voxieWindowID)
    widget.winId()  # Trigger creation of native window
    window = None
    if 'windowHandle' in dir(widget):  # Available starting with v5.3.2
        window = widget.windowHandle()
    if voxieWindow is not None and window is not None:
        window.setTransientParent(voxieWindow)


def showDialog(title, msg):
    dialog = PyQt5.QtWidgets.QMessageBox()
    setTransientParent(dialog)
    dialog.setWindowTitle(title)
    dialog.setText(msg)
    ok = dialog.exec()


try:
    nodes = instance.Gui.SelectedNodes
    # print(nodes)
    if len(nodes) != 2:
        showDialog('extract-profile-line', 'Two nodes (dataset and plane) must be selected')
        sys.exit(0)
    plane = None
    ds = None
    for node in nodes:
        # print(node.Prototype.Name)
        if node.Prototype.NodeKind == 'de.uni_stuttgart.Voxie.NodeKind.Property' and node.Prototype.Name == 'de.uni_stuttgart.Voxie.Property.Plane':
            if plane is not None:
                showDialog('extract-profile-line', 'Got multiple planes')
                sys.exit(0)
            plane = node
        elif node.Prototype.NodeKind == 'de.uni_stuttgart.Voxie.NodeKind.Data' and node.Prototype.Name == 'de.uni_stuttgart.Voxie.Data.Volume':
            if ds is not None:
                showDialog('extract-profile-line', 'Got multiple volumes')
                sys.exit(0)
            ds = node

    origin = plane.GetProperty('de.uni_stuttgart.Voxie.Property.Plane.Origin').getValue('(ddd)')
    orientation = plane.GetProperty('de.uni_stuttgart.Voxie.Property.Plane.Orientation').getValue('(dddd)')
    # print(origin, orientation)

    filename = '/tmp/test'

    data = ds.CastTo('de.uni_stuttgart.Voxie.DataObject').Data.CastTo('de.uni_stuttgart.Voxie.VolumeData')
    data = data.CastTo('de.uni_stuttgart.Voxie.VolumeDataVoxel')

    pixSize = np.mean(data.GridSpacing) / 3
    pixCount = 4000

    shift = np.asarray((-pixCount / 2, -0.5, 0))
    shift2 = np.asarray((pixCount / 2, 0.5, 0))

    pos = np.asarray(origin) + voxie.Rotation(orientation) * (pixSize * shift)
    print(origin, orientation, pixSize, shift, pos)
    print(np.asarray(pos) + voxie.Rotation(orientation) * (pixSize * np.asarray((0, 0, 0))))
    print(np.asarray(pos) + voxie.Rotation(orientation) * (pixSize * np.asarray((pixCount, 1, 0))))

    width = pixCount
    height = 1

    options = {}
    # options['Interpolation'] = voxie.Variant('s', 'NearestNeighbor')
    options['Interpolation'] = voxie.Variant('s', 'Linear')

    with instance.CreateImage((width, height), 1, ('float', 32, 'native')) as image, image.GetBufferReadonly() as buffer:
        instance.Utilities.ExtractSlice(data, pos, orientation, (width, height), (pixSize, pixSize), image, options)
        data = np.asarray(buffer)[:, 0, 0]

    # print(data)

    # (filename, x) = PyQt5.QtWidgets.QFileDialog.getSaveFileName(None, "Directory for image stack")
    fileDialog = PyQt5.QtWidgets.QFileDialog()
    fileDialog.setAcceptMode(PyQt5.QtWidgets.QFileDialog.AcceptSave)
    # fileDialog.setFileMode(PyQt5.QtWidgets.QFileDialog.Directory)
    # fileDialog.setOption(PyQt5.QtWidgets.QFileDialog.ShowDirsOnly)
    setTransientParent(fileDialog)
    ok = fileDialog.exec()
    if not ok:
        sys.exit(0)

    filename = fileDialog.selectedFiles()[0]
    if filename is None or filename == '':
        sys.exit(0)

    np.savetxt(filename, data)
except Exception as e:
    showDialog('extract-profile-line error', str(e))
    raise
