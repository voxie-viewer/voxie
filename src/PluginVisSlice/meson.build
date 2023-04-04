qt_modules = [
  'Core',
  'DBus',
  'Gui',
  'Widgets',
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

if not is_variable('additional_pluginvisslice_moc_sources')
   additional_pluginvisslice_moc_sources = []
endif
if not is_variable('additional_pluginvisslice_moc_headers')
   additional_pluginvisslice_moc_headers = []
endif
moc_files = qt5.preprocess(
  ui_files : [
    'PointProperties.ui',
  ],
  moc_sources : [
  ] + additional_pluginvisslice_moc_sources,
  moc_headers : [
    'ColorizerWorker.hpp',
    'DefaultTool.hpp',
    'GeometricPrimitiveLayer.hpp',
    'BrushSelectionTool.hpp',
    'LassoSelectionTool.hpp', 
    #'ToolUtils.hpp',
    'Grid.hpp',
    #'GridSizeMode.hpp',
    'ImageGeneratorWorker.hpp',
    'ImageLayer.hpp',
    'LabelLayer.hpp',
    'SelectionLayer.hpp',
    'ImagePaintWidget.hpp',
    'InfoWidget.hpp',
    'Layer.hpp',
    #'MakeHandButton.hpp',
    'PointProperties.hpp',
    #'Prototypes.forward.hpp',
    'Prototypes.hpp',
    'SliceCenterLayer.hpp',
    'Ruler.hpp',
    #'SizeUnit.hpp',
    'SliceVisualizer.hpp',
    'SurfaceVisualizerTool.hpp',
    'ToolSelection.hpp',
    'View3DPropertiesConnection.hpp',
    'ViewCenterLayer.hpp',
    'VisSlicePlugin.hpp',
    'Visualizer2DTool.hpp',
    'VolumeGrid.hpp',
  ] + additional_pluginvisslice_moc_headers,
  dependencies : [ plugin_dependencies, qt5_dep_moc ],
)

if not is_variable('additional_pluginvisslice_sources')
   additional_pluginvisslice_sources = []
endif
plugin = shared_library(
  'VoxiePluginVisSlice',
  [
    moc_files,

    'ColorizerWorker.cpp',
    'DefaultTool.cpp',
    'GeometricPrimitiveLayer.cpp',
    'BrushSelectionTool.cpp',
    'LassoSelectionTool.cpp', 
    'Grid.cpp',
    'ImageGeneratorWorker.cpp',
    'ImageLayer.cpp',
    'LabelLayer.cpp',
    'SelectionLayer.cpp',
    'ImagePaintWidget.cpp',
    'InfoWidget.cpp',
    'Layer.cpp',
    'PointProperties.cpp',
    'Prototypes.cpp',
    'Ruler.cpp',
    'SliceCenterLayer.cpp',
    'SliceVisualizer.cpp',
    'SurfaceVisualizerTool.cpp',
    'ToolSelection.cpp',
    'View3DPropertiesConnection.cpp',
    'ViewCenterLayer.cpp',
    'VisSlicePlugin.cpp',
    'Visualizer2DTool.cpp',
    'VolumeGrid.cpp',
  ] + additional_pluginvisslice_sources,
  implicit_include_directories : false,
  dependencies : [
    plugin_dependencies,
    qt5_dep,
  ],
  install : true,
  install_dir : plugin_install_dir,
)
