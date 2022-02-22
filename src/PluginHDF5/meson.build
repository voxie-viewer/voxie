qt_modules = [
  'Core',
  'DBus',
  'Gui',
  'Widgets',
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

pluginhdf5_incdir = include_directories('.')

imagehlp_dep = declare_dependency()
if target_machine.system() == 'windows'
  imagehlp_dep = declare_dependency(link_args : '-limagehlp')
endif

if not is_variable('additional_pluginhdf5_sources')
   additional_pluginhdf5_sources = []
endif
if not is_variable('additional_pluginhdf5_cpp_args')
   additional_pluginhdf5_cpp_args = []
endif

moc_files = qt5.preprocess(
  ui_files : [
  ],
  moc_sources : [
  ],
  moc_headers : [
    'HDF5Plugin.hpp',
    'HDFExporter.hpp',
    'HDFImporter.hpp',
    'HDFSliceExporter.hpp',
  ],
  dependencies : [ plugin_dependencies, qt5_dep_moc ],
)

plugin = shared_library(
  'VoxiePluginHDF5',
  [
    moc_files,

    'HDF5Plugin.cpp',
    'HDFExporter.cpp',
    'HDFImporter.cpp',
    'HDFSliceExporter.cpp',

    'CT/DataFiles.cpp',
    'CT/rawdatafiles.cpp',
    'CT/typefiles.cpp',

    'Core/Assert.cpp',
    'Core/CheckedCast.cpp',
    'Core/Exception.cpp',
    'Core/HasMemberCalled.cpp',
    'Core/Memory.cpp',
    'Core/NumericException.cpp',
    'Core/Reflection.cpp',
    'Core/StaticCache.cpp',
    'Core/Type.cpp',

    'HDF5/AtomicType.cpp',
    'HDF5/AtomicTypes.cpp',
    'HDF5/Attribute.cpp',
    'HDF5/BaseTypes.cpp',
    'HDF5/ComplexConversion.cpp',
    'HDF5/CompoundType.cpp',
    'HDF5/DataSet.cpp',
    'HDF5/DataSpace.cpp',
    'HDF5/DataType.cpp',
    'HDF5/DataTypes.cpp',
    'HDF5/DelayedArray.cpp',
    'HDF5/File.cpp',
    'HDF5/Group.cpp',
    'HDF5/HDF5_Array.cpp',
    'HDF5/HDF5_Exception.cpp',
    'HDF5/HDF5_Type.cpp',
    'HDF5/HDF5_Vector3.cpp',
    'HDF5/IdComponent.cpp',
    'HDF5/Matlab.cpp',
    'HDF5/MatlabDiagMatrix3.cpp',
    'HDF5/MatlabVector2.cpp',
    'HDF5/MatlabVector3.cpp',
    'HDF5/Object.cpp',
    'HDF5/OpaqueType.cpp',
    'HDF5/PropList.cpp',
    'HDF5/PropLists.cpp',
    'HDF5/ReferenceType.cpp',
    'HDF5/Serialization.cpp',
    'HDF5/SerializationKey.cpp',
    'HDF5/StdVectorSerialization.cpp',
    'HDF5/Util.cpp',

    'Math/Abs.cpp',
    'Math/Array.cpp',
    'Math/DiagMatrix3.cpp',
    'Math/Float.cpp',
    'Math/Math.cpp',
    'Math/Vector2.cpp',
    'Math/Vector3.cpp',


    'Core/StrError.c',


    additional_pluginhdf5_sources,
  ],
  include_directories : [
    pluginhdf5_incdir,
  ],
  implicit_include_directories : false,
  dependencies : [
    plugin_dependencies,
    qt5_dep,
    hdf5,
    boost,
    imagehlp_dep,
  ],
  link_args : [
    '-ldl',
  ],
  cpp_args : [
    '-DOMPI_SKIP_MPICXX', # Needed to prevent unnecessary dependency when libhdf5-openmpi is used
    '-DNO_BOOST_FILESYSTEM_PATH', # Avoid dependency on BoostFilesystem
    '-DH5_BUILT_AS_DYNAMIC_LIB',
    additional_pluginhdf5_cpp_args,
  ],
  install : true,
  install_dir : plugin_install_dir,
)
