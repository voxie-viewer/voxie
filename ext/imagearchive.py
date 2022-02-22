import csv
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

typeNameMap = {
    "float": "f",
    "uint": "u",
    "int": "i",
}
endianMap = {
    "none": "|",
    "little": "<",
    "big": ">",
}


class ImageArchive:
    def __init__(self, fn):
        self.filename = fn
        # self.file = open(self.filename)
        infoFile = fn + '.iai'
        # infoFile = fn + '.iai.new'
        with open(infoFile, newline='') as file:
            infodata = list(csv.reader(file))
        infoNames = {}
        for i in range(len(infodata[0])):
            infoNames[infodata[0][i]] = i
        self.count = len(infodata) - 2
        idCol = infoNames['ID']
        if infodata[self.count + 1][idCol] != '':
            self.count += 1
        entries = [None for i in range(self.count)]
        self.complete = False
        for i in range(1, len(infodata)):
            if infodata[i][idCol] == '':
                self.complete = True
                continue
            id = int(infodata[i][idCol])
            if entries[id] is not None:
                raise Exception('entries[id] is not None')
            data = {}
            for name in infoNames:
                data[name] = infodata[i][infoNames[name]]
            entries[id] = data
        self.info = entries
        # print(entries)

    def readImage(self, id):
        info = self.info[id]
        bitsPerPixel = int(info['TypeSize'])
        width = int(info['Width'])
        height = int(info['Height'])
        offset = int(info['Offset'])
        if bitsPerPixel % 8 != 0:
            raise 'bitsPerPixel % 8 != 0'
        bytesPerPixel = bitsPerPixel // 8
        dtype = endianMap[info['TypeEndian']] + typeNameMap[info['TypeName']] + str(bytesPerPixel)
        size = width * height * bytesPerPixel
        # print(size)
        with open(self.filename, 'rb') as file:
            file.seek(offset)
            data = file.read(size)
            if len(data) != size:
                raise Exception('len(data) != size')
        array = np.ndarray((width, height), dtype=dtype, strides=(bytesPerPixel, bytesPerPixel * width), buffer=data, offset=0)
        array = array[:, ::-1]  # Mirror Y axis
        return array
