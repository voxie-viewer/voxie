qt_modules = [
  'Core',
  'DBus',
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

if not is_variable('additional_voxieclient_moc_sources')
   additional_voxieclient_moc_sources = []
endif
if not is_variable('additional_voxieclient_moc_headers')
   additional_voxieclient_moc_headers = []
endif
moc_files = qt5.preprocess(
  moc_sources : [
    'ObjectExport/BusManager.cpp',
    'ObjectExport/ClientManager.cpp',
  ] + additional_voxieclient_moc_sources,
  moc_headers : [
    #'Array.hpp',
    #'ArrayInfo.hpp',
    #'Bool8.hpp',
    'ClaimedOperation.hpp',
    #'DataTypeExt.hpp',
    'DBusAdaptors.hpp',
    #'DBusAsJSON.hpp',
    #'DBusClient.hpp',
    #'DBusConnectionUtil.hpp',
    'DBusProxies.hpp',
    #'DBusTypes.hpp',
    #'DBusTypeList.hpp',
    #'DBusUtil.hpp',
    #'DebugOption.hpp',
    #'Exception.hpp',
    #'Exceptions.hpp',
    #'HmgVector.hpp',
    #'JsonDBus.hpp',
    #'JsonUtil.hpp',
    #'MappedBuffer.hpp',
    #'Map.hpp',
    #'Matrix.hpp',
    #'Quaternion.hpp',
    #'QDBusPendingReplyWrapper.hpp',
    #'QtUtil.hpp',
    #'RefCountHolder.hpp',
    #'Rotation.hpp',
    #'SharedFunPtr.hpp',
    #'TypeTraits.hpp',
    #'Vector.hpp',
    #'VoxieClient.hpp',
    #'VoxieDBus.hpp',
    'ObjectExport/BusManager.hpp',
    #'ObjectExport/DBusCallUtil.hpp',
    'ObjectExport/Client.hpp',
    'ObjectExport/ClientManager.hpp',
    'ObjectExport/ExportedObject.hpp',
  ] + additional_voxieclient_moc_headers,
  # moc_extra_arguments : ['-DMAKES_MY_MOC_HEADER_COMPILE'],
  include_directories : [ project_incdir ],
  dependencies : qt5_dep_moc,
)

if not is_variable('additional_voxieclient_sources')
   additional_voxieclient_sources = []
endif
voxieclient = shared_library(
  'VoxieClient',
  [
    moc_files,

    'Array.cpp',
    'ArrayInfo.cpp',
    'Bool8.cpp',
    'ClaimedOperation.cpp',
    'DataTypeExt.cpp',
    'DBusAsJSON.cpp',
    'DBusClient.cpp',
    'DBusConnectionUtil.cpp',
    'DBusProxies.cpp',
    'DBusTypes.cpp',
    'DBusTypeList.cpp',
    'DBusUtil.cpp',
    'DebugOption.cpp',
    'Exception.cpp',
    'Exceptions.cpp',
    'HmgVector.cpp',
    'JsonDBus.cpp',
    'JsonUtil.cpp',
    'Map.cpp',
    'MappedBuffer.cpp',
    'Matrix.cpp',
    'Quaternion.cpp',
    'QDBusPendingReplyWrapper.cpp',
    'QtUtil.cpp',
    'RefCountHolder.cpp',
    'Rotation.cpp',
    'SharedFunPtr.cpp',
    'TypeTraits.cpp',
    'Vector.cpp',
    'VoxieDBus.cpp',
    'ObjectExport/BusManager.cpp',
    'ObjectExport/DBusCallUtil.cpp',
    'ObjectExport/Client.cpp',
    'ObjectExport/ClientManager.cpp',
    'ObjectExport/ExportedObject.cpp'
  ] + additional_voxieclient_sources,
  include_directories : [ project_incdir ],
  implicit_include_directories : false,
  dependencies : [
    qt5_dep,
    half_dep,
  ],
  cpp_args : [ '-DVOXIECLIENT_LIBRARY' ],
  install : true,
  install_dir : lib_install_dir,
)

voxieclient_dep = declare_dependency(
  include_directories : [
    project_incdir,
  ],
  link_with : [
    voxieclient,
  ],
  dependencies : [
    half_dep,
  ],
)

# TODO: Install headers?
