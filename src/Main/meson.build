qt_modules = [
  'Core',
]
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
build_voxie_path = executable(
  'BuildVoxiePath',
  [
    'BuildVoxiePath.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [
    qt5_dep,
  ],
)
if not is_variable('additional_build_voxie_path_args')
   additional_build_voxie_path_args = []
endif
custom_target(
  'VoxiePath',
  output : 'voxie-path.json',
  command : [
    build_voxie_path,
    '@OUTPUT@', meson.current_source_dir() + '/../..',
    meson.current_build_dir() + '/..',
    meson.current_build_dir() + '/../extra/old-manual.pdf',
    meson.current_build_dir() + '/../extra/licenses',
    additional_build_voxie_path_args,
  ],
  #build_by_default: true,
  # TODO: This will break running the voxie in the build dir on MacOS, but the CI currently fails otherwise with
  # TODO: Set DYLD_FRAMEWORK_PATH, see src/extra/build.meson?
  #   dyld: Library not loaded: @rpath/QtCore.framework/Versions/5/QtCore
  #   Referenced from: /Users/gitlab-runner/.../voxie-intern/build/src/Main/BuildVoxiePath
  #   Reason: image not found
  build_by_default: target_machine.system() != 'darwin',
)
if target_machine.system() == 'darwin'
  install_data('voxie-path-install-macos.json', rename : 'voxie-path.json', install_dir : bin_install_dir)
else
  install_data('voxie-path-install.json', rename : 'voxie-path.json', install_dir : bin_install_dir)
endif


qt_modules = [
  'Core',
]
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
collect_licenses = executable(
  'CollectLicenses',
  [
    'CollectLicenses.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [
    qt5_dep,
    voxiebackend_dep,
  ],
)


