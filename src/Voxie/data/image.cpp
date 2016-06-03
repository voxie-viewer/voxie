#include "image.hpp"

using namespace voxie::data;
using namespace voxie::scripting;

Image::Image(quint64 width, quint64 height) : ScriptingContainer ("Image", nullptr, false, true), image_ (width, height, true) {
}
Image::~Image() {
}

voxie::scripting::Array2Info Image::GetDataReadonly () {
    try {
        Array2Info info;

        image().getBuffer().getSharedMemory()->getHandle(false, info.handle);
        info.offset = 0;

        info.dataType = "float";
        info.dataTypeSize = sizeof (float) * 8;
        if (info.dataTypeSize == 1) {
            info.byteorder = "none";
        } else {
            if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
                info.byteorder = "big";
            } else if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
                info.byteorder = "little";
            } else {
                info.byteorder = "unknown";
            }
        }

        info.sizeX = width ();
        info.sizeY = height ();
        info.strideX = sizeof (float);
        info.strideY = info.strideX * info.sizeX;

        return info;
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(this);
        return voxie::scripting::Array2Info();
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
