Data sources
============

The supported data source types are for [vxvol](voxie:///help/topic/file-formats/vxvol) and [vxtraw](voxie:///help/topic/file-formats/vxtraw) files are
`DenseBinaryFile`, `ImageArchive` and `ZippedImages`.

Data source type `DenseBinaryFile`
----------------------------------

For the data source type `DenseBinaryFile`, the data is stored as binary in a
file, without any padding. It is possible to skip over a header before the data.

The following keys are in `DataSource`:
- `"DataFilename"`: A string containing the filename of the data file.
- `"Offset"`: An integer which points to the first byte in the file which contains data. Defaults to 0.
- `"StorageOrder"`: An array with 3 integer values (for X, Y and Z) indicating how each dimension of the data is stored in the file. The default is `[1, 2, 3]`.The meaning of the values is:
  - 1: The dimension is stored as with the smallest stride.
  - -1: The dimension is stored as with the smallest stride (mirrored).
  - 2: The dimension is stored as with the middle stride.
  - -2: The dimension is stored as with the middle stride (mirrored).
  - 3: The dimension is stored with the largest stride.
  - -3: The dimension is stored with the largest stride (mirrored).
- `"ValueOffset"`, `"ValueScalingFactor"`: The value which is loaded will be `ValueOffset + ValueScalingFactor * x` where `x` is the value in the file. If either `"ValueOffset"` or `"ValueScalingFactor"` is set, the volume will be loaded as a floating point volume.


Data source type `ImageArchive`
-------------------------------

For the data source type `ImageArchive`, the data is stored as binary in a
file. In addition, the is an image archive info file (`.iai`) which contains
information where in the file each image is stored.

The following keys are in `DataSource`:
- `"DataFilename"`: A string containing the filename of the data file. The image archive info file has the same name with `.iai` appended.
- `"StorageOrder"`: An array with 3 integer values (for X, Y and Z) indicating how each dimension of the data is stored in the file. The default is `[1, 2, 3]`. The meaning of the values is:
  - 1: The dimension is stored as the X dimension of the image
  - -1: The dimension is stored as the X dimension of the image (mirrored)
  - 2: The dimension is stored as the Y dimension of the image
  - -2: The dimension is stored as the Y dimension of the image (mirrored)
  - 3: The dimension is stored as the position in the list of images
  - -3: The dimension is stored as the position in the list of images (mirrored)


Data source type `ZippedImages`
-------------------------------

For the data source type `ZippedImages`, the data is stored in a zip file
containing images.

The following keys are in `DataSource`:
- `"DataFilename"`: A string containing the filename of the zip file.
- `"FirstImageIndex"`: The index of the first image.
- `"ImageFilePattern"`: A [python format string] which will be passed one integer value (the position in the list of images plus the `"FirstImageIndex`" value).
- `"ImageFileNames"`: An object mapping integral values to strings (filenames). These names have higher priority than the names produced by `"ImageFilePattern"` if both are given.
- `"StorageOrder"`: An array with 3 integer values (for X, Y and Z) indicating how each dimension of the data is stored in the file. The default is `[1, 2, 3]`. The meaning of the values is:
  - 1: The dimension is stored as the X dimension of the images
  - -1: The dimension is stored as the X dimension of the images (mirrored)
  - 2: The dimension is stored as the Y dimension of the images
  - -2: The dimension is stored as the Y dimension of the images (mirrored)
  - 3: The dimension is stored as the position in the list of images
  - -3: The dimension is stored as the position in the list of images (mirrored)

[python format string]: <https://docs.python.org/3/library/string.html#format-string-syntax>


Data source type `ImageStack`
-----------------------------

For the data source type `ImageStack`, the data is stored in a list of images
files.

This data source type is similar to `ZippedImages`, except that there is no
`"DataFilename"` key and the file pattern instead provides file names relative
to the file itself.
