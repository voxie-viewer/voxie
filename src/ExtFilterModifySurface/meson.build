name = 'ExtFilterModifySurface'

moc_files = qt5.preprocess(
  moc_sources : [
  ],
  moc_headers : [
    #'BilateralFiltering.hpp',
    #'Edge.hpp',
    #'FastEffectiveDPFilter.hpp',
    #'FeatureConvincedDenoising.hpp',
    #'MeanNormalFiltering.hpp',
    #'NoiseApplicator.hpp',
    #'ProgressiveMeshDecimation.hpp',
    'TaubinFiltering.hpp',
  ],
  dependencies : [ ext_dependencies, ext_qt5_dep_gui_moc ],
)

main = executable(
  name,
  [
    moc_files,

    'BilateralFiltering.cpp',
    'Edge.cpp',
    'ExtFilterModifySurface.cpp',
    'FastEffectiveDPFilter.cpp',
    'FeatureConvincedDenoising.cpp',
    'MeanNormalFiltering.cpp',
    'NoiseApplicator.cpp',
    'ProgressiveMeshDecimation.cpp',
    'TaubinFiltering.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [ ext_dependencies, ext_qt5_dep_gui ],
  install : true,
  install_rpath : ext_install_rpath,
  install_dir : filter_install_dir,
  # cpp_args : '-Wconversion',
)

configure_file(
  input : name + '.json',
  output : name + '.json',
  copy : true,
)
install_data(name + '.json', install_dir : filter_install_dir)
