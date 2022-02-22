name = 'ExtFileVgi'

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

    'ExtFileVgi.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [ ext_dependencies, ext_qt5_dep_gui ],
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
