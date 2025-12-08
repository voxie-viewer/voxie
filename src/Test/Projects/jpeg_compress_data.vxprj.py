#!/usr/bin/python3
# Stored using voxie version 'Voxie 0.6.0.git-intern revision f0cd16afe5c049002e35e73646187121a04c6c1c built with QT 5.15.8'
import voxie, dbus, os
instance = voxie.instanceFromArgs()


### de.uni_stuttgart.Voxie.NodeKind.Data ###
# de.uni_stuttgart.Voxie.Data.Volume #
o_Volume_1 = instance.OpenFileChecked(os.path.dirname(__file__) + '/jpeg_voxel_vol_1.vxvol.json') # 'de.uni_stuttgart.Voxie.Data.Volume'
if o_Volume_1 is not None:
    o_Volume_1.SetPropertiesChecked({
  'de.uni_stuttgart.Voxie.MovableDataNode.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataNode.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
if o_Volume_1 is not None:
    o_Volume_1.GraphPosition = (-85.0, 0.0)
o_Volume_2 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataNode.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataNode.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
if o_Volume_2 is not None:
    o_Volume_2.GraphPosition = (-85.0, 400.0)
o_Volume_3 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Data.Volume', {
  'de.uni_stuttgart.Voxie.MovableDataNode.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataNode.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
if o_Volume_3 is not None:
    o_Volume_3.GraphPosition = (-85.0, 200.0)

### de.uni_stuttgart.Voxie.NodeKind.Filter ###
# de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg #
o_CompressVolumeJpeg_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg', {
  'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.Quality': voxie.Variant('x', 100),
  'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Lower': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.MinMax'),
  'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Lower.ManualValue': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Upper': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.MinMax'),
  'de.uni_stuttgart.Voxie.Filter.CompressVolumeJpeg.RangeSelection.Upper.ManualValue': voxie.Variant('d', 100.0),
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('o', o_Volume_3),
  'de.uni_stuttgart.Voxie.Output': voxie.Variant('o', o_Volume_2),
})
if o_CompressVolumeJpeg_1 is not None:
    o_CompressVolumeJpeg_1.GraphPosition = (-85.0, 300.0)
# de.uni_stuttgart.Voxie.Filter.ConvertDataType #
o_ConvertDataType_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Filter.ConvertDataType', {
  'de.uni_stuttgart.Voxie.Filter.ConvertDataType.DataType': voxie.Variant('(sus)', ('float', 32, 'native')),
  'de.uni_stuttgart.Voxie.Input': voxie.Variant('o', o_Volume_1),
  'de.uni_stuttgart.Voxie.Output': voxie.Variant('o', o_Volume_3),
})
if o_ConvertDataType_1 is not None:
    o_ConvertDataType_1.GraphPosition = (-85.0, 100.0)

### Node Group Relations ###
### GUI ###
instance.Gui.MdiViewMode = 'de.uni_stuttgart.Voxie.MdiViewMode.SubWindow'
