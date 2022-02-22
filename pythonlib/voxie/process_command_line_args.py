import voxie
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
import functools
import dbus
import math
import subprocess
import os
import json
import voxie.dbus_as_json

knownOptions = {'iso', 'slice', 'raw'}
knownArgOptions = {'import-property'}


def process(instance, cwd, files, options, argOptions):
    options = set(options)
    # print(options)
    # print(files)
    unknownOptions = options.difference(knownOptions)
    unknownArgOptions = set(argOptions).difference(knownArgOptions)
    if len(unknownOptions) != 0:
        raise Exception('Unknown options: ' + repr(unknownOptions))
    if len(unknownArgOptions) != 0:
        raise Exception('Unknown argument options: ' + repr(unknownArgOptions))

    for fileName in files:
        fileName = os.path.join(cwd, fileName)
        if fileName.endswith('.vxprj.py'):
            # TODO: This doesn't work under Windows because PYTHONPATH is ignored
            subprocess.run([sys.executable, fileName] +
                           sys.argv[1:], check=True)
        else:
            print('Opening %s...' % (repr(fileName),))
            if 'import-property' in argOptions:
                # TODO: Do finding the importer in a proper way
                import fnmatch
                importer = None
                bn = os.path.basename(fileName)
                for imp0 in instance.Components.ListComponents('de.uni_stuttgart.Voxie.ComponentType.Importer'):
                    imp = imp0.CastTo('de.uni_stuttgart.Voxie.Importer')
                    found = False
                    for pat in imp.Filter['Patterns'].getValue('as'):
                        if fnmatch.fnmatch(bn, pat):
                            found = True
                            break
                    if found:
                        importer = imp
                        break
                if importer is None:
                    raise Exception('Could not find importer for ' + repr(fileName))
                props = {}
                for prop in importer.ListProperties():
                    props[prop.Name] = prop
                propVals = {}
                for arg in argOptions['import-property']:
                    pos = arg.find('=')
                    if pos < 0:
                        raise Exception('--import-property argument does not contain a "="')
                    key = arg[:pos]
                    valJson = arg[pos + 1:]
                    if key not in props:
                        print('Warning: Could not find importer property {0} in importer {1}'.format(repr(key), repr(importer.Name)))
                        continue
                    sig = props[key].Type.DBusSignature
                    # TODO: Use VariantStyle.JsonVariant?
                    val = voxie.dbus_as_json.decode_dbus_as_json(sig, json.loads(valJson))
                    propVals[key] = voxie.Variant(sig, val)
                obj = importer.ImportObject(fileName, {'Properties': voxie.Variant('a{sv}', propVals)})
            else:
                obj = instance.OpenFile(fileName)
            interfaces = obj.Data.SupportedInterfaces
            if 'iso' in options and 'de.uni_stuttgart.Voxie.VolumeData' in interfaces:
                threshold = 10  # TODO
                instance.GetPrototype('de.uni_stuttgart.Voxie.Filter.CreateSurface').CreateObject({
                    'de.uni_stuttgart.Voxie.Input': voxie.Variant('ao', [obj]),
                    'de.uni_stuttgart.Voxie.Filter.CreateSurface.Threshold': voxie.Variant('d', threshold),
                })
            if 'slice' in options and 'de.uni_stuttgart.Voxie.VolumeData' in interfaces:
                instance.GetPrototype('de.uni_stuttgart.Voxie.Visualizer.Slice').CreateObject({
                    'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume': voxie.Variant('o', obj),
                })
            if 'raw' in options and 'de.uni_stuttgart.Voxie.TomographyRawDataBase' in interfaces:
                instance.GetPrototype('de.uni_stuttgart.Voxie.Visualizer.TomographyRawData').CreateObject({
                    'de.uni_stuttgart.Voxie.Visualizer.TomographyRawData.RawData': voxie.Variant('o', obj),
                })
