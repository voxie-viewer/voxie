QT += core gui widgets
QT += dbus

include($$PWD/../Plugin.pri)

SOURCES += \
    filterplugin.cpp \
    gaussfilter2d.cpp \
    normalizefilter2d.cpp \
    boxblur3d.cpp \
    valuelimiter3d.cpp

HEADERS += \
    filterplugin.hpp \
    gaussfilter2d.hpp \
    normalizefilter2d.hpp \
    boxblur3d.hpp \
    valuelimiter3d.hpp
