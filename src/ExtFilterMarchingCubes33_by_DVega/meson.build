name = 'ExtFilterMarchingCubes33_by_DVega'

moc_files = qt5.preprocess(
  moc_sources : [
  ],
  moc_headers : [
  ],
  dependencies : [ ext_dependencies, ext_qt5_dep_gui_moc ],
)

main = executable(
  name,
  [
    moc_files,

    'ExtFilterMarchingcubes33.cpp',
    'grid3d.cpp',
    'helper.cpp',
    'marchingcubes33.cpp',
    'surface.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [ ext_dependencies, ext_qt5_dep_gui ],
  install : true,
  install_rpath : ext_install_rpath,
  install_dir : filter_install_dir,
  cpp_args : [ '-Dsize_type_GRD=4', '-DMC33_double_precision=0' ],
)

configure_file(
  input : name + '.json',
  output : name + '.json',
  copy : true,
)
install_data(name + '.json', install_dir : filter_install_dir)
