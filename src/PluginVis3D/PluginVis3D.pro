QT += core gui widgets opengl
QT += dbus

include($$PWD/../Plugin.pri)

RESOURCES += \
    Voxie3D.qrc

win32:LIBS += -lopengl32

SOURCES += voxie3d.cpp \
    isosurfacemetavisualizer.cpp \
    isosurfacevisualizer.cpp \
    isosurfaceview.cpp \
    marchingcubes.cpp \
    xrayvisualizer.cpp \
    xraymetavisualizer.cpp

HEADERS += voxie3d.hpp \
    isosurfacemetavisualizer.hpp \
    isosurfacevisualizer.hpp \
    isosurfaceview.hpp \
    marchingcubes.hpp \
    sharpthread.hpp \
    xrayvisualizer.hpp \
    xraymetavisualizer.hpp
