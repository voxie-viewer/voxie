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

import voxie

import PyQt5.QtCore
import PyQt5.QtWidgets

app = PyQt5.QtWidgets.QApplication(sys.argv)

# PyQt5.QtWidgets.QMessageBox.information(None, "foo", "bar")

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

voxieWindowID = instance.dbus_gui.GetMainWindowID({})
# print (voxieWindowID)

dataSet = instance.getVolumeObject()

data = dataSet.getVolumeDataVoxel()

array = data.getDataReadonly().array


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


# (destDir, x) = PyQt5.QtWidgets.QFileDialog.getSaveFileName(None, "Directory for image stack")
fileDialog = PyQt5.QtWidgets.QFileDialog()
fileDialog.setAcceptMode(PyQt5.QtWidgets.QFileDialog.AcceptSave)
# fileDialog.setFileMode(PyQt5.QtWidgets.QFileDialog.Directory)
fileDialog.setOption(PyQt5.QtWidgets.QFileDialog.ShowDirsOnly)
setTransientParent(fileDialog)
ok = fileDialog.exec()
if not ok:
    sys.exit(0)

destDir = fileDialog.selectedFiles()[0]
if destDir is None or destDir == '':
    sys.exit(0)

os.mkdir(destDir)

zCount = array.shape[2]

min = 0
max = numpy.max(array)

print('Max value: %f' % (max))

for z in range(0, zCount):
    ar = array[:, :, z]
    scipy.misc.toimage(ar, cmin=min, cmax=max).save(
        destDir + '/img-%04d.tiff' % z)
