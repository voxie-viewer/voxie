name = 'ExtFilterMultiMaterialVolumeSegmentation'

moc_files = qt5.preprocess(
  moc_sources : [
  ],
  moc_headers : [
    #'MultiMaterialVolumeSegmentation.hpp',
    #'Voxel.hpp',
  ],
  dependencies : [ ext_dependencies, ext_qt5_dep_gui_moc ],
)

main = executable(
  name,
  [
    moc_files,

    'ExtFilterMultiMaterialVolumeSegmentation.cpp',
    'MultiMaterialVolumeSegmentation.cpp',
    'Voxel.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [
    ext_dependencies,
    ext_qt5_dep_gui,
    thread_dep,
  ],
  install : true,
  install_rpath : ext_install_rpath,
  install_dir : filter_install_dir,
  link_args : [
    '-pthread',
  ],  
)

configure_file(
  input : name + '.json',
  output : name + '.json',
  copy : true,
)
install_data(name + '.json', install_dir : filter_install_dir)
