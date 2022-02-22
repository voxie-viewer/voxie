project('voxie', ['c', 'cpp'], default_options : [ 'debug=false', 'optimization=3', 'cpp_std=c++14', 'warning_level=3' ])

if get_option('plugins').enabled() and get_option('hdf5').enabled()
  if get_option('hdf5_path') != ''
    hdf5_path = get_option('hdf5_path')
    hdf5 = declare_dependency(
      include_directories : include_directories(hdf5_path + '/include'),
      link_args : [
        '-L' + join_paths(meson.current_source_dir(), hdf5_path) + '/lib',
        '-lhdf5',
      ],
    )
  else
    hdf5 = dependency('hdf5-serial', include_type : 'system')
  endif

  if get_option('boost_include_path') != ''
    boost_include_path = get_option('boost_include_path')
    boost = declare_dependency(
      include_directories : include_directories(boost_include_path),
    )
  else
    boost = declare_dependency()
  endif
endif

if get_option('plugins').enabled() and get_option('lapack').enabled()
  if get_option('lapacke_path') != ''
    lapacke_path = get_option('lapacke_path')
    lapacke = declare_dependency(
      include_directories : include_directories(lapacke_path + '/include'),
      link_args : [
        '-L' + join_paths(meson.current_source_dir(), lapacke_path) + '/lib',
        '-llapacke',
     ],
    )
  else
    lapacke = declare_dependency(link_args : [ '-llapacke' ])
  endif
endif


voxie_install_dir = 'lib/voxie'
lib_install_dir = voxie_install_dir + '/lib'
bin_install_dir = voxie_install_dir + '/bin'
doc_install_dir = voxie_install_dir + '/doc'


if not is_variable('additional_collect_licenses_args')
   additional_collect_licenses_args = []
endif
if get_option('additional_licenses_file') != ''
   additional_collect_licenses_args += files(get_option('additional_licenses_file'))
endif


fs = import('fs')
if get_option('intern').enabled() and fs.is_file('intern/meson.build')
  subdir('intern')
endif


subdir('src')
