#include "floatimage.hpp"

#include <string.h>

#include <QtGui/QColor>
#include <QtGui/QRgb>

using namespace voxie::data;
using namespace voxie;

FloatImage::FloatImage(size_t width, size_t height, bool enableSharedMemory) :
    imageData(new FloatImageData(width, height, enableSharedMemory))
{
}

FloatImage::FloatImage() :
    imageData(new FloatImageData(0,0,false))
{
}

size_t FloatImage::getWidth() const
{
    return this->imageData->width;
}

size_t FloatImage::getHeight() const
{
    return this->imageData->height;
}

QSize FloatImage::getDimension() const
{
    return QSize((int)this->getWidth(), (int)this->getHeight());
}

FloatBuffer FloatImage::getBufferCopy(bool *insync) const
{
        FloatBuffer buffer = this->imageData->pixels.copy(false);
        if(this->getMode() == CLMEMORY_MODE){
            // fetch from clBuffer
            try{
                this->imageData->clInstance->readBuffer(this->imageData->clPixels, buffer.data());
                if(insync != nullptr)
                    *insync = true;
            } catch(opencl::CLException& ex){
                if(insync != nullptr)
                    *insync = false;
                qWarning() << ex;
            }
        } else {
            if(insync != nullptr)
                *insync = true;
        }
        return buffer;
}

FloatBuffer &FloatImage::getBuffer()
{
    if(this->getMode() == CLMEMORY_MODE){
        qWarning() << "Image is in CLMEMORY_MODE - Buffer with write access may be overwritten on mode switch";
    }
    return this->imageData->pixels;
}
const FloatBuffer& FloatImage::getBuffer() const
{
    if(this->getMode() == CLMEMORY_MODE){
        qWarning() << "Image is in CLMEMORY_MODE - Buffer with write access may be overwritten on mode switch";
    }
    return this->imageData->pixels;
}


FloatImage::MemoryMode
FloatImage::switchMode(bool syncMemory, opencl::CLInstance* clInstance) EXCEPT
{
    if(this->getMode() == STDMEMORY_MODE){
        // switch to CL
        this->imageData->clInstance = (clInstance == nullptr ? opencl::CLInstance::getDefaultInstance() : clInstance);
        try{
            this->imageData->clPixels = this->imageData->clInstance->createBuffer(this->imageData->pixels.byteSize(), (syncMemory ? this->imageData->pixels.data() : nullptr) );
            this->imageData->mode = CLMEMORY_MODE;
        } catch(opencl::CLException& ex){
            qWarning() << "Cannot switch mode" << ex;
            this->imageData->clInstance = nullptr;
            ex.raise(); // raise exception to next lvl
        }

    } else {
        // switch to STD
        if(syncMemory){
            try{
                this->getCLInstance()->readBuffer(this->imageData->clPixels, this->imageData->pixels.data());
            } catch(opencl::CLException& ex){
                qWarning() << "Cannot sync buffers" << ex;
                //dont throw, this must always succeed//ex.raise();
            }
        }
        this->imageData->clPixels = cl::Buffer(); // reset clbuffer to free memory on device
        this->imageData->clInstance = nullptr;
        this->imageData->mode = STDMEMORY_MODE;
    }

    return this->getMode();
}

cl::Buffer
FloatImage::getCLBufferCopy() const EXCEPT
{
    if(this->imageData->mode == STDMEMORY_MODE){
        return cl::Buffer();
    } else {
        Q_ASSERT(this->imageData->clInstance != nullptr);
        return this->imageData->clInstance->copyBuffer(this->imageData->clPixels);
    }
}

cl::Buffer& FloatImage::getCLBuffer()
{
    return this->imageData->clPixels;
}
const cl::Buffer& FloatImage::getCLBuffer() const
{
    return this->imageData->clPixels;
}

QImage
FloatImage::toQImage(float lowestValue, float highestValue) const
{
    Colorizer* colorizer = new Colorizer();
    colorizer->putMapping(lowestValue, qRgb(0,0,0));
    colorizer->putMapping(highestValue, qRgb(255,255,255));
    QImage img = toQImage(colorizer);
    delete colorizer;
    return img;
}


