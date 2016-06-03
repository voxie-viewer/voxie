QT += core gui widgets
QT += dbus

include($$PWD/../Plugin.pri)

SOURCES += exampleplugin.cpp \
    examplevisualizer.cpp \
    examplemetavisualizer.cpp \
    thespheregenerator.cpp \
    rawimporter.cpp

HEADERS += exampleplugin.hpp \
    rawimporter.hpp \
    examplemetavisualizer.hpp \
    examplevisualizer.hpp \
    thespheregenerator.hpp
