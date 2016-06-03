#include "imagecomparator.hpp"

#include <Voxie/data/floatimage.hpp>

#include <Voxie/filtermask/rectangleData.hpp>

#include <QtCore/QDebug>

using namespace voxie::filter;
using namespace voxie::opencl;
using namespace voxie::data;

void ImageComparator::compareImage(FloatImage sourceImage, FloatImage filterImage,
                                           QRectF area, Selection2DMask* mask)
{
    if(mask->isEmpty()){
        return;
    }

    mask->getLock().lock();
    try{
        // obtain clinstance
        CLInstance* instance = sourceImage.getCLInstance();
        if(instance == nullptr){
            instance = filterImage.getCLInstance();
        } else {
            if(filterImage.getCLInstance() != instance){
                // either in stdmemory or incompatible clinstances -> switch to stdMemory
                filterImage.switchMode(FloatImage::STDMEMORY_MODE);
            }
        }
        if(instance == nullptr){
            instance = CLInstance::getDefaultInstance();
        }
        // instance obtained

        QString progID = "imagecomp";
        if(!instance->hasProgramID(progID)){
            instance->createProgramFromFile(":/cl_kernels/imageComparator.cl","",progID);
        }
        cl::Kernel kernel = instance->getKernel(progID, "imageComparator");

        // try to swicth to CLMEMORY_MODE, may throw
        sourceImage.switchMode(FloatImage::CLMEMORY_MODE, instance);
        filterImage.switchMode(FloatImage::CLMEMORY_MODE, instance);

        cl::Buffer rectangle = mask->getRectangleBuffer(instance);
        cl::Buffer ellipse = mask->getEllipseBuffer(instance);
        cl::Buffer polygon = mask->getPolygonBuffer(instance);
        cl::Buffer polygonOffset = mask->getPolygonBufferOffset(instance);
        cl_float relX = area.width() / sourceImage.getWidth();
        cl_float relY = area.height() / sourceImage.getHeight();

        kernel.setArg(0, sourceImage.getCLBuffer());
        kernel.setArg(1, filterImage.getCLBuffer());
        kernel.setArg(2, rectangle);
        kernel.setArg<cl_int>(3, mask->getRectangleSize());
        kernel.setArg(4, ellipse);
        kernel.setArg<cl_int>(5, mask->getEllipseSize());
        kernel.setArg(6, polygonOffset);
        kernel.setArg<cl_int>(7, mask->getPolygonSize());
        kernel.setArg(8, polygon);
        kernel.setArg(9, opencl::clVec4f(area.topLeft().x(), area.topLeft().y(), relX, relY));

        instance->executeKernel(kernel, cl::NDRange(sourceImage.getWidth(), sourceImage.getHeight()));
    } catch(CLException &ex){
        qWarning() << ex;
        qWarning() << "cl failed -> cpu fallback";
        compareImageCPU(sourceImage, filterImage, area, mask);
    } catch(opencl::IOException &ex){
        qWarning() << ex;
        qWarning() << "cl failed -> cpu fallback";
        compareImageCPU(sourceImage, filterImage, area, mask);
    }

    mask->getLock().unlock();
}

void ImageComparator::compareImageCPU(FloatImage sourceImage, FloatImage filterImage, QRectF area, Selection2DMask* mask)
{
    if(mask->isEmpty()){
        return;
    }
    mask->getLock().lock();
    for(size_t x = 0; x < sourceImage.getWidth(); x++)
    {
        for(size_t y = 0; y < sourceImage.getHeight();y++)
        {
            qreal relX =  area.width() / sourceImage.getWidth();
            qreal relY = area.height() / sourceImage.getHeight();
            qreal planeX = (x * relX) + area.topLeft().x();
            qreal planeY = (y * relY) + area.topLeft().y();

            if(!mask->isPointIn(QPointF(planeX, planeY))){
                filterImage.setPixel(x, y, sourceImage.getPixel(x, y));
            }
        }
    }
    mask->getLock().unlock();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