QImage
FloatImage::toQImage(Colorizer* colorizer) const
{
    if(colorizer == nullptr || colorizer->getNumMappings() < 1){
        return toQImage();
    }

    QImage qimage(this->getDimension(), QImage::Format_ARGB32);
    QRgb* qimgbuffer = (QRgb*) qimage.bits();
    //
    bool oclFailed = false;
    if(this->imageData->mode == CLMEMORY_MODE){
        // ocl
        try{
            opencl::CLInstance* clInstance = this->imageData->clInstance;
            Q_ASSERT(clInstance != nullptr);

            QString progID("colorizer");
            if(!clInstance->hasProgramID(progID)){
                clInstance->createProgramFromFile(":/cl_kernels/colorizer.cl", "", progID);
            }

            cl::Kernel kernel = clInstance->getKernel(progID, "colorize");
            cl::Buffer qimgCLBuffer = clInstance->createBuffer(qimage.width()*qimage.height()*sizeof(QRgb), qimgbuffer);
            auto colorKeys = colorizer->getInputs();
            auto colorValues = colorizer->getOutputs();
            cl::Buffer colorKeysCL = clInstance->createBuffer(colorKeys.length() * sizeof(float), colorKeys.data());
            cl::Buffer colorValuesCL = clInstance->createBuffer(colorValues.length() * sizeof(QRgb), colorValues.data());

            //cl_int error;
            cl::Buffer tempBuffer = this->imageData->clPixels;
            // error =
            kernel.setArg(0, tempBuffer);
            //error =
            kernel.setArg(1, qimgCLBuffer);
            //error =
            kernel.setArg(2, colorKeysCL);
            //error =
            kernel.setArg(3, colorValuesCL);
            //error =
            kernel.setArg<cl_int>(4, (cl_int) colorKeys.length());
            // error =
            kernel.setArg<cl_uint>(5, (cl_uint) colorizer->getNanColor());
            //Q_UNUSED(error);

            clInstance->executeKernel( kernel, cl::NDRange(this->getWidth()*this->getHeight()) );
            // read back
            clInstance->readBuffer(qimgCLBuffer, qimgbuffer);

        } catch(opencl::CLException& ex){
            qWarning() << ex;
            oclFailed = true;
        } catch(opencl::IOException& ex){
            qWarning() << ex;
            oclFailed = true;
        }
    }
    if(oclFailed || this->getMode() == STDMEMORY_MODE) {
    //
    FloatBuffer buffer = this->getBufferCopy();
    for(size_t i = 0; i < buffer.length(); i++){
        float value = buffer[i];
        QRgb color = colorizer->getColor(value);
        qimgbuffer[i] = color;
    }
    }

    // Mirror y axis (the y axis of FloatImage goes from bottom to top, the y axis of QImage goes from top to bottom)
    for (std::size_t y = 0; y < (std::size_t) qimage.height() / 2; y++) {
        for (std::size_t x = 0; x < (std::size_t) qimage.width(); x++) {
            std::size_t y2 = qimage.height() - 1 - y;
            std::swap (qimgbuffer[y * qimage.width() + x], qimgbuffer[y2 * qimage.width() + x]);
        }
    }

    return qimage;
}


FloatImage
FloatImage::clone(bool enableSharedMemory) const
{
    FloatImage _clone;
    _clone.imageData->width = this->getWidth();
    _clone.imageData->height = this->getHeight();

    bool clFailed = false;
    if(this->getMode() == CLMEMORY_MODE){
        try{
            _clone.imageData->clPixels = this->getCLBufferCopy(); // might throw
            _clone.imageData->pixels = this->imageData->pixels.copy(enableSharedMemory); // not the same as getBufferCopy
            _clone.imageData->clInstance = this->imageData->clInstance;
            _clone.imageData->mode = CLMEMORY_MODE;
        } catch(opencl::CLException&){
            clFailed = true;
        }
    }
    if(this->getMode() == STDMEMORY_MODE || clFailed){
        _clone.imageData->pixels = this->getBufferCopy();
    }
    return _clone;
}


void
FloatImage::testColorizing()
{
    FloatImage img(100,100,false);
    for(size_t i = 0; i < 100; i++){for(size_t j = 0; j < 100; j++){
        img.setPixel(j,i, j == 50 ? NAN : ((i*100 +j) / 10000.0f));
    }}
    img.switchMode(CLMEMORY_MODE);

    Colorizer* colorizer = Colorizer::redPeakColorizer();

    img.toQImage(colorizer).save("blah.png");
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
