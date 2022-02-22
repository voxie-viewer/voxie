name = 'ExtFilterMarchingCubes33'

moc_files = qt5.preprocess(
  moc_sources : [
  ],
  moc_headers : [
    #'MarchingCubes33.hpp',
  ],
  dependencies : [ ext_dependencies, ext_qt5_dep_gui_moc ],
)

main = executable(
  name,
  [
    moc_files,

    'ExtFilterMarchingCubes33.cpp',
    'MarchingCubes33.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [ ext_dependencies, ext_qt5_dep_gui ],
  install : true,
  install_rpath : ext_install_rpath,
  install_dir : filter_install_dir,
)

configure_file(
  input : name + '.json',
  output : name + '.json',
  copy : true,
)
install_data(name + '.json', install_dir : filter_install_dir)
