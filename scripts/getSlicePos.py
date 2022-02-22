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

import PyQt5.QtWidgets
import PyQt5.QtCore
import numpy
import voxie
import sys

args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

obj = instance.Gui.SelectedObjects[0]
ty = obj.Prototype.Name
if ty == 'de.uni_stuttgart.Voxie.Property.Plane':
    origin = obj.GetProperty(
        'de.uni_stuttgart.Voxie.Property.Plane.Origin').getValue('(ddd)')
    orientation = voxie.Rotation(obj.GetProperty(
        'de.uni_stuttgart.Voxie.Property.Plane.Orientation').getValue('(dddd)'))
elif ty == 'de.uni_stuttgart.Voxie.SliceVisualizer':
    origin = obj.GetProperty(
        'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin').getValue('(ddd)')
    orientation = voxie.Rotation(obj.GetProperty(
        'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation').getValue('(dddd)'))
else:
    raise Exception('Unknown object type: ' + repr(ty))

originPlane = orientation.inverse * origin

print(originPlane[2])

app = PyQt5.QtWidgets.QApplication(sys.argv)
voxieWindowID = instance.Gui.GetMainWindowID()
# print (voxieWindowID)


def setTransientParent(widget, voxieWindowID):
    voxieWindow = None
    if voxieWindowID is not None:
        voxieWindow = PyQt5.QtGui.QWindow.fromWinId(voxieWindowID)
    widget.winId()  # Trigger creation of native window
    window = None
    if 'windowHandle' in dir(widget):  # Available starting with v5.3.2
        window = widget.windowHandle()
    if voxieWindow is not None and window is not None:
        window.setTransientParent(voxieWindow)


dialog = PyQt5.QtWidgets.QMessageBox()
dialog.setWindowTitle('Plane Z position')
dialog.setText('Plane Z position is:\n' + repr(originPlane[2]))
setTransientParent(dialog, voxieWindowID)
ok = dialog.exec()
