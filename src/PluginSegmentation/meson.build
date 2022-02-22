qt_modules = [
  'Core',
  'DBus',
  'Gui',
  'Widgets',
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

moc_files = qt5.preprocess(
  ui_files : [
  ],
  moc_sources : [
  ],
  moc_headers : [
    #'Segmentation.hpp',
    'PluginSegmentation.hpp',
    'Prototypes.hpp',
    'StepManager.hpp',
    'Gui/SegmentationWidget.hpp',
    'Gui/ThresholdWidget.hpp',
    'Gui/ThresholdSelectionWidget.hpp',
    #'Gui/HistoryViewModel.hpp',
    #'SegmentationUtils.hpp',
    #'Steps/ManualSelectionStep.hpp',
    #'Steps/AssignmentStep.hpp',
    #'Steps/MetaStep.hpp',
    #'Steps/MultiThresholdStep.hpp',
    #'Steps/ThresholdSelectionStep.hpp',
    #'Steps/BrushSelectionStep.hpp',
    #'Steps/LassoSelectionStep.hpp',
    #'Steps/RemoveLabelStep.hpp',
    #'Steps/SubtractStep.hpp',
  ],
  dependencies : [ plugin_dependencies, qt5_dep_moc ],
)

plugin = shared_library(
  'VoxiePluginSegmentation',
  [
    moc_files,
    'Segmentation.cpp',
    'PluginSegmentation.cpp',
    'Prototypes.cpp',
    'StepManager.cpp',
    'SegmentationUtils.cpp',
    'Gui/SegmentationWidget.cpp',
    'Gui/ThresholdSelectionWidget.cpp',
    'Gui/ThresholdWidget.cpp',
    'Gui/HistoryViewModel.cpp',
    'Steps/ManualSelectionStep.cpp',
    'Steps/AssignmentStep.cpp',
    'Steps/MetaStep.cpp',
    'Steps/MultiThresholdStep.cpp',
    'Steps/ThresholdSelectionStep.cpp',
    'Steps/BrushSelectionStep.cpp',
    'Steps/LassoSelectionStep.cpp',
    'Steps/RemoveLabelStep.cpp',
    'Steps/SubtractStep.cpp',
  ],
  implicit_include_directories : false,
  dependencies : [
    plugin_dependencies,
    qt5_dep,
  ],
  install : true,
  install_dir : plugin_install_dir,
  cpp_args : [
    '-DIN_PLUGINSEGMENTATION',
  ],
)
