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
- `"DataProperties"` (optional): Data properties in JSON format, see [data properties documentation](voxie:///help/topic/file-formats/data-properties)

For the supported data source types see the [data source documentation](voxie:///help/topic/file-formats/data-sources).
