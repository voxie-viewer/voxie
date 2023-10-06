`.vxtraw.json` files
====================

`.vxtraw.json` files contain metadata about a tomography raw data set. They are JSON files with the following keys:

- `"Type"`: Always the String `"de.uni_stuttgart.Voxie.FileFormat.TomographyRawData.VxTRaw"`
- `"ImageKind"`: A JSON object describing the image kind of the images.
- `"Streams"`: A list of JSON objects representing streams.
- `"Metadata"`: Metadata for the raw object
- `"Geometries"`: A list of objects where each object contains two entries: `"Name"`, a string with the name of the geometry, and `"Data"` which contains the geometry-specific data.


Stream objects
--------------

Stream objects contain the following keys:

- `"Name"`: A string with the name of the stream.
- `"ImageCount"`: The number of images in the stream.
- `"ImageShape"`: An array with 2 values, giving the number of pixels in each image in in X and Y (optional, if absent different images can have different sizes)
- `"GridSpacing"`: An array with 2 values, giving the pixel size (im m) in X and Y direction (optional)
- `"ImageOrigin"`: An array with 2 values, giving the position of the lower left corner of the first pixel (im m) in X and Y (optional)
- `"DataSourceType"`: A string indicating how the data is stored
- `"DataSource"`: Additional data depeneding on `DataSourceType`
- `"PerImageMetadata"`: An array with `ImageCount` entries. Each entry contains the per-image metadata for one image.

For the supported data source types see the [data source documentation](voxie:///help/topic/file-formats/data-sources).
Currently the last value of the `"StorageOrder"` (if set) must always be 3.
