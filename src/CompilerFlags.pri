win32-msvc* {
    QMAKE_CXXFLAGS *= /FS /MP # /Ox /GL
	DEFINES += NOMINMAX
}

DEFINES += _USE_MATH_DEFINES

CONFIG += c++11

CONFIG += no_include_pwd

INCLUDEPATH += $$PWD

unix:LIBS += $$QMAKE_LIBS_DYNLOAD

*clang* {
	QMAKE_CXXFLAGS_WARN_ON += -Wextra
        QMAKE_CXXFLAGS *= -O1
}

# Hide internal symbols of plugins (and VoxieCore)
unix: QMAKE_CXXFLAGS += -fvisibility=hidden
unix: QMAKE_CFLAGS += -fvisibility=hidden
# To show symbols exported by plugins:
# ls build/*/*.so | grep -v VoxieCore | while read a; do echo "=== $a ==="; nm -CD "$a" --defined-only; echo; echo; done
# To check for missing symbols:
# LD_BIND_NOW=1 ./voxie.sh

addresssanitizer {
	CONFIG += debug
	QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1
	QMAKE_CFLAGS += -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1
	QMAKE_LFLAGS += -fsanitize=address -g
}

VOXIE_TARGET_PATH = /usr/local/lib/voxie
win32: VOXIE_TARGET_PATH = $$(ProgramFiles)/Voxie

exists(localconfig.pri) {
    include(localconfig.pri)
}
