#!/usr/bin/python3
#
# Copyright (c) 2014-2022 The Voxie Authors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# Stored using voxie version 'Git revision 6f78281cc3a2951dcc7a3124d4341a78c5bcfc14 built with QT 5.15.2'
import voxie, dbus
instance = voxie.instanceFromArgs()


### de.uni_stuttgart.Voxie.NodeKind.Data ###
# de.uni_stuttgart.Voxie.Data.ContainerNode #
o_ContainerNode_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Data.ContainerNode', {
})
o_ContainerNode_1.ManualDisplayName = (False, '')
o_ContainerNode_1.GraphPosition = (0.0, 100.0)
# de.uni_stuttgart.Voxie.Data.Volume #
o_Volume_1 = instance.OpenFileChecked('/home/so/enpro-2021-voxie/reconstructed_volumes/copperWire/CopperWire-Downscaled8.hdf5') # 'de.uni_stuttgart.Voxie.Data.Volume'
o_Volume_1.SetPropertiesChecked({
  'de.uni_stuttgart.Voxie.MovableDataNode.Rotation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.MovableDataNode.Translation': voxie.Variant('(ddd)', (0.0, 0.0, 0.0)),
})
o_Volume_1.ManualDisplayName = (False, '')
o_Volume_1.GraphPosition = (-21.25, -30.0)

### de.uni_stuttgart.Voxie.NodeKind.SegmentationStep ###
# de.uni_stuttgart.Voxie.SegmentationStep.AssignmentStep #
o_AssignmentStep_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.AssignmentStep', {
  'de.uni_stuttgart.Voxie.SegmentationStep.AssignmentStep.LabelID': voxie.Variant('x', 1),
})
o_AssignmentStep_1.ManualDisplayName = (False, '')
o_AssignmentStep_1.GraphPosition = (0.0, 0.0)
o_AssignmentStep_2 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.AssignmentStep', {
  'de.uni_stuttgart.Voxie.SegmentationStep.AssignmentStep.LabelID': voxie.Variant('x', 2),
})
o_AssignmentStep_2.ManualDisplayName = (False, '')
o_AssignmentStep_2.GraphPosition = (0.0, 0.0)
# de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep #
o_BrushSelectionStep_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep', {
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.BrushEraseCentersWithRadius': voxie.Variant('a((ddd)d)', []),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.BrushSelectCentersWithRadius': voxie.Variant('a((ddd)d)', [((0.00015540291497018188, 8.673617379884035e-19, -0.0027751163579523563), 0.00023999999393709004)]),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.PlaneOrientation': voxie.Variant('(dddd)', (0.7071067690849304, 0.7071067690849304, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.PlaneOrigin': voxie.Variant('(ddd)', (-1.2597069144248962e-05, 0.0, -1.511676236987114e-05)),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.VolumeOrientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.VolumeOrigin': voxie.Variant('(ddd)', (-0.003217293182387948, -0.00427292799577117, -0.003925248980522156)),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.VoxelSize': voxie.Variant('(ddd)', (4.031064236187376e-05, 4.031064236187376e-05, 4.031064236187376e-05)),
})
o_BrushSelectionStep_1.ManualDisplayName = (False, '')
o_BrushSelectionStep_1.GraphPosition = (0.0, 0.0)
o_BrushSelectionStep_2 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep', {
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.BrushEraseCentersWithRadius': voxie.Variant('a((ddd)d)', []),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.BrushSelectCentersWithRadius': voxie.Variant('a((ddd)d)', [((-0.0027485969476401806, -1.3877787807814457e-17, -0.002727116458117962), 0.00023999999393709004)]),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.PlaneOrientation': voxie.Variant('(dddd)', (0.7071067690849304, 0.7071067690849304, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.PlaneOrigin': voxie.Variant('(ddd)', (-1.2597069144248962e-05, 0.0, -1.511676236987114e-05)),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.VolumeOrientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.VolumeOrigin': voxie.Variant('(ddd)', (-0.003217293182387948, -0.00427292799577117, -0.003925248980522156)),
  'de.uni_stuttgart.Voxie.SegmentationStep.BrushSelectionStep.VoxelSize': voxie.Variant('(ddd)', (4.031064236187376e-05, 4.031064236187376e-05, 4.031064236187376e-05)),
})
o_BrushSelectionStep_2.ManualDisplayName = (False, '')
o_BrushSelectionStep_2.GraphPosition = (0.0, 0.0)
# de.uni_stuttgart.Voxie.SegmentationStep.ExtSegmentationStepWatershed #
o_ExtSegmentationStepWatershed_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.ExtSegmentationStepWatershed', {
  'de.uni_stuttgart.Voxie.SegmentationStep.ExtSegmentationStepWatershed.ExecutionKind': voxie.Variant('s', 'de.uni_stuttgart.Voxie.SegmentationStep.ExtSegmentationStepWatershed.ExecutionKind.Sequential'),
  'de.uni_stuttgart.Voxie.SegmentationStep.ExtSegmentationStepWatershed.Seeds': voxie.Variant('at', [1, 2]),
  'de.uni_stuttgart.Voxie.SegmentationStep.ExtSegmentationStepWatershed.Sigma': voxie.Variant('x', 5),
  'de.uni_stuttgart.Voxie.SegmentationStep.ExtSegmentationStepWatershed.Volume': voxie.Variant('o', None),
})
o_ExtSegmentationStepWatershed_1.ManualDisplayName = (False, '')
o_ExtSegmentationStepWatershed_1.GraphPosition = (0.0, 0.0)
# de.uni_stuttgart.Voxie.SegmentationStep.MetaStep #
o_MetaStep_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.MetaStep', {
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Color': voxie.Variant('(dddd)', (0.0, 0.6, 0.2, 1.0)),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Description': voxie.Variant('s', ''),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.LabelID': voxie.Variant('x', 1),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.ModificationKind': voxie.Variant('s', 'AddLabel'),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Name': voxie.Variant('s', 'Label #1'),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Visibility': voxie.Variant('b', True),
})
o_MetaStep_1.ManualDisplayName = (False, '')
o_MetaStep_1.GraphPosition = (0.0, 0.0)
o_MetaStep_2 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.MetaStep', {
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Color': voxie.Variant('(dddd)', (0.0, 0.4, 0.6, 1.0)),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Description': voxie.Variant('s', ''),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.LabelID': voxie.Variant('x', 2),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.ModificationKind': voxie.Variant('s', 'AddLabel'),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Name': voxie.Variant('s', 'Label #2'),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Visibility': voxie.Variant('b', True),
})
o_MetaStep_2.ManualDisplayName = (False, '')
o_MetaStep_2.GraphPosition = (0.0, 0.0)
o_MetaStep_3 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.MetaStep', {
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Color': voxie.Variant('(dddd)', (0.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Description': voxie.Variant('s', ''),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.LabelID': voxie.Variant('x', 2),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.ModificationKind': voxie.Variant('s', 'Name'),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Name': voxie.Variant('s', 'Air'),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Visibility': voxie.Variant('b', False),
})
o_MetaStep_3.ManualDisplayName = (False, '')
o_MetaStep_3.GraphPosition = (0.0, 0.0)
o_MetaStep_4 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.SegmentationStep.MetaStep', {
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Color': voxie.Variant('(dddd)', (0.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Description': voxie.Variant('s', ''),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.LabelID': voxie.Variant('x', 1),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.ModificationKind': voxie.Variant('s', 'Name'),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Name': voxie.Variant('s', 'Copper'),
  'de.uni_stuttgart.Voxie.SegmentationStep.MetaStep.Visibility': voxie.Variant('b', False),
})
o_MetaStep_4.ManualDisplayName = (False, '')
o_MetaStep_4.GraphPosition = (0.0, 0.0)

