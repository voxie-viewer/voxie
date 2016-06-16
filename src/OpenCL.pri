DEFINES += CL_USE_DEPRECATED_OPENCL_1_1_APIS

INCLUDEPATH += $$PWD/../lib/opencl-headers-2.1

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lib/OpenCLICDLoader/release/
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../lib/OpenCLICDLoader/debug/
unix:LIBS += -L$$OUT_PWD/../lib/OpenCLICDLoader

LIBS += -lOpenCL
