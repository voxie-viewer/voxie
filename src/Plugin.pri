TARGET = Voxie$$basename(_PRO_FILE_PWD_)
TEMPLATE = lib
CONFIG += plugin

include($$PWD/CompilerFlags.pri)
include($$PWD/OpenCL.pri)
include($$PWD/LinkVoxie.pri)

OTHER_FILES += Voxie$$TARGET.json

unix {
    target.path = /usr/local/lib/voxie/plugins/
    INSTALLS += target
}
