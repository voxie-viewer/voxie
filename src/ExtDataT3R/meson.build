name = 'ExtDataT3R'

qt_modules = [
  'Core',
  'DBus',
  'Gui',
  'Xml',
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

moc_files = qt5.preprocess(
  moc_sources : [
  ],
  moc_headers : [
    #'EventListBufferGroup.hpp',
    #'EventListCacheCommon.hpp',
    #'EventListCacheReader.hpp',
    #'EventListCacheWriter.hpp',
    'EventListProvider.hpp',
    #'EventListReader.hpp',
    #'EventListTypes.hpp',
    'T3RLazyLoader.hpp',
    #'T3RLookupTables.hpp',
    #'TimepixRawMetadata.hpp',
  ],
  dependencies : [ ext_dependencies, qt5_dep_moc ],
)

main = executable(
  name,
  [
    moc_files,

    'EventListCacheReader.cpp',
    'EventListCacheWriter.cpp',
    'EventListProvider.cpp',
    'EventListReader.cpp',
    'ExtDataT3R.cpp',
    'T3RLazyLoader.cpp',
    'T3RLookupTables.cpp',
    'TimepixRawMetadata.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [ ext_dependencies, qt5_dep ],
  install : true,
  install_rpath : ext_install_rpath,
  install_dir : ext_install_dir,
)

configure_file(
  input : name + '.json',
  output : name + '.json',
  copy : true,
)
install_data(name + '.json', install_dir : ext_install_dir)
