name = 'ExtFilterVolumeRegistration'

moc_files = qt5.preprocess(
  moc_sources : [
  ],
  moc_headers : [
    #'VolumeRegistration.hpp',
  ],
  dependencies : [ ext_dependencies, ext_qt5_dep_gui_moc ],
)

fft = declare_dependency(include_directories : include_directories('../../lib/kissfft'), sources : [ '../../lib/kissfft/kiss_fft.c', '../../lib/kissfft/kiss_fftnd.c' ], compile_args : [ '-DUSE_FFTW=0' ])
#fft = declare_dependency(link_args : [ '-lfftw3f' ], compile_args : [ '-DUSE_FFTW=1' ])

main = executable(
  name,
  [
    moc_files,

    'ExtFilterVolumeRegistration.cpp',
    'VolumeRegistration.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [
    ext_dependencies,
    ext_qt5_dep_gui,
    fft,
  ],
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
