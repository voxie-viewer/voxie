function assertEqual (x, y) {
    if (x !== y) {
        throw Error("Assertion failed: " + x + " !== " + y);
    }
}

describe(voxie)
describe(voxie.Gui)

voxie.Gui.RaiseWindow()

var examplePlugin = voxie.GetPluginByName('ExamplePlugin')
var examplePlugin2 = voxie.GetPluginByName('ExamplePlugin')
assertEqual (examplePlugin, examplePlugin2)
var sphereImporter = examplePlugin.GetMemberByName('de.uni_stuttgart.Voxie.Importer', 'TheSphereGenerator')

var hdfPlugin = voxie.GetPluginByName('HDFIO')
var hdfLoader = hdfPlugin.GetMemberByName('de.uni_stuttgart.Voxie.Loader', 'HDFLoader')
var loaders = hdfPlugin.ListMembers('de.uni_stuttgart.Voxie.Loader')
assertEqual (loaders.length, 1)
assertEqual (hdfLoader, loaders[0])
assertEqual (hdfLoader.Filter.Description, 'HDF5 Files')
assertEqual (hdfLoader.Filter.Patterns.length, 2)
assertEqual (hdfLoader.Filter.Patterns[0], '*.h5')
assertEqual (hdfLoader.Filter.Patterns[1], '*.hdf5')

var slicePlugin = voxie.GetPluginByName('SliceView')
var visualizerFactory = slicePlugin.GetMemberByName('de.uni_stuttgart.Voxie.VisualizerFactory', 'SliceMetaVisualizer')

assertEqual (slicePlugin.ListMembers, examplePlugin.ListMembers)

var dataSet = sphereImporter.GenerateSphere(50)
var slice = dataSet.CreateSlice()

var visualizer = visualizerFactory.Create ([], [slice])
assertEqual (visualizer, voxie.Gui.ActiveVisualizer)

print ('Success')
