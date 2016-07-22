QT += core gui widgets
QT += dbus

include($$PWD/../Plugin.pri)

exists(localconfig.pri) {
    include(localconfig.pri)
}

INCLUDEPATH += $$PWD

unix: INCLUDEPATH += /usr/lib/openmpi/include/

win32 {
    isEmpty(HDF5_PATH):HDF5_PATH = $$quote("C:/Program Files/HDF_Group/HDF5/1.8.13")
    #isEmpty(HDF5_PATH):HDF5_PATH = $$quote("C:/Program Files/HDF_Group/HDF5/1.1.10")

    isEmpty(BOOST_PATH):BOOST_PATH = $$quote("C:/Program Files/boost")
}

CONFIG += link_pkgconfig

HAVE_HDF5 =
!isEmpty(HDF5_PATH) {
    INCLUDEPATH += $$HDF5_PATH/include
    LIBS += -L$$HDF5_PATH/lib
    HAVE_HDF5 = 1
}
isEmpty(HAVE_HDF5) {
    packagesExist(hdf5) {
        PKGCONFIG += hdf5
        HAVE_HDF5 = 1
    }
}
isEmpty(HAVE_HDF5) {
    packagesExist(hdf5-serial) {
        PKGCONFIG += hdf5-serial
        HAVE_HDF5 = 1
    }
}
isEmpty(HAVE_HDF5) {
    packagesExist(hdf5-openmpi) {
        PKGCONFIG += hdf5-openmpi
        HAVE_HDF5 = 1
    }
}
LIBS += -lhdf5
DEFINES += H5_BUILT_AS_DYNAMIC_LIB

!isEmpty(BOOST_PATH) {
    INCLUDEPATH += $$BOOST_PATH
}

win32: LIBS += -limagehlp

DEFINES += NO_BOOST_FILESYSTEM_PATH # Avoid dependency on BoostFilesystem

DISTFILES += \
    localconfig.pri

SOURCES += hdf5plugin.cpp \
    hdfloader.cpp \
    hdfexporter.cpp \
    hdfsliceexporter.cpp \
    Core/Assert.cpp Core/CheckedCast.cpp Core/Exception.cpp Core/HasMemberCalled.cpp Core/Memory.cpp Core/NumericException.cpp Core/Reflection.cpp Core/Type.cpp CT/DataFiles.cpp HDF5/AtomicType.cpp HDF5/AtomicTypes.cpp HDF5/Attribute.cpp HDF5/BaseTypes.cpp HDF5/ComplexConversion.cpp HDF5/CompoundType.cpp HDF5/DataSet.cpp HDF5/DataSpace.cpp HDF5/DataType.cpp HDF5/DataTypes.cpp HDF5/DelayedArray.cpp HDF5/File.cpp HDF5/Group.cpp HDF5/HDF5_Array.cpp HDF5/HDF5_Exception.cpp HDF5/HDF5_Type.cpp HDF5/HDF5_Vector3.cpp HDF5/IdComponent.cpp HDF5/Matlab.cpp HDF5/MatlabDiagMatrix3.cpp HDF5/MatlabVector2.cpp HDF5/MatlabVector3.cpp HDF5/Object.cpp HDF5/OpaqueType.cpp HDF5/PropList.cpp HDF5/PropLists.cpp HDF5/ReferenceType.cpp HDF5/Serialization.cpp HDF5/SerializationKey.cpp HDF5/StdVectorSerialization.cpp HDF5/Util.cpp Math/Abs.cpp Math/Array.cpp Math/DiagMatrix3.cpp Math/Float.cpp Math/Math.cpp Math/Vector2.cpp Math/Vector3.cpp \
    Core/StrError.c

HEADERS += hdf5plugin.hpp \
    hdfloader.hpp \
    hdfexporter.hpp \
    hdfsliceexporter.hpp \
    Core/Assert.hpp Core/CheckedCast.hpp Core/Exception.hpp Core/HasMemberCalled.hpp Core/Memory.hpp Core/NumericException.hpp Core/Reflection.hpp Core/Type.hpp Core/Util.hpp CT/DataFiles.hpp HDF5/Array.hpp HDF5/AtomicType.hpp HDF5/AtomicTypes.hpp HDF5/Attribute.hpp HDF5/BaseTypes.hpp HDF5/ComplexConversion.hpp HDF5/CompoundType.hpp HDF5/DataSet.hpp HDF5/DataSpace.hpp HDF5/DataType.hpp HDF5/DataTypes.hpp HDF5/DelayedArray.hpp HDF5/Exception.hpp HDF5/File.hpp HDF5/Forward.hpp HDF5/Group.hpp HDF5/IdComponent.hpp HDF5/MatlabDiagMatrix3.hpp HDF5/Matlab.hpp HDF5/MatlabVector2.hpp HDF5/MatlabVector3.hpp HDF5/Object.hpp HDF5/OpaqueType.hpp HDF5/PropList.hpp HDF5/PropLists.hpp HDF5/ReferenceType.hpp HDF5/Serialization.forward.hpp HDF5/Serialization.hpp HDF5/SerializationKey.hpp HDF5/StdVectorSerialization.hpp HDF5/Type.hpp HDF5/Util.hpp HDF5/Vector3.hpp Math/Abs.hpp Math/Array.hpp Math/DiagMatrix3.hpp Math/Float.hpp Math/Forward.hpp Math/Math.hpp Math/Vector2.hpp Math/Vector3.hpp \
    Core/StrError.h Core/Util.h
