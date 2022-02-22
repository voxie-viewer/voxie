moc_files = qt5.preprocess(
  moc_sources : [
  ],
  moc_headers : [
  ],
  dependencies : [ ext_dependencies, ext_qt5_dep_widgets_moc ],
)

main = executable(
  'ScriptGetAverage',
  [
    moc_files,

    'ScriptGetAverage.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [ ext_dependencies, ext_qt5_dep_widgets ],
  install : true,
  install_rpath : ext_install_rpath,
  install_dir : script_install_dir,
)
