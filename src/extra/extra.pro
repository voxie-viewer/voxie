# qmake file containing instructions for building and installing the manual and for installing the DBus interface XML file and the wrapper script

include($$PWD/../CompilerFlags.pri)

TEMPLATE = aux

# Needed for the wrapper script
CONFIG += nostrip

CONFIG += ignore_no_exist no_check_exist

unix {
    wrapper.path = $$VOXIE_TARGET_PATH
    wrapper.files = wrapper-script/voxie
    INSTALLS += wrapper

    wrapper-link.path = $$VOXIE_TARGET_PATH/../../bin
    wrapper-link.extra = ln -f -s ../lib/voxie/voxie $(INSTALL_ROOT)$$VOXIE_TARGET_PATH/../../bin/voxie
    INSTALLS += wrapper-link
}


dbus-xml.path = $$VOXIE_TARGET_PATH
dbus-xml.files = $$PWD/../../scripts/de.uni_stuttgart.Voxie.xml
INSTALLS += dbus-xml

example-scripts.path = $$VOXIE_TARGET_PATH/scripts
example-scripts.files = $$PWD/../../scripts/Example*.js $$PWD/../../scripts/voxie.py $$PWD/../../scripts/getAverage.py
example-scripts.CONFIG += executable
INSTALLS += example-scripts

dbusproxy-voxie-scripting.path = $$VOXIE_TARGET_PATH/dbusproxy/Voxie/scripting
dbusproxy-voxie-scripting.files = ../Voxie/scripting/dbustypes.hpp ../Voxie/scripting/dbustypes.cpp
dbusproxy-main.path = $$VOXIE_TARGET_PATH/dbusproxy/Main
dbusproxy-main.files = ../Main/dbusproxies.hpp ../Main/dbusproxies.cpp
INSTALLS += dbusproxy-voxie-scripting dbusproxy-main

unix {
    system(pdflatex --version > /dev/null 2>&1) {
        manual.target = manual.pdf
        manual.depends = $$PWD/../../doc/manual/*.tex $$PWD/../../doc/manual/img/* $$PWD/../../doc/manual/img/*/*
        manual.commands = rm -rf doc-build && mkdir doc-build && cp -R $$PWD/../../doc/manual/*.tex $$PWD/../../doc/manual/img doc-build/ && cd doc-build && : | pdflatex manual.tex > manual.1.log && : | pdflatex manual.tex > manual.2.log && : | pdflatex manual.tex > manual.3.log && cp manual.pdf ..
        QMAKE_EXTRA_TARGETS = manual
        ALL_DEPS += manual.pdf

        manual-install.path = $$VOXIE_TARGET_PATH/doc
        manual-install.files = $$OUT_PWD/manual.pdf
        manual-install.depends += manual
        manual-install.CONFIG += no_check_exist
        INSTALLS += manual-install
    }
}
