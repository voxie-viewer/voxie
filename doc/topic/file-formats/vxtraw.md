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

Currently the only data source type is `ImageArchive`


Data source type `ImageArchive`
-------------------------------

For the data source type `ImageArchive`, the data is stored as binary in a
file. In addition, the is an image archive info file (`.iai`) which contains
information where in the file each image is stored.

The following keys are in `DataSourceType`:
- `"DataFilename"`: A string containing the filename of the data file. The image archive info file has the same name with `.iai` appended.
