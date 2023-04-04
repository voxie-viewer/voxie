# meson file containing instructions for building and installing the manual and for installing the DBus interface XML file and the wrapper script

if get_option('main').enabled()

# TODO: How to find the Qt framework path?
set_qt_libdir = []
if target_machine.system() == 'darwin'
  qt_qmake = find_program('qmake')
  qt_libdir = fs.parent(fs.parent(qt_qmake.full_path())) + '/lib'
  set_qt_libdir = [ 'env', 'DYLD_FRAMEWORK_PATH=' + qt_libdir ]
endif

line_ending_option = []
if target_machine.system() == 'windows'
   line_ending_option = [
     '--license-crlf',
   ]
endif

custom_target(
  'Licenses',
  output : [ 'licenses' ],
  command : [
    set_qt_libdir,
    collect_licenses,
    '@OUTDIR@',
    line_ending_option,
    additional_collect_licenses_args,
  ],
  build_by_default: true,
  install: true,
  install_dir: voxie_install_dir,
)

install_data('wrapper-script/voxie', install_dir : voxie_install_dir)

# TODO: Install link to wrapper here?
# custom_target(
#   'wrapper-link',
#   output : 'voxie',
#   command : [ 'ln', '-f', '-s', '../lib/voxie/voxie', '@OUTPUT@' ],
#   build_by_default : true,
#   install : true,
#   install_dir : 'bin',
# )


if not is_variable('additional_xml_files')
   additional_xml_files = []
endif
install_data(['../../de.uni_stuttgart.Voxie.xml'] + additional_xml_files, install_dir : voxie_install_dir)

# TODO: Filter files to copy?
install_subdir('../../pythonlib/voxie', install_dir : voxie_install_dir + '/pythonlib', exclude_directories : '__pycache__', exclude_files : '.gitignore')
install_data(['../../de.uni_stuttgart.Voxie.xml'] + additional_xml_files, install_dir : voxie_install_dir + '/pythonlib/voxie')

# TODO: Don't install __pycache__
# TODO: Filter files to copy?
install_subdir('../../filters', install_dir : voxie_install_dir, exclude_directories : '__pycache__', exclude_files : [ '.gitignore', 'importMinecraftLevel.nbt/.gitignore', 'lib/.gitignore' ])

foreach name : bundeled_python_packages
  # TODO: Don't install everything
  install_subdir('../../lib/python/' + name, install_dir : voxie_install_dir + '/python-extra', exclude_directories : '__pycache__', strip_directory : true)
endforeach

# TODO: Don't install __pycache__
# TODO: Filter files to copy?
install_subdir('../../ext', install_dir : voxie_install_dir, exclude_directories : '__pycache__', exclude_files : [ '.gitignore', 'importMinecraftLevel.nbt/.gitignore', 'lib/.gitignore' ])

# TODO: Filter files to copy?
install_subdir('../../doc/topic', install_dir : doc_install_dir)
install_subdir('../../doc/prototype', install_dir : doc_install_dir)

install_subdir('../../lib/katex-0.11.1', install_dir : lib_install_dir)

install_data('../../scripts/getAverage.py', install_dir : script_install_dir)

endif
