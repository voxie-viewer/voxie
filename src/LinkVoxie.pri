win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Voxie/release/
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Voxie/debug/

unix:LIBS += -L$$OUT_PWD/../Voxie

LIBS += -lVoxie

DEPENDPATH += $$PWD/../Voxie
