var plugin = voxie.GetPluginByName('ExamplePlugin')
var importer = plugin.GetMemberByName('de.uni_stuttgart.Voxie.Importer', 'TheSphereGenerator')

var visPlugin = voxie.GetPluginByName('SliceView')
var visualizerFactory = visPlugin.GetMemberByName('de.uni_stuttgart.Voxie.VisualizerFactory', 'SliceMetaVisualizer')

var dataSet = importer.GenerateSphere(50)
var slice = dataSet.CreateSlice()
var visualizer = visualizerFactory.Create ([], [slice])

// Make visible for other scripts
sphereVisualizer = visualizer
