qt_modules = [
  'Core',
  'DBus',
  'Gui',
  'Widgets',
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

moc_files = qt5.preprocess(
  ui_files : [
  ],
  moc_sources : [
  ],
  moc_headers : [
    #'CcaResultSelection.hpp',
    #'ColorizeLabeledSurface.hpp',
    'ColorizeLabeledSurfaceOperation.hpp',
    'ColorizeSurfaceFromAttribute.hpp',
    'ColorizeSurfaceFromAttributeOperation.hpp',
    'PluginFilter.hpp',
    #'Prototypes.forward.hpp',
    'Prototypes.hpp',

    #'CreateSurface/CreateSurface.hpp',
    'CreateSurface/Cuberille.hpp',
    'CreateSurface/IsosurfaceExtractionOperation.hpp',
    'CreateSurface/MarchingCubes.hpp',
    'CreateSurface/SurfaceExtractor.hpp',
  ],
  dependencies : [ plugin_dependencies, qt5_dep_moc ],
)

plugin = shared_library(
  'VoxiePluginFilter',
  [
    moc_files,

    'CcaResultSelection.cpp',
    'ColorizeLabeledSurface.cpp',
    'ColorizeLabeledSurfaceOperation.cpp',
    'ColorizeSurfaceFromAttribute.cpp',
    'ColorizeSurfaceFromAttributeOperation.cpp',
    'PluginFilter.cpp',
    'Prototypes.cpp',

    'CreateSurface/CreateSurface.cpp',
    'CreateSurface/Cuberille.cpp',
    'CreateSurface/IsosurfaceExtractionOperation.cpp',
    'CreateSurface/MarchingCubes.cpp',
    'CreateSurface/SurfaceExtractor.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [
    plugin_dependencies,
    qt5_dep,
  ],
  install : true,
  install_dir : plugin_install_dir,
  cpp_args : [
    '-DIN_PLUGINFILTERS',
  ],
)
plugin_filter = plugin
