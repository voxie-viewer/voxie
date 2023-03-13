name = 'ExtFileHdf5'

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
  moc_sources : [
  ],
  moc_headers : [
    # 'HDFExporter.hpp',
    # 'HDFImporter.hpp',
  ],
  dependencies : [ ext_dependencies, ext_qt5_dep_gui_moc ],
)

main = executable(
  name,
  [
    moc_files,

    'ExtFileHdf5.cpp',

    'HDFExporter.cpp',
    'HDFImporter.cpp',

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
    ext_dependencies,
    ext_qt5_dep_gui,
    qt5_dep,
    hdf5,
    boost,
    imagehlp_dep,
  ],
  install : true,
  install_rpath : ext_install_rpath,
  install_dir : ext_install_dir,
  cpp_args : [
    '-DOMPI_SKIP_MPICXX', # Needed to prevent unnecessary dependency when libhdf5-openmpi is used
    '-DNO_BOOST_FILESYSTEM_PATH', # Avoid dependency on BoostFilesystem
    '-DH5_BUILT_AS_DYNAMIC_LIB',
    additional_pluginhdf5_cpp_args,
  ],
  link_args : [
    '-ldl',
  ],
)

configure_file(
  input : name + '.json',
  output : name + '.json',
  copy : true,
)
install_data(name + '.json', install_dir : ext_install_dir)
