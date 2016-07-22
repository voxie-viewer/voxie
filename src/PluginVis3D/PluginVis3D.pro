QT += core gui widgets opengl
QT += dbus

include($$PWD/../Plugin.pri)

RESOURCES += \
    PluginVis3D.qrc

win32:LIBS += -lopengl32

SOURCES += vis3dplugin.cpp \
    cuberille.cpp \
    isosurfacemetavisualizer.cpp \
    isosurfacevisualizer.cpp \
    isosurfaceview.cpp \
    marchingcubes.cpp \
    surfaceextractor.cpp \
    xrayvisualizer.cpp \
    xraymetavisualizer.cpp

HEADERS += vis3dplugin.hpp \
    cuberille.hpp \
    isosurfacemetavisualizer.hpp \
    isosurfacevisualizer.hpp \
    isosurfaceview.hpp \
    marchingcubes.hpp \
    surfaceextractor.hpp \
    sharpthread.hpp \
    xrayvisualizer.hpp \
    xraymetavisualizer.hpp

!isEmpty(ENABLE_OSVR) {
    INCLUDEPATH += $$OSVR_CORE_SRC_DIR/inc $$OSVR_CORE_BUILD_DIR/src
    INCLUDEPATH += $$OSVR_RENDERMANAGER_SRC_DIR $$OSVR_RENDERMANAGER_BUILD_DIR
    unix {
        # Specify includes again with -isystem to suppress warnings
        QMAKE_CXXFLAGS += -isystem $$OSVR_CORE_SRC_DIR/inc -isystem $$OSVR_CORE_BUILD_DIR/src
        QMAKE_CXXFLAGS += -isystem $$OSVR_RENDERMANAGER_SRC_DIR -isystem $$OSVR_RENDERMANAGER_BUILD_DIR
    }
    LIBS += -L$$OSVR_CORE_BUILD_DIR/lib -losvrClientKit
    LIBS += -L$$OSVR_RENDERMANAGER_BUILD_DIR/lib -losvrRenderManager
    LIBS += -Wl,-rpath,$$OSVR_CORE_BUILD_DIR/lib
    LIBS += -Wl,-rpath,$$OSVR_RENDERMANAGER_BUILD_DIR/lib
    DEFINES += ENABLE_OSVR

    SOURCES += osvr/osvrdisplay.cpp

    HEADERS += osvr/osvrdisplay.hpp
}
