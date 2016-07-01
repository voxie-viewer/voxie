QT += widgets script opengl
QT += dbus
unix {
    QT += x11extras
}

TARGET = Voxie
TEMPLATE = lib

include($$PWD/../CompilerFlags.pri)
include($$PWD/../OpenCL.pri)


DEFINES += VOXIECORE_LIBRARY

SOURCES += \
    data/colorizer.cpp \
    data/dataset.cpp \
    data/floatbuffer.cpp \
    data/floatimage.cpp \
    data/image.cpp \
    data/sharedmemory.cpp \
    data/slice.cpp \
    data/sliceimage.cpp \
    data/surface.cpp \
    data/surfacebuilder.cpp \
    data/voxeldata.cpp \
    filter/filter2d.cpp \
    filter/filter3d.cpp \
    filter/filterchain2d.cpp \
    filter/filterchain3d.cpp \
    filtermask/ellipse.cpp \
    filtermask/imagecomparator.cpp \
    filtermask/polygon.cpp \
    filtermask/rectangle.cpp \
    filtermask/selection2dmask.cpp \
    filtermask/selection3dmask.cpp \
    filtermask/shape3d.cpp \
    filtermask/test.cpp \
    histogram/histogram.cpp \
    io/importer.cpp \
    io/loader.cpp \
    io/operation.cpp \
    io/sliceexporter.cpp \
    io/voxelexporter.cpp \
    opencl/clinstance.cpp \
    opencl/clutil.cpp \
    plugin/metafilter2d.cpp \
    plugin/metafilter3d.cpp \
    plugin/metavisualizer.cpp \
    plugin/pluginmember.cpp \
    plugin/voxieplugin.cpp \
    scripting/client.cpp \
    scripting/dbustypes.cpp \
    scripting/scriptingcontainer.cpp \
    scripting/scriptingexception.cpp \
    visualization/filterchain2dwidget.cpp \
    visualization/filterchain3dwidget.cpp \
    visualization/openglwidget.cpp \
    visualization/qveclineedit.cpp \
    visualization/view3d.cpp \
    visualization/visualizer.cpp \
    ivoxie.cpp \
    spnav/spacenavevent.cpp \
    spnav/spacenavclient.cpp \
    spnav/spacenavclient_x11.cpp \
    spnav/spacenavvisualizer.cpp

HEADERS += \
    data/colorizer.hpp \
    data/dataset.hpp \
    data/floatbuffer.hpp \
    data/floatimage.hpp \
    data/image.hpp \
    data/interpolationmethod.hpp \
    data/plane.hpp \
    data/range.hpp \
    data/sharedmemory.hpp \
    data/slice.hpp \
    data/sliceimage.hpp \
    data/sliceimagecontext.hpp \
    data/surface.hpp \
    data/surfacebuilder.hpp \
    data/voxeldata.hpp \
    filter/filter2d.hpp \
    filter/filter3d.hpp \
    filter/filterchain2d.hpp \
    filter/filterchain3d.hpp \
    filtermask/imagecomparator.hpp \
    filtermask/polygonPoint.hpp \
    filtermask/rectangleData.hpp \
    filtermask/selection2dmask.hpp \
    filtermask/selection3dmask.hpp \
    filtermask/shape3d.hpp \
    filtermask/shapes.hpp \
    histogram/histogram.hpp \
    io/importer.hpp \
    io/loader.hpp \
    io/operation.hpp \
    io/sliceexporter.hpp \
    io/voxelexporter.hpp \
    opencl/clinstance.hpp \
    opencl/clutil.hpp \
    plugin/interfaces.hpp \
    plugin/metafilter2d.hpp \
    plugin/metafilter3d.hpp \
    plugin/metavisualizer.hpp \
    plugin/pluginmember.hpp \
    plugin/voxieplugin.hpp \
    visualization/filterchain2dwidget.hpp \
    visualization/filterchain3dwidget.hpp \
    visualization/openglwidget.hpp \
    visualization/qveclineedit.hpp \
    visualization/view3d.hpp \
    visualization/visualizer.hpp \
    _minmax.hpp \
    ivoxie.hpp \
    voxiecore_global.hpp \
    filtermask/ellipse.hpp \
    filtermask/polygon.hpp \
    filtermask/polygonData.hpp \
    filtermask/ellipseData.hpp \
    filtermask/rectangle.hpp \
    scripting/client.hpp \
    scripting/dbustypes.hpp \
    scripting/scriptingcontainer.hpp \
    scripting/scriptingexception.hpp \
    filtermask/test.hpp \
    spnav/spacenavevent.hpp \
    spnav/spacenavclient.hpp \
    spnav/spacenavvisualizer.hpp


win32-msvc* {
    QMAKE_CXXFLAGS *= /FS /MP
}

CONFIG += copy_dir_files

headerDirs = $$dirname(HEADERS)
uniqueHeaderDirs = . $$unique(headerDirs)
for (dir, uniqueHeaderDirs) {
    headers-$${dir}.path = $$VOXIE_TARGET_PATH/include/Voxie/$$dir
    headers-$${dir}.files = $$dir/*.hpp
    INSTALLS += headers-$$dir
}

target.path = $$VOXIE_TARGET_PATH/lib
INSTALLS += target

OTHER_FILES +=

DISTFILES += \
    filtermask/imagecomparator.cl