### de.uni_stuttgart.Voxie.NodeKind.Filter ###
# de.uni_stuttgart.Voxie.Filter.Segmentation #
o_Segmentation_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Filter.Segmentation', {
  'de.uni_stuttgart.Voxie.Filter.Segmentation.Input': voxie.Variant('o', o_Volume_1),
  'de.uni_stuttgart.Voxie.Filter.Segmentation.Output': voxie.Variant('o', o_ContainerNode_1),
  'de.uni_stuttgart.Voxie.Filter.Segmentation.StepList': voxie.Variant('ao', [o_BrushSelectionStep_1, o_MetaStep_1, o_AssignmentStep_1, o_BrushSelectionStep_2, o_MetaStep_2, o_AssignmentStep_2, o_MetaStep_3, o_MetaStep_4, o_ExtSegmentationStepWatershed_1]),
})
o_Segmentation_1.ManualDisplayName = (False, '')
o_Segmentation_1.GraphPosition = (0.0, 50.0)

### de.uni_stuttgart.Voxie.NodeKind.Visualizer ###
# de.uni_stuttgart.Voxie.Visualizer.Slice #
o_Slice_1 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Visualizer.Slice', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.009),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 1.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 1.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.InfoTable': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Interpolation': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Interpolation.Linear'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.LabelContainer': voxie.Variant('o', o_ContainerNode_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin': voxie.Variant('(ddd)', (-1.2597069144248962e-05, 0.0, -1.511676236987114e-05)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.SegmentationFilter': voxie.Variant('o', o_Segmentation_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Show2DFilterMask': voxie.Variant('x', 0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping': voxie.Variant('a(d(dddd)i)', [(-291.03350830078125, (0.0, 0.0, 0.0, 1.0), 0), (710.4412841796875, (1.0, 1.0, 1.0, 1.0), 0)]),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume': voxie.Variant('o', o_Volume_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridShow': voxie.Variant('b', False),
})
o_Slice_1.ManualDisplayName = (True, 'SegmentationViewSliceXY')
o_Slice_1.GraphPosition = (0.0, 150.0)
o_Slice_1.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').IsAttached = True
o_Slice_1.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').VisualizerPosition = (719.0, 0.0)
o_Slice_1.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').VisualizerSize = (719.0, 486.0)
o_Slice_2 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Visualizer.Slice', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.009),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 1.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 1.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.InfoTable': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Interpolation': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Interpolation.Linear'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.LabelContainer': voxie.Variant('o', o_ContainerNode_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation': voxie.Variant('(dddd)', (0.7071067690849304, 0.7071067690849304, 0.0, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin': voxie.Variant('(ddd)', (-1.2597069144248962e-05, 0.0, -1.511676236987114e-05)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.SegmentationFilter': voxie.Variant('o', o_Segmentation_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Show2DFilterMask': voxie.Variant('x', 0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping': voxie.Variant('a(d(dddd)i)', [(-291.03350830078125, (0.0, 0.0, 0.0, 1.0), 0), (710.4412841796875, (1.0, 1.0, 1.0, 1.0), 0)]),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume': voxie.Variant('o', o_Volume_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridShow': voxie.Variant('b', False),
})
o_Slice_2.ManualDisplayName = (True, 'SegmentationViewSliceXZ')
o_Slice_2.GraphPosition = (0.0, 200.0)
o_Slice_2.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').IsAttached = True
o_Slice_2.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').VisualizerPosition = (0.0, 486.0)
o_Slice_2.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').VisualizerSize = (719.0, 486.0)
o_Slice_3 = instance.CreateNodeChecked('de.uni_stuttgart.Voxie.Visualizer.Slice', {
  'de.uni_stuttgart.Voxie.View2D.CenterPoint': voxie.Variant('(dd)', (0.0, 0.0)),
  'de.uni_stuttgart.Voxie.View2D.VerticalSize': voxie.Variant('d', 0.009),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Filter2DConfiguration': voxie.Variant('s', '<?xml version="1.0"?>\n<!DOCTYPE filterchain>\n<filterchain2d version="1.0"/>\n'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitive': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorBehindSlice': voxie.Variant('(dddd)', (0.0, 0.0, 1.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorInFrontOfSlice': voxie.Variant('(dddd)', (1.0, 0.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveColorOnSlice': voxie.Variant('(dddd)', (0.0, 1.0, 0.0, 1.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GeometricPrimitiveVisibilityDistance': voxie.Variant('d', 0.0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.GridSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.InfoTable': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Interpolation': voxie.Variant('s', 'de.uni_stuttgart.Voxie.Interpolation.Linear'),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.LabelContainer': voxie.Variant('o', o_ContainerNode_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane': voxie.Variant('o', None),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Orientation': voxie.Variant('(dddd)', (0.7071067690849304, 0.0, 0.7071067690849304, 0.0)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Plane.Origin': voxie.Variant('(ddd)', (-1.2597069144248962e-05, 0.0, -1.511676236987114e-05)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerShow': voxie.Variant('b', False),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacing': voxie.Variant('d', 0.001),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.RulerSpacingAutomatic': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.SegmentationFilter': voxie.Variant('o', o_Segmentation_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Show2DFilterMask': voxie.Variant('x', 0),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ShowSliceCenter': voxie.Variant('b', True),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Surface': voxie.Variant('ao', []),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.ValueColorMapping': voxie.Variant('a(d(dddd)i)', [(-291.03350830078125, (0.0, 0.0, 0.0, 1.0), 0), (710.4412841796875, (1.0, 1.0, 1.0, 1.0), 0)]),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.Volume': voxie.Variant('o', o_Volume_1),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridColor': voxie.Variant('(dddd)', (1.0, 1.0, 0.0, 0.5)),
  'de.uni_stuttgart.Voxie.Visualizer.Slice.VolumeGridShow': voxie.Variant('b', False),
})
o_Slice_3.ManualDisplayName = (True, 'SegmentationViewSliceYZ')
o_Slice_3.GraphPosition = (0.0, 250.0)
o_Slice_3.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').IsAttached = True
o_Slice_3.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').VisualizerPosition = (719.0, 486.0)
o_Slice_3.CastTo('de.uni_stuttgart.Voxie.VisualizerNode').VisualizerSize = (719.0, 486.0)
instance.Gui.MdiViewMode = 'de.uni_stuttgart.Voxie.MdiViewMode.SubWindow'
