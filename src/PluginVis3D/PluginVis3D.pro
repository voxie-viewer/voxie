QT += core gui widgets opengl
QT += dbus

include($$PWD/../Plugin.pri)

RESOURCES += \
    Voxie3D.qrc

win32:LIBS += -lopengl32

SOURCES += voxie3d.cpp \
    cuberille.cpp \
    isosurfacemetavisualizer.cpp \
    isosurfacevisualizer.cpp \
    isosurfaceview.cpp \
    marchingcubes.cpp \
    surfaceextractor.cpp \
    xrayvisualizer.cpp \
    xraymetavisualizer.cpp

HEADERS += voxie3d.hpp \
    cuberille.hpp \
    isosurfacemetavisualizer.hpp \
    isosurfacevisualizer.hpp \
    isosurfaceview.hpp \
    marchingcubes.hpp \
    surfaceextractor.hpp \
    sharpthread.hpp \
    xrayvisualizer.hpp \
    xraymetavisualizer.hpp
