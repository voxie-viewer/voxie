#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/data/sharedmemory.hpp>

#include <cstring>

#include <QtCore/QDebug>
#include <QtCore/QSharedPointer>
#include <QtCore/QVector>

class VOXIECORESHARED_EXPORT FloatBuffer
{
public:
    // throws ScriptingException if enableSharedMemory is true
    FloatBuffer(size_t numElements, bool enableSharedMemory);

    inline size_t numElements() const {return this->_numElements;}
    inline size_t length() const {return this->_numElements;}
    inline size_t size() const {return this->_numElements;}
    inline size_t byteSize() const {return this->_numElements*sizeof(float);}
    inline bool isEmpty() const {return this->_numElements == 0;}

    inline const float* data() const {return this->_values;}
    inline float* data() {return this->_values;}
    inline float at(size_t index) const {Q_ASSERT(index < _numElements); return this->data()[index];}
    inline void setElement(size_t index, float element){Q_ASSERT(index < _numElements); this->data()[index] = element;}
    inline float& operator[](size_t i) {Q_ASSERT(i < _numElements); return this->data()[i];}
    inline const float& operator[](size_t i) const {Q_ASSERT(i < _numElements); return this->data()[i];}

    voxie::data::SharedMemory* getSharedMemory() { return sharedMemory_.data(); }

    inline QVector<float> toQVector() const
        {
            QVector<float> vec((int)this->numElements()); // QVector maximum size is int
            if(this->_numElements > 0)
                memcpy(vec.data(), this->data(), this->byteSize());
            return vec;
        }
    // throws ScriptingException if enableSharedMemory is true
    inline FloatBuffer copy(bool enableSharedMemory) const
        {
            FloatBuffer bufferCopy(this->_numElements, enableSharedMemory);
            if(this->_numElements > 0)
                memcpy(bufferCopy.data(), this->data(), this->byteSize());
            return bufferCopy;
        }

private:
    float* _values;
    QSharedPointer<float> buffer_;
    QSharedPointer<voxie::data::SharedMemory> sharedMemory_;
    size_t _numElements;

    template <typename T_>
    static void deleteArray(T_ array[])
    {
        delete[] array;
    }
};


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
