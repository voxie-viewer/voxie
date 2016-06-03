# Source taken from http://qt-project.org/forums/viewthread/188747

DEFINES += CL_USE_DEPRECATED_OPENCL_1_1_APIS

_AMDAPPSDK = $$(AMDAPPSDKROOT)
win32{
isEmpty(_AMDAPPSDK) {
    message(\"AMD APP SDK\" not detected...)
} else {
    message(\"AMD APP SDK\" detected...)
    DEFINES += _AMDAPPSDK
}
}

win32:contains(DEFINES , _AMDAPPSDK) {
    INCLUDEPATH += $$quote($$_AMDAPPSDK/include)
    LIBS += -L$$quote($$_AMDAPPSDK/lib/x86_64) -lOpenCL
    message(INCLUDEPATH = \"$$INCLUDEPATH\")
    message(LIBS = \"$$LIBS\")
}

_INTELOCLSDK = $$(INTELOCLSDKROOT)
win32 {
isEmpty(_INTELOCLSDK) {
    message(\"INTEL APP SDK\" not detected...)
} else {
    message(\"INTEL APP SDK\" detected...)
    DEFINES += _INTELOCLSDK
}
}

win32:contains(DEFINES , _INTELOCLSDK) {
    INCLUDEPATH += $$quote($$_INTELOCLSDK/include)
    LIBS += -L\"$$quote($$_INTELOCLSDK/lib/x64)\" -lOpenCL
    message(INCLUDEPATH = \"$$INCLUDEPATH\")
    message(LIBS = \"$$LIBS\")
}

# OpenCl for Unix
unix:contains(DEFINES , _AMDAPPSDK) {
    INCLUDEPATH += $$quote($$_AMDAPPSDK/include)
    LIBS += -L$$quote($$_AMDAPPSDK/lib/x86_64)
    message(INCLUDEPATH = \"$$INCLUDEPATH\")
    message(LIBS = \"$$LIBS\")
}
unix: LIBS += -lOpenCL
