QT += core gui widgets
QT += dbus

include($$PWD/../Plugin.pri)

SOURCES += diffview.cpp \
    diffmetavisualizer.cpp \
    diffvisualizer.cpp \
    colorizerworker.cpp \
    diffimagecolorizerwidget.cpp \
    histogramwidget.cpp \
    imagegeneratorworker.cpp \
    imagepaintwidget.cpp \
    toolvisualizer2ddiff.cpp \
    toolvisualizer2d.cpp \
    toolselection.cpp \
    toolsliceadjustment.cpp \
    toolvalueviewer.cpp \
    toolzoom.cpp \
    toolexport.cpp \
    histogramworker.cpp

HEADERS += diffview.hpp \
    diffmetavisualizer.hpp \
    diffvisualizer.hpp \
    colorizerworker.hpp \
    diffimagecolorizerwidget.hpp \
    histogramwidget.hpp \
    imagegeneratorworker.hpp \
    imagepaintwidget.hpp \
    makehandbutton.hpp \
    toolvisualizer2ddiff.hpp \
    toolvisualizer2d.hpp \
    toolselection.hpp \
    toolsliceadjustment.hpp \
    toolvalueviewer.hpp \
    toolzoom.hpp \
    diffsliceimage.hpp \
    toolexport.hpp \
    histogramworker.hpp
