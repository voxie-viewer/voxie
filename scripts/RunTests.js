/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

function assertEqual (x, y) {
    if (x !== y) {
        throw Error("Assertion failed: " + x + " !== " + y);
    }
}

describe(voxie)
describe(voxie.Gui)

voxie.Gui.RaiseWindow()

var examplePlugin = voxie.GetPluginByName('Example')
var examplePlugin2 = voxie.GetPluginByName('Example')
assertEqual (examplePlugin, examplePlugin2)
var sphereImporter = examplePlugin.GetComponent('de.uni_stuttgart.Voxie.Importer', 'TheSphereGenerator')

var hdfPlugin = voxie.GetPluginByName('HDF5')
var hdfImporter = hdfPlugin.GetComponent('de.uni_stuttgart.Voxie.Importer', 'HDFImporter')
var importers = hdfPlugin.ListComponents('de.uni_stuttgart.Voxie.Importer')
assertEqual (importers.length, 1)
assertEqual (hdfImporter, importers[0])
assertEqual (hdfImporter.Filter.Description, 'HDF5 Files')
assertEqual (hdfImporter.Filter.Patterns.length, 2)
assertEqual (hdfImporter.Filter.Patterns[0], '*.h5')
assertEqual (hdfImporter.Filter.Patterns[1], '*.hdf5')

var slicePlugin = voxie.GetPluginByName('VisSlice')
var visualizerPrototype = slicePlugin.GetComponent('de.uni_stuttgart.Voxie.VisualizerPrototype', 'SliceMetaVisualizer')

assertEqual (slicePlugin.ListComponents, examplePlugin.ListComponents)

var dataSet = sphereImporter.GenerateSphere(50)
var slice = dataSet.CreateSlice()

var visualizer = visualizerPrototype.Create ([], [slice])
assertEqual (visualizer, voxie.Gui.ActiveVisualizer)

print ('Success')
