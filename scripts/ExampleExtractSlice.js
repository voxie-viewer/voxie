var size = [20, 5];

var client = voxie.CreateClient ()
try {
    //print (client)
    var dataSet = voxie.Gui.ActiveVisualizer.DataSet
    var image = voxie.CreateImage (client, size)
    dataSet.FilteredData.ExtractSlice ([0, 0, 0], [1, 0, 0, 0], size, [0.1, 0.1], image, { Interpolation: 'NearestNeighbor' })
    var str = '\n';
    for (var y = 0; y < image.Height; y++) {
        for (var x = 0; x < image.Width; x++) {
            str += image.GetPixel (x, y)
            str += ' '
        }
        str += '\n'
    }
    print (str)
} finally {
    voxie.DestroyClient (client)
}
