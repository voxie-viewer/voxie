QT += core gui script opengl
QT += dbus
QT += widgets

include($$PWD/../CompilerFlags.pri)

TARGET = ScriptGetAverage
TEMPLATE = app

target.path = $$VOXIE_TARGET_PATH/scripts
INSTALLS += target

HEADERS += \
    ../Voxie/scripting/dbustypes.hpp \
    ../Main/dbusproxies.hpp

SOURCES += \
    ../Voxie/scripting/dbustypes.cpp \
    ../Main/dbusproxies.cpp \
    getaverage.cpp
