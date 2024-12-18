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
    'PluginJpegLibjpeg.hpp',
  ],
  dependencies : [ plugin_dependencies, qt5_dep_moc ],
)

cpp_files = [
    moc_files,

    'BlockEncoderImpl.cpp',
    'BlockDecoderImpl.cpp',
    'BlockJpegImplementationLibjpeg.cpp',
    'Common.cpp',
    'Libjpeg.cpp',
    'PluginJpegLibjpeg.cpp',
]

if get_option('libjpeg').enabled()
  plugin_8 = shared_library(
    'VoxiePluginJpegLibjpeg',
    cpp_files,
    implicit_include_directories : false,
    cpp_args : [
      '-DVX_LIBJPEG_BITS=8',
    ],
    dependencies : [
      plugin_dependencies,
      qt5_dep,
      jpeg_dep,
    ],
    install : true,
    install_dir : plugin_install_dir,
  )
endif

# Build libjpeg-turbo using:
# cmake -DWITH_12BIT=1 -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_INSTALL_PREFIX=/some/path .. && make -j16 && make install

# TODO: Drop support for libjpeg<3 (libjpeg3 does not require two different libraries for 8bit and 12bit)?

if get_option('libjpeg12').enabled()
  plugin_12 = shared_library(
    'VoxiePluginJpegLibjpeg12',
    cpp_files,
    implicit_include_directories : false,
    cpp_args : [
      '-DVX_LIBJPEG_BITS=12',
    ],
    dependencies : [
      plugin_dependencies,
      qt5_dep,
      jpeg12_dep,
    ],
    install : true,
    install_dir : plugin_install_dir,
  )
endif
