`.vxvol.json` files
===================

`.vxvol.json` files contain metadata about a volume. They are JSON files with
the following keys:

- `"Type"`: Always the String `"de.uni_stuttgart.Voxie.FileFormat.Volume.VxVol"`
- `"VolumeType"`: Currently always the String `"Voxel"`
- `"DataType"`: The type used to store one voxel as an array with 3 values (base type as a string, number of bits as an integer and endianness as a string), see [data type documentation](voxie:///help/topic/data-types)
- `"ArrayShape"`: An array with 3 values, giving the number of voxels in X, Y and Z direction
- `"GridSpacing"`: An array with 3 values, giving the voxel size (im m) in X, Y and Z direction
- `"VolumeOrigin"`: An array with 3 values, giving the position of the lower left corner of the first voxel (im m) in X, Y and Z direction
- `"DataSourceType"`: A string indicating how the data is stored
- `"DataSource"`: Additional data depeneding on `DataSourceType`

Currently the only data source type is `ImageArchive`


Data source type `ImageArchive`
-------------------------------

For the data source type `ImageArchive`, the data is stored as binary in a
file. In addition, the is an image archive info file (`.iai`) which contains
information where in the file each image is stored.

The following keys are in `DataSourceType`:
- `"DataFilename"`: A string containing the filename of the data file. The image archive info file has the same name with `.iai` appended.
- `"StorageOrder"`: An array with 3 integer values (for X, Y and Z) indicating how each dimension of the data is stored in the file. The meaning of the values is:
  - 1: The dimension is stored as the X dimension of the image
  - -1: The dimension is stored as the X dimension of the image (mirrored)
  - 2: The dimension is stored as the Y dimension of the image
  - -2: The dimension is stored as the Y dimension of the image (mirrored)
  - 3: The dimension is stored as the position in the list of images
  - -3: The dimension is stored as the position in the list of images (mirrored)
