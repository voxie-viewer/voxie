QT += core gui script opengl
QT += dbus
QT += widgets

include($$PWD/../CompilerFlags.pri)
include($$PWD/../OpenCL.pri)
include($$PWD/../LinkVoxie.pri)

TARGET = voxie
TEMPLATE = app

target.path = $$VOXIE_TARGET_PATH/bin
INSTALLS += target

# get git revision in c++
GIT_VERSION = $$system(git --git-dir $$PWD/../../.git --work-tree $$PWD describe --always --tags)
DEFINES += GIT_VERSION=\\\"$$GIT_VERSION\\\"

DEFINES += VOXIE_MAIN

HEADERS += \
    dbusproxies.hpp \
    directorymanager.hpp \
    gui/corewindow.hpp \
    gui/scriptconsole.hpp \
    root.hpp \
    gui/sliceview.hpp \
    gui/planeview.hpp \
    gui/dataselectiondialog.hpp \
    gui/buttonlabel.hpp \
    gui/vscrollarea.hpp \
    gui/preferences/openclpreferences.hpp \
    gui/visualizercontainer.hpp \
    gui/pluginmanagerwindow.hpp \
    gui/datasetview.hpp \
    gui/scriptlineedit.hpp \
    gui/preferenceswindow.hpp \
    gui/preferences/scriptpreferences.hpp \
    gui/aboutdialogwindow.hpp \
    gui/about/licensetab.hpp \
    gui/about/informationtab.hpp \
    gui/about/thirdpartytab.hpp \
    metatyperegistration.hpp \
    scriptwrapper.hpp

SOURCES += \
    dbusproxies.cpp \
    directorymanager.cpp \
    gui/corewindow.cpp \
    gui/scriptconsole.cpp \
    root.cpp \
    main.cpp \
    gui/datasetview.cpp \
    gui/sliceview.cpp \
    gui/planeview.cpp \
    gui/dataselectiondialog.cpp \
    gui/preferenceswindow.cpp \
    gui/preferences/scriptpreferences.cpp \
    gui/scriptlineedit.cpp \
    gui/buttonlabel.cpp \
    gui/vscrollarea.cpp \
    gui/preferences/openclpreferences.cpp \
    gui/visualizercontainer.cpp \
    gui/pluginmanagerwindow.cpp \
    gui/aboutdialogwindow.cpp \
    gui/about/licensetab.cpp \
    gui/about/informationtab.cpp \
    gui/about/thirdpartytab.cpp \
    metatyperegistration.cpp \
    scriptwrapper.cpp

RESOURCES += \
    icons.qrc \
    cl_kernels.qrc
