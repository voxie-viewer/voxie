QT += core gui widgets
QT += dbus

include($$PWD/../Plugin.pri)

HEADERS += \
    colorizerworker.hpp \
    histogramwidget.hpp \
    imagegeneratorworker.hpp \
    imagepaintwidget.hpp \
    makehandbutton.hpp \
    sliceimagecolorizerwidget.hpp \
    slicemetavisualizer.hpp \
    vissliceplugin.hpp \
    slicevisualizer.hpp \
    toolsliceadjustment.hpp \
    toolvalueviewer.hpp \
    toolvisualizer2d.hpp \
    toolzoom.hpp \
    toolselectionpolygon.hpp \
    toolselectionellipse.hpp \
    toolselection.hpp \
    toolexport.hpp \
    histogramworker.hpp

SOURCES += \
    colorizerworker.cpp \
    histogramwidget.cpp \
    imagegeneratorworker.cpp \
    imagepaintwidget.cpp \
    sliceimagecolorizerwidget.cpp \
    slicemetavisualizer.cpp \
    vissliceplugin.cpp \
    slicevisualizer.cpp \
    toolsliceadjustment.cpp \
    toolvalueviewer.cpp \
    toolvisualizer2d.cpp \
    toolzoom.cpp \
    toolselectionpolygon.cpp \
    toolselectionellipse.cpp \
    toolselection.cpp \
    toolexport.cpp \
    histogramworker.cpp
