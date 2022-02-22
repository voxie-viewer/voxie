qt_modules = [
  'Core',
  'Test',
  'Gui', # Needed for Voxie/MathQt.hpp
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

moc_files = qt5.preprocess(
  moc_sources : [
    'MapTest.cpp',
    'MatrixTest.cpp',
    'RotationTest.cpp', 
    'VectorTest.cpp', 
 ],
  moc_headers : [
  ],
  include_directories : [ project_incdir ],
  dependencies : qt5_dep_moc,
)

main = executable(
  'VoxieClientTest',
  [
    moc_files,

    'Main.cpp',
    'MapTest.cpp',
    'MatrixTest.cpp',
    'RotationTest.cpp',
    'VectorTest.cpp',
  ],
  include_directories : [
    project_incdir,
  ],
  implicit_include_directories : false,
  dependencies : [
    qt5_dep,
    voxieclient_dep,
  ],
  link_with : [
  ],
  link_args : [
  ],
)
