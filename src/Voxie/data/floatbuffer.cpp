#include "floatbuffer.hpp"

FloatBuffer::FloatBuffer(size_t numElements, bool enableSharedMemory) {
    _numElements = numElements;
    if (!enableSharedMemory) {
        buffer_.reset (new float[numElements], deleteArray<float>);
        _values = buffer_.data();
    } else {
        sharedMemory_.reset (new voxie::data::SharedMemory (numElements * sizeof (float)));
        _values = (float*) sharedMemory_->getData ();
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
