#!/usr/bin/python3

import sys
import os
import numpy
import time
import scipy.misc

import voxie

import PyQt5.QtCore
import PyQt5.QtWidgets

app = PyQt5.QtWidgets.QApplication(sys.argv)

#PyQt5.QtWidgets.QMessageBox.information(None, "foo", "bar")

args = voxie.parser.parse_args()

instance = voxie.Voxie(args)

voxieWindowID = instance.dbus_gui.GetMainWindowID()
#print (voxieWindowID)

dataSet = instance.getDataSet(args)

data = dataSet.getOriginalData()

array = data.getDataReadonly().array

def setTransientParent(widget):
    voxieWindow = None
    if voxieWindowID is not None:
        voxieWindow = PyQt5.QtGui.QWindow.fromWinId(voxieWindowID)
    widget.winId() # Trigger creation of native window
    window = None
    if 'windowHandle' in dir(widget): # Available starting with v5.3.2
        window = widget.windowHandle()
    if voxieWindow is not None and window is not None:
        window.setTransientParent(voxieWindow)

#(destDir, x) = PyQt5.QtWidgets.QFileDialog.getSaveFileName(None, "Directory for image stack")
fileDialog = PyQt5.QtWidgets.QFileDialog()
fileDialog.setAcceptMode(PyQt5.QtWidgets.QFileDialog.AcceptSave)
#fileDialog.setFileMode(PyQt5.QtWidgets.QFileDialog.Directory)
fileDialog.setOption(PyQt5.QtWidgets.QFileDialog.ShowDirsOnly)
setTransientParent(fileDialog)
ok = fileDialog.exec()
if not ok:
    sys.exit (0)

destDir = fileDialog.selectedFiles()[0]
if destDir is None or destDir == '':
    sys.exit (0)

os.mkdir (destDir)

zCount = array.shape[2]

min = 0
max = numpy.max(array)

print ('Max value: %f' % (max))

for z in range (0, zCount):
    ar = array[:,:,z]
    scipy.misc.toimage(ar, cmin = min, cmax = max).save(destDir + '/img-%04d.tiff' % z)
