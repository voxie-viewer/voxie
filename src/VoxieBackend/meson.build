qt_modules = [
  'Core',
  'DBus',
  'Gui', # TODO: remove?
  'Widgets', # TODO: remove
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

rt_dep = meson.get_compiler('c').find_library('rt', required : false)

if not is_variable('additional_voxiebackend_moc_sources')
   additional_voxiebackend_moc_sources = []
endif
if not is_variable('additional_voxiebackend_moc_headers')
   additional_voxiebackend_moc_headers = []
endif
if not is_variable('additional_voxiebackend_pub_cpp_args')
   additional_voxiebackend_pub_cpp_args = []
endif
moc_files = qt5.preprocess(
  moc_sources : [
    'Data/VolumeData.cpp',
    'Data/VolumeDataVoxel.cpp',
    'Data/VolumeStructure.cpp',
    'Data/VolumeStructureVoxel.cpp',

    'Property/PropertyBase.cpp',
  ] + additional_voxiebackend_moc_sources,
  moc_headers : [
    #'DebugOptions.hpp',
    #'VoxieBackend.hpp',

    'Component/Component.hpp',
    #'Component/ComponentContainer.hpp',
    'Component/ComponentContainerList.hpp',
    #'Component/ComponentTypeInfo.hpp',
    #'Component/ComponentType.hpp',
    #'Component/ExtensionExporter.hpp',
    #'Component/ExtensionImporter.hpp',
    'Component/Extension.hpp',
    #'Component/ExtensionLauncher.hpp',
    'Component/ExternalOperation.hpp',

    #'DBus/ClientWrapper.hpp',
    #'DBus/DBusTypes.hpp',
    'DBus/DynamicObject.hpp',
    #'DBus/ObjectWrapper.hpp',

    'Data/Data.hpp',
    #'Data/DataChangedReason.hpp',
    #'Data/DataType.hpp',
    'Data/EventListDataAccessor.hpp',
    'Data/EventListDataBuffer.hpp',
    #'Data/FloatBuffer.hpp',
    #'Data/FloatImage.hpp',
    #'Data/GeometricPrimitive.hpp',
    'Data/GeometricPrimitiveData.hpp',
    'Data/GeometricPrimitiveType.hpp',
    'Data/HistogramProvider.hpp',
    'Data/ImageDataPixel.hpp',
    #'Data/ImageDataPixelInst.hpp',
    #'Data/InterpolationMethod.hpp',
    #'Data/PlaneInfo.hpp',
    'Data/TomographyRawData2DAccessor.hpp',
    #'Data/SharedMemory.hpp',
    #'Data/SharedMemoryArray.hpp',
    #'Data/SliceImage.hpp',
    #'Data/SliceImageContext.hpp',
    #'Data/SurfaceAttribute.hpp',
    'Data/SurfaceData.hpp',
    'Data/TomographyRawData2DRegular.hpp',
    #'Data/TomographyRawData2DRegularInst.hpp',
    #'Data/TomographyRawData2DRegularShmemInst.hpp',
    'Data/VolumeData.hpp',
    'Data/VolumeDataVoxel.hpp',
    #'Data/VolumeDataVoxelInst.hpp',
    'Data/VolumeStructure.hpp',
    'Data/VolumeStructureVoxel.hpp',

    'IO/Exporter.hpp',
    #'IO/FilenameFilter.hpp',
    'IO/Importer.hpp',
    'IO/Operation.hpp',
    'IO/OperationImport.hpp',
    'IO/OperationRegistry.hpp',
    'IO/OperationResult.hpp',
    'IO/SharpThread.hpp',

    'OpenCL/CLInstance.hpp',
    #'OpenCL/CLUtil.hpp',
    #'OpenCL/Lazy.hpp',

    'Property/PropertyBase.hpp',
    'Property/PropertyType.hpp',

    #'lib/CL/cl-lazy-define.hpp',
    #'lib/CL/cl-lazy.hpp',
    #'lib/CL/cl-patched.hpp',
  ] + additional_voxiebackend_moc_headers,
  include_directories : [ project_incdir ],
  dependencies : [ qt5_dep_moc, half_dep],
)

if not is_variable('additional_voxiebackend_sources')
   additional_voxiebackend_sources = []
endif
voxiebackend = shared_library(
  'VoxieBackend',
  [
    moc_files,

    'DebugOptions.cpp',

    'Component/Component.cpp',
    'Component/ComponentContainer.cpp',
    'Component/ComponentContainerList.cpp',
    'Component/ComponentTypeInfo.cpp',
    'Component/ComponentType.cpp',
    'Component/Extension.cpp',
    'Component/ExtensionExporter.cpp',
    'Component/ExtensionImporter.cpp',
    'Component/ExtensionLauncher.cpp',
    'Component/ExternalOperation.cpp',

    'DBus/ClientWrapper.cpp',
    'DBus/DBusTypes.cpp',
    'DBus/DynamicObject.cpp',
    'DBus/ObjectWrapper.cpp',

    'Data/Data.cpp',
    'Data/DataChangedReason.cpp',
    'Data/DataType.cpp',
    'Data/EventListDataAccessor.cpp',
    'Data/EventListDataBuffer.cpp',
    'Data/FloatBuffer.cpp',
    'Data/FloatImage.cpp',
    'Data/GeometricPrimitive.cpp',
    'Data/GeometricPrimitiveData.cpp',
    'Data/GeometricPrimitiveType.cpp',
    'Data/HistogramProvider.cpp',
    'Data/ImageDataPixel.cpp',
    'Data/ImageDataPixelInst.cpp',
    'Data/InterpolationMethod.cpp',
    'Data/TomographyRawData2DAccessor.cpp',
    'Data/SharedMemory.cpp',
    'Data/SharedMemoryArray.cpp',
    'Data/SliceImage.cpp',
    'Data/SurfaceAttribute.cpp',
    'Data/SurfaceData.cpp',
    'Data/TomographyRawData2DRegular.cpp',
    'Data/TomographyRawData2DRegularInst.cpp',
    'Data/TomographyRawData2DRegularShmemInst.cpp',
    'Data/VolumeData.cpp',
    'Data/VolumeDataVoxel.cpp',
    'Data/VolumeDataVoxelInst.cpp',
    'Data/VolumeStructure.cpp',
    'Data/VolumeStructureVoxel.cpp',

    'IO/Exporter.cpp',
    'IO/FilenameFilter.cpp',
    'IO/Importer.cpp',
    'IO/Operation.cpp',
    'IO/OperationImport.cpp',
    'IO/OperationRegistry.cpp',
    'IO/OperationResult.cpp',

    'OpenCL/CLInstance.cpp',
    'OpenCL/CLUtil.cpp',
    'OpenCL/Lazy.cpp',

    'Property/PropertyBase.cpp',
    'Property/PropertyType.cpp',
  ] + additional_voxiebackend_sources,
  include_directories : [
    project_incdir,
    opencl_incdir,
    valgrind_incdir,
  ],
  implicit_include_directories : false,
  dependencies : [
    qt5_dep,
    rt_dep,
    voxieclient_dep,
  ],
  cpp_args : [
    '-DVOXIEBACKEND_LIBRARY',
    additional_voxiebackend_pub_cpp_args,
  ],
  install : true,
  install_dir : lib_install_dir,
)

voxiebackend_dep = declare_dependency(
  include_directories : [
    project_incdir,
    opencl_incdir,
  ],
  link_with : [
    voxiebackend,
  ],
  dependencies : [
    voxieclient_dep,
  ],
  compile_args : additional_voxiebackend_pub_cpp_args,
)

# TODO: Install headers?
