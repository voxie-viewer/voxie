if get_option('main').enabled()
  subdir('cmark')
endif

bundeled_python_packages = [
  'funcsigs-1.0.2',
  'reikna-0.7.5',
  'nbt',
]

if not is_variable('additional_build_voxie_path_args')
   additional_build_voxie_path_args = []
endif
foreach name : bundeled_python_packages
  additional_build_voxie_path_args += [
    '--additional-python-lib-dir=' + meson.current_source_dir() + '/../../lib/python/' + name,
  ]
  additional_collect_licenses_args += files(
    '../../lib/python/' + name + '.sw.json',
  )
endforeach
