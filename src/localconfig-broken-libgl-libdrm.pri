# For some versions of the amdgpu driver the compiler option -ldrm is needed for compiling OpenGL programs
equals(TEMPLATE, "app") {
    unix {
        LIBS += -ldrm
    }
}