qt_modules = [
  'Core',
  'DBus',
  'Gui',
  'Widgets',
  'Script',
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

# get git revision in c++
GIT_REVISION = run_command(['git', 'rev-parse', 'HEAD'], check : true).stdout().strip()
GIT_REVISION_DESCRIBE = run_command(['git', 'describe', '--always', '--tags'], check : true).stdout().strip()
# Build the git revision into a static library to avoid passing GIT_VERSION to the compilation of every file
if not is_variable('additional_version_cpp_args')
   additional_version_cpp_args = []
endif
gitVersionLibrary = static_library(
  'GitVersion',
  [ 'Version.cpp' ],
  implicit_include_directories : false,
  cpp_args : [
    '-DGIT_REVISION="' + GIT_REVISION + '"',
    '-DGIT_REVISION_DESCRIBE="' + GIT_REVISION_DESCRIBE + '"',
    additional_version_cpp_args,
  ],
  dependencies : [
    qt5_dep,
  ],
)

additional_collect_licenses_args += files(
  '../../lib/icons.sw.json',
)

if not is_variable('additional_main_resources')
   additional_main_resources = []
endif
moc_files = qt5.preprocess(
  qresources : [
    'cl_kernels.qrc',
    'help.qrc',
    '../../lib/icons.qrc',
    'icons-voxie.qrc',
    'tags.qrc',
  ] + additional_main_resources,
  ui_files : [
  ],
  moc_sources : [
    'Utilities.cpp',
  ],
  moc_headers : [
    'DirectoryManager.hpp',
    'Instance.hpp',
    #'MetatypeRegistration.hpp',
    'Root.hpp',
    'ScriptWrapper.hpp',
    'Utilities.hpp',

    #'Component/ComponentTypes.hpp',
    'Component/ExtensionFilterNode.hpp',
    #'Component/ExtensionNodePrototype.hpp',
    'Component/ExtensionSegmentationStep.hpp',

    'Component/ScriptLauncher.hpp',
    #'Component/SessionManager.hpp',
    'Component/Tool.hpp',

    'Gui/AboutDialogWindow.hpp',
    'Gui/ButtonLabel.hpp',
    'Gui/CoreWindow.hpp',
    #'Gui/DataFlowScene.hpp',
    #'Gui/DataNodeUI.hpp',
    #'Gui/Edge.hpp',
    'Gui/GraphWidget.hpp',
    'Gui/HelpWindow.hpp',
    #'Gui/Node.hpp',
    'Gui/OpenFileDialog.hpp',
    'Gui/PlaneView.hpp',
    'Gui/PluginManagerWindow.hpp',
    #'Gui/PosRotValid.hpp',
    'Gui/PreferencesWindow.hpp',
    'Gui/ScriptConsole.hpp',
    'Gui/ScriptLineEdit.hpp',
    'Gui/SelectObjectConnection.hpp',
    #'Gui/SelectWindow.hpp',
    'Gui/SidePanel.hpp',
    'Gui/SliceView.hpp',
    'Gui/VisualizerContainer.hpp',
    'Gui/VScrollArea.hpp',

    'Gui/About/AboutLicenseTab.hpp',
    'Gui/About/InformationTab.hpp',
    'Gui/About/ThirdPartyTab.hpp',

    'Gui/Preferences/OpenclPreferences.hpp',
    'Gui/Preferences/ScriptPreferences.hpp',

    #'IO/Load.hpp',
    'IO/RunAllFilterOperation.hpp',
    'IO/RunMultipleFilterOperationBase.hpp',
    'IO/RunMultipleFilterOperation.hpp',

    #'Help/CMark.hpp',
    #'Help/CMarkUtil.hpp',
    #'Help/ExportHelpPages.hpp',
    'Help/HelpBrowserBackendQTextBrowser.hpp',
    #'Help/HelpLinkHandler.hpp',
    #'Help/HelpPageGenerator.hpp',
    #'Help/HelpPage.hpp',
    #'Help/HelpPageRegistry.hpp',
    #'Help/HelpPageSourceAll.hpp',
    #'Help/HelpPageSourceDBusInterface.hpp',
    #'Help/HelpPageSource.hpp',
    #'Help/HelpPageSourceIndex.hpp',
    #'Help/HelpPageSourcePrototype.hpp',
    #'Help/HelpPageSourceTopic.hpp',
  ],
  include_directories : [ project_incdir ],
  dependencies : qt5_dep_moc,
)

main = executable(
  'voxie',
  [
    moc_files,

    'DirectoryManager.cpp',
    'Instance.cpp',
    'Main.cpp',
    'MetatypeRegistration.cpp',
    'Root.cpp',
    'ScriptWrapper.cpp',
    'Utilities.cpp',

    'Component/ComponentTypes.cpp',
    'Component/ExtensionNodePrototype.cpp',
    'Component/ExtensionFilterNode.cpp',
    'Component/ExtensionSegmentationStep.cpp',
    'Component/ScriptLauncher.cpp',
    'Component/SessionManager.cpp',
    'Component/Tool.cpp',

    'Gui/AboutDialogWindow.cpp',
    'Gui/ButtonLabel.cpp',
    'Gui/CoreWindow.cpp',
    'Gui/DataFlowScene.cpp',
    'Gui/DataNodeUI.cpp',
    'Gui/Edge.cpp',
    'Gui/GraphWidget.cpp',
    'Gui/HelpWindow.cpp',
    'Gui/Node.cpp',
    'Gui/OpenFileDialog.cpp',
    'Gui/PlaneView.cpp',
    'Gui/PluginManagerWindow.cpp',
    'Gui/PosRotValid.cpp',
    'Gui/PreferencesWindow.cpp',
    'Gui/ScriptConsole.cpp',
    'Gui/ScriptLineEdit.cpp',
    'Gui/SelectObjectConnection.cpp',
    'Gui/SelectWindow.cpp',
    'Gui/NodeGroupSelectWindow.cpp',
    'Gui/SidePanel.cpp',
    'Gui/SliceView.cpp',
    'Gui/VisualizerContainer.cpp',
    'Gui/VScrollArea.cpp',

    'Gui/About/AboutLicenseTab.cpp',
    'Gui/About/InformationTab.cpp',
    'Gui/About/ThirdPartyTab.cpp',

    'Gui/Preferences/OpenclPreferences.cpp',
    'Gui/Preferences/ScriptPreferences.cpp',

    'IO/Load.cpp',
    'IO/RunAllFilterOperation.cpp',
    'IO/RunMultipleFilterOperationBase.cpp',
    'IO/RunMultipleFilterOperation.cpp',

    'Help/CMark.cpp',
    'Help/CMarkUtil.cpp',
    'Help/ExportHelpPages.cpp',
    'Help/HelpBrowserBackendQTextBrowser.cpp',
    'Help/HelpLinkHandler.cpp',
    'Help/HelpPage.cpp',
    'Help/HelpPageGenerator.cpp',
    'Help/HelpPageRegistry.cpp',
    'Help/HelpPageSourceAll.cpp',
    'Help/HelpPageSource.cpp',
    'Help/HelpPageSourceDBusInterface.cpp',
    'Help/HelpPageSourceIndex.cpp',
    'Help/HelpPageSourcePrototype.cpp',
    'Help/HelpPageSourceTopic.cpp',
  ],
  include_directories : [
    project_incdir,
    opencl_incdir,
    cmark_incdir,
  ],
  implicit_include_directories : false,
  dependencies : [
    qt5_dep,
    libvoxie_dep,
  ],
  link_with : [
    cmark,
    gitVersionLibrary,
  ],
  link_args : [
    gui_link_flags,
  ],
  install : true,
  install_dir : bin_install_dir,
  install_rpath : bin_install_rpath,
  win_subsystem : 'windows',
)
