QT += core gui script opengl
QT += dbus
QT += widgets

include($$PWD/../CompilerFlags.pri)

TARGET = ExtLoadVgi
TEMPLATE = app

target.path = $$VOXIE_TARGET_PATH/scripts
INSTALLS += target

HEADERS += \
    ../Voxie/scripting/dbustypes.hpp \
    ../Main/dbusproxies.hpp

SOURCES += \
    ../Voxie/scripting/dbustypes.cpp \
    ../Main/dbusproxies.cpp \
    loadvgi.cpp

CONFFILE = $${TARGET}.conf
win32:CONFIG(release, debug|release): CONFFILE = release/$$CONFFILE
else:win32: CONFFILE = debug/$$CONFFILE

conf.target = $$CONFFILE
conf.depends = $$PWD/$${TARGET}.conf

conf.commands = cp $$PWD/$${TARGET}.conf $$CONFFILE
win32: conf.commands = copy $$shell_path($$PWD/$${TARGET}.conf) $$shell_path($$CONFFILE)

QMAKE_EXTRA_TARGETS = conf
ALL_DEPS += $$CONFFILE

conf-install.path = $$VOXIE_TARGET_PATH/scripts
conf-install.files = $$PWD/$${TARGET}.conf
INSTALLS += conf-install
