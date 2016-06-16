TARGET = OpenCL
TEMPLATE = lib

CONFIG -= qt

OCLHDRDIR = $$PWD/../../../lib/opencl-headers-2.1
ICDSRCDIR = $$PWD/../../../lib/opencl-icd-loader

INCLUDEPATH += $$OCLHDRDIR

VERSION = 1

unix {
    QMAKE_CFLAGS += -Wno-deprecated-declarations
}

SOURCES += \
    $$ICDSRCDIR/icd.c \
    $$ICDSRCDIR/icd_dispatch.c
unix: SOURCES += $$ICDSRCDIR/icd_linux.c
windows: SOURCES += $$ICDSRCDIR/icd_windows.c

unix {
    LIBS += -ldl
    QMAKE_LFLAGS += -pthread -Wl,--version-script,$$ICDSRCDIR/icd_exports.map
}

windows {
    CONFIG += skip_target_version_ext
    LIBS += -ladvapi32
    DEF_FILE += $$ICDSRCDIR/OpenCL.def
    RC_FILE += $$ICDSRCDIR/OpenCL.rc
}
