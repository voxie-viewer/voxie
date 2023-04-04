enableOSVR = false # TODO
osvrHeaders = []
osvrSources = []
if enableOSVR
  osvrHeaders = [
    'Osvr/OsvrDisplay.hpp',
  ]
  osvrSources = [
    'Osvr/OsvrDisplay.cpp',
  ]
endif
# TODO
    # INCLUDEPATH += $$OSVR_CORE_SRC_DIR/inc $$OSVR_CORE_BUILD_DIR/src
    # INCLUDEPATH += $$OSVR_RENDERMANAGER_SRC_DIR $$OSVR_RENDERMANAGER_BUILD_DIR
    # unix {
    #     # Specify includes again with -isystem to suppress warnings
    #     QMAKE_CXXFLAGS += -isystem $$OSVR_CORE_SRC_DIR/inc -isystem $$OSVR_CORE_BUILD_DIR/src
    #     QMAKE_CXXFLAGS += -isystem $$OSVR_RENDERMANAGER_SRC_DIR -isystem $$OSVR_RENDERMANAGER_BUILD_DIR
    # }
    # LIBS += -L$$OSVR_CORE_BUILD_DIR/lib -losvrClientKit
    # LIBS += -L$$OSVR_RENDERMANAGER_BUILD_DIR/lib -losvrRenderManager
    # LIBS += -Wl,-rpath,$$OSVR_CORE_BUILD_DIR/lib
    # LIBS += -Wl,-rpath,$$OSVR_RENDERMANAGER_BUILD_DIR/lib
    # DEFINES += ENABLE_OSVR


qt_modules = [
  'Core',
  'DBus',
  'Gui',
  'Widgets',
  'OpenGL',
]
# Note: moc does not like "include_type:'system'" because meson only considers -I and -D for moc, not -isystem: <https://github.com/mesonbuild/meson/blob/398df5629863e913fa603cbf02c525a9f501f8a8/mesonbuild/modules/qt.py#L193>. See also <https://github.com/mesonbuild/meson/pull/8139>. Note that currently voxie should compile even without this workaround, see update-dbus-proxies.py.
qt5_dep = dependency('qt5', modules : qt_modules, include_type : 'system')
qt5_dep_moc = dependency('qt5', modules : qt_modules)

if not is_variable('additional_pluginvis3d_resources')
   additional_pluginvis3d_resources = []
endif
if not is_variable('additional_pluginvis3d_cpp_args')
   additional_pluginvis3d_cpp_args = []
endif
moc_files = qt5.preprocess(
  qresources : [
    'PluginVis3D.qrc',
    additional_pluginvis3d_resources,
  ],
  ui_files : [
    'Grid3DWidget.ui',
    'LightSourceListElement.ui',
    'LightSourceProperties.ui',
    'PlanePropertiesUi.ui',
    'RenderImplementationSelection.ui',
  ],
  moc_sources : [
  ],
  moc_headers : [
    'GeometricPrimitive.hpp',
    'Grid.hpp',
    'Grid3D.hpp',
    'Grid3DWidget.hpp',
    'LightSource.hpp',
    'LightSourceListElement.hpp',
    'LightSourceProperties.hpp',
    'Plane.hpp',
    'PlanePropertiesUi.hpp',
    #'Prototypes.forward.hpp',
    'Prototypes.hpp',
    'RenderImplementationSelection.hpp',
    'Surface.hpp',
    'Test3DObject.hpp',
    'ThreadSafe_MxN_Matrix.hpp',
    'View3DPropertiesConnection.hpp',
    'Vis3DPlugin.hpp',
    'Vis3DShaders.hpp',
    'Visualizer3D.hpp',
    'Visualizer3DView.hpp',
    'VolumeImageRenderer.hpp',
    'VolumeRenderingVisualizer.hpp',

    'Data/AxisFilter.hpp',
    'Data/CuttingPlane.hpp',
    'Data/PlaneData.hpp',
    'Data/SurfaceData3D.hpp',
    'Data/Texture.hpp',

    #'Helper/NotifyingLinkedList.hpp',
    'Helper/NotifyingList.hpp',
    #'Helper/PlaneShader.hpp',
    #'Helper/gluhelper.hpp',

    osvrHeaders,
  ],
  dependencies : [ plugin_dependencies, qt5_dep_moc ],
)

plugin = shared_library(
  'VoxiePluginVis3D',
  [
    moc_files,

    'GeometricPrimitive.cpp',
    'Grid.cpp',
    'Grid3D.cpp',
    'Grid3DWidget.cpp',
    'LightSource.cpp',
    'LightSourceListElement.cpp',
    'LightSourceProperties.cpp',
    'Plane.cpp',
    'PlanePropertiesUi.cpp',
    'Prototypes.cpp',
    'RenderImplementationSelection.cpp',
    'Surface.cpp',
    'Test3DObject.cpp',
    'ThreadSafe_MxN_Matrix.cpp',
    'View3DPropertiesConnection.cpp',
    'Vis3DPlugin.cpp',
    'Vis3DShaders.cpp',
    'Visualizer3D.cpp',
    'Visualizer3DView.cpp',
    'VolumeImageRenderer.cpp',
    'VolumeRenderingVisualizer.cpp',

    'Data/SurfaceData3D.cpp',

    'Helper/NotifyingLinkedList.cpp',
    'Helper/NotifyingList.cpp',
    'Helper/PlaneShader.cpp',
    'Helper/gluhelper.cpp',

    osvrSources,
  ],
  implicit_include_directories : false,
  dependencies : [
    plugin_dependencies,
    qt5_dep,
  ],
  install : true,
  install_dir : plugin_install_dir,
  cpp_args : [
    additional_pluginvis3d_cpp_args,
  ],
)
