#include "voxeldata.hpp"

#include <Voxie/ivoxie.hpp>

#include <Voxie/data/image.hpp>
#include <Voxie/data/slice.hpp>

#include <Voxie/opencl/clinstance.hpp>

#include <QtCore/QRunnable>
#include <QtCore/QThreadPool>
#include <QtCore/QUuid>

using namespace voxie::data;
using namespace voxie::data::internal;

class TransformWorker :
        public QRunnable
{
private:
    QSharedPointer<VoxelData> dataSet;
    size_t slice;
    const std::function<Voxel(Voxel)> &f;
public:
    TransformWorker(const QSharedPointer<VoxelData>& dataSet, size_t slice, const std::function<Voxel(Voxel)> &f) :
        QRunnable(),
        dataSet(dataSet),
        slice(slice),
        f(f)
    {
        this->setAutoDelete(true);
    }

    virtual void run() override
    {
        auto data = this->dataSet.data();
        for(size_t y = 0; y < data->getDimensions().y; y++)
        {
            for(size_t x = 0; x < data->getDimensions().x; x++)
            {
                data->setVoxel(x, y, this->slice, this->f(data->getVoxel(x, y, this->slice)));
            }
        }
    }
};

class CoordinatedTransformWorker :
        public QRunnable
{
private:
    QSharedPointer<VoxelData> dataSet;
    size_t slice;
    const std::function<Voxel(size_t,size_t,size_t,Voxel)> &f;
public:
    CoordinatedTransformWorker(const QSharedPointer<VoxelData>& dataSet, size_t slice, const std::function<Voxel(size_t,size_t,size_t,Voxel)> &f) :
        QRunnable(),
        dataSet(dataSet),
        slice(slice),
        f(f)
    {
        this->setAutoDelete(true);
    }

    virtual void run() override
    {
        auto data = this->dataSet.data();
        for(size_t y = 0; y < data->getDimensions().y; y++)
        {
            for(size_t x = 0; x < data->getDimensions().x; x++)
            {
                data->setVoxel(x, y, this->slice, this->f(x, y, this->slice, data->getVoxel(x, y, this->slice)));
            }
        }
    }
};

VoxelData::VoxelData(size_t width, size_t height, size_t depth) :
    voxie::scripting::ScriptingContainer("VoxelData"),
	//data(nullptr),
    dataSH(width*height*depth*sizeof(Voxel)),
    size(width * height * depth),
	dimensions(width, height, depth),
    dimensionsMetric(width, height, depth),
	spacing(1.0f, 1.0f, 1.0f),
	firstVoxelPos(0.0f, 0.0f, 0.0f)
{
    new VoxelDataAdaptor(this);

	//this->data = new Voxel[width*height*depth];
    connect(this, &VoxelData::changed, this, &VoxelData::invalidate);
}

QSharedPointer<VoxelData> VoxelData::create(size_t width, size_t height, size_t depth) {
    QSharedPointer<VoxelData> data(new VoxelData(width, height, depth), [](QObject* obj) { obj->deleteLater(); });
    data->thisPointerWeak = data;
    ScriptingContainer::registerObject(data);
    return data;
}

VoxelData::~VoxelData()
{
  //delete[] this->data;
	// slices delete themselves
}

Voxel VoxelData::getVoxelMetric(qreal x, qreal y, qreal z, InterpolationMethod method) const
{
	// transform to 'integer' coordinatesystem
	x -= getFirstVoxelPosition().x();
	y -= getFirstVoxelPosition().y();
	z -= getFirstVoxelPosition().z();
	x /= this->spacing.x();
	y /= this->spacing.y();
	z /= this->spacing.z();

	if( x < 0 || y < 0 || z < 0 ) {
		return NAN;
	}

	if (method == nearestNeighbor) {
		return getVoxelSafe((size_t)(x+0.5), (size_t)(y+0.5), (size_t)(z+0.5));
	}
	if (method == linear) {
		//coefficients
		qreal kx = x - (size_t)x;
		qreal ky = y - (size_t)y;
		qreal kz = z - (size_t)z;
        //qDebug() << kx << ky << kz;
		Voxel vox =	 kx*ky*kz*			   getVoxelSafe(x+1,y+1,z+1)
				+	   kx*ky*(1-kz)*		   getVoxelSafe(x+1,y+1,z)
				+	   kx*(1-ky)*kz*		   getVoxelSafe(x+1,y,z+1)
				+	   kx*(1-ky)*(1-kz)*	   getVoxelSafe(x+1,y,z)
				+	   (1-kx)*ky*kz*		   getVoxelSafe(x,y+1,z+1)
				+	   (1-kx)*ky*(1-kz)*	   getVoxelSafe(x,y+1,z)
				+	   (1-kx)*(1-ky)*kz*	   getVoxelSafe(x,y,z+1)
				+	   (1-kx)*(1-ky)*(1-kz)*   getVoxelSafe(x,y,z);
		return vox;
	}
	return NAN;
}

QSharedPointer<VoxelData> VoxelData::clone() const
{
    QSharedPointer<VoxelData> clone = create(this->dimensions.x, this->dimensions.y, this->dimensions.z);
    memcpy(clone->getData(), this->getData(), this->getByteSize());
    clone->spacing = this->spacing;
    clone->firstVoxelPos = this->firstVoxelPos;
    clone->dimensionsMetric = this->dimensionsMetric;
    return clone;
}


QSharedPointer<VoxelData>
VoxelData::reducedSize(uint xStepsize, uint yStepsize, uint zStepsize) const
{
    xStepsize = xStepsize == 0 ? 1:xStepsize;
    yStepsize = yStepsize == 0 ? 1:yStepsize;
    zStepsize = zStepsize == 0 ? 1:zStepsize;

    auto currentDims = this->dimensions;
    voxie::scripting::IntVector3 newDims(
                (currentDims.x + xStepsize-1)/xStepsize,
                (currentDims.y + yStepsize-1)/yStepsize,
                (currentDims.z + zStepsize-1)/zStepsize);

    auto newVoxelData = create(newDims.x,newDims.y,newDims.z);

    QVector3D spacing = this->spacing * QVector3D(xStepsize, yStepsize, zStepsize);
    newVoxelData->setSpacing(spacing);
    newVoxelData->setFirstVoxelPos(this->getFirstVoxelPosition());

    for(size_t z = 0; z < newDims.z; z++){
        for(size_t y = 0; y < newDims.y; y++){
            for(size_t x = 0; x < newDims.x; x++){
                Voxel vox = this->getVoxel(x*xStepsize, y*yStepsize, z*zStepsize);
                newVoxelData->setVoxel(x,y,z, vox);
            }
        }
    }

    return newVoxelData;
}


cl::Image3D &VoxelData::getCLImage()
{
    if (!this->isImageInit) {
        try{
            this->clImage = voxie::opencl::CLInstance::getDefaultInstance()->createImage3D(
					cl::ImageFormat(CL_R, CL_FLOAT),
					this->dimensions.x,
					this->dimensions.y,
					this->dimensions.z,
					this->getData());
        } catch (voxie::opencl::CLException& ex){
            this->clImage = cl::Image3D();
            qWarning() << "Error when allocing OpenCL image for voxel data:" << ex;
        }
        // Set isImageInit even if allocating the image failed. In this case,
        // clImage is null and there will be no further attempt to allocate
        // the image.
        this->isImageInit = this->clImageValid = true;
	}
    if(!this->clImageValid){
        updateClImage();
    }
	return this->clImage;
}

void VoxelData::updateClImage()
{
    if(isImageInit && clImage() != nullptr) {
        try {
            voxie::opencl::CLInstance::getDefaultInstance()->fillImage(&this->clImage, this->getData());
            this->clImageValid = true;
            //qDebug() << "climage update";
        } catch(voxie::opencl::CLException& ex){
            qWarning() << ex;
        }
    }
}

void VoxelData::transform(const std::function<Voxel(Voxel)> &f)
{
    QSharedPointer<VoxelData> thisPointer(thisPointerWeak);
    if (!thisPointer) {
        qCritical() << "VoxelData::transform() called before constructor has finished or during destruction";
        return;
    }
    QThreadPool pool;
    for(size_t z = 0; z < this->dimensions.z; z++)
    {
        pool.start(new TransformWorker(thisPointer, z, f));
    }
    pool.waitForDone();
    this->invalidate();
}

void VoxelData::transformCoordinate(const std::function<Voxel(size_t,size_t,size_t,Voxel)> &f)
{
    QSharedPointer<VoxelData> thisPointer(thisPointerWeak);
    if (!thisPointer) {
        qCritical() << "VoxelData::transformCoordinate() called before constructor has finished or during destruction";
        return;
    }
    QThreadPool pool;
    for(size_t z = 0; z < this->dimensions.z; z++)
    {
        pool.start(new CoordinatedTransformWorker(thisPointer, z, f));
    }
    pool.waitForDone();
    this->invalidate();
}

voxie::scripting::Array3Info VoxelData::dataFd (bool rw) {
    voxie::scripting::Array3Info info;

    dataSH.getHandle (rw, info.handle);
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

    info.sizeX = dimensions.x;
    info.sizeY = dimensions.y;
    info.sizeZ = dimensions.z;
    info.strideX = sizeof (float);
    info.strideY = info.strideX * info.sizeX;
    info.strideZ = info.strideY * info.sizeY;

    return info;
}

void VoxelData::extractSlice(const QVector3D& origin, const QQuaternion& rotation, const QSize& outputSize, double pixelSizeX, double pixelSizeY, InterpolationMethod interpolation, FloatImage& outputImage) {
    Plane plane (origin, rotation);
    QRectF sliceArea (0, 0, outputSize.width() * pixelSizeX, outputSize.height() * pixelSizeY);
    
    if ((size_t) outputSize.width() > outputImage.getWidth() || (size_t) outputSize.height() > outputImage.getHeight())
        throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IndexOutOfRange", "Index is out of range");

    bool useCL = true;
    // When OpenCL is not available, fall back to CPU
    if (useCL)
        useCL = voxie::opencl::CLInstance::getDefaultInstance()->isValid();
    // When OpenCL is available but allocating the OpenCL image for the current
    // dataset failed, fall back to CPU
    if (useCL)
        useCL = getCLImage()() != nullptr;
    bool clFailed = false;
    if(useCL){
        using namespace voxie::opencl;
//        QTime timer;
        try {
//            timer.start();
            CLInstance* clInstance = CLInstance::getDefaultInstance();

            QString progId("slice_image_generator");
            if(!clInstance->hasProgramID(progId)){
                clInstance->createProgramFromFile(":/cl_kernels/sliceimagegenerator.cl", "", progId);
            }
            cl::Kernel kernel = clInstance->getKernel(progId, (interpolation == nearestNeighbor ? "extract_slice_nearest" : "extract_slice_linear") );

            cl::Image3D clvolume = this->getCLImage();
            size_t numElements = outputSize.width() * outputSize.height();
            outputImage.switchMode(FloatImage::CLMEMORY_MODE, false, clInstance);
            cl::Buffer clbuffer = outputImage.getCLBuffer();

            QVector3D origin = (plane.origin - this->getFirstVoxelPosition());

            //cl_uint error;
			cl_uint index = 0;
			//error =
			kernel.setArg(index++, clvolume);
			//error =
			kernel.setArg(index++, clbuffer);
			QVector3D volDims = this->getDimensionsMetric();
			//error =
            kernel.setArg(index++, clVec4f( volDims.x(), volDims.y(), volDims.z(), outputImage.getWidth() ));
			//error =
            kernel.setArg<cl_uint>(index++, outputImage.getWidth());
			//error =
            kernel.setArg(index++, qVec3_To_clVec3f( origin ));
			//error =
            kernel.setArg(index++, qVec3_To_clVec3f( plane.tangent() ));
			//error =
            kernel.setArg(index++, qVec3_To_clVec3f( plane.cotangent() ));
			//error =
            kernel.setArg(index++, qRectF_To_clVec4f( sliceArea ));
			//Q_UNUSED(error);

            clInstance->executeKernel(kernel, cl::NDRange(numElements));

//            int total = timer.elapsed();
//            qDebug() << "gpu time:" << total;

        } catch(CLException& ex){
            qWarning() << ex;
            clFailed = true;
        } catch(IOException& ex){
            qWarning() << ex;
            clFailed = true;
        }
    }

    if(!useCL || clFailed){
        //qDebug() << "creating slice on cpu";
        QTime timer;
        timer.start();
        if(outputImage.getMode() != SliceImage::STDMEMORY_MODE){
            outputImage.switchMode(false);//swicth mode without syncing memory
        }
        FloatBuffer buffer = outputImage.getBuffer();
        for(size_t y = 0; y < (size_t) outputSize.height(); y++){
            for(size_t x = 0; x < (size_t) outputSize.width(); x++){
                QPointF planePoint;
                SliceImage::imagePoint2PlanePoint(x, y, outputSize, sliceArea, planePoint, false);
                //buffer[y*outputImage.getWidth() + x] = this->getPixelValue(p.x(), p.y(), interpolation);
                QVector3D volumePoint = plane.get3DPoint(planePoint.x(), planePoint.y());
                buffer[y*outputImage.getWidth() + x] = this->getVoxelMetric(volumePoint.x(), volumePoint.y(), volumePoint.z(), interpolation);
            }
        }
        //int total = timer.elapsed();
        //qDebug() << "cpu Time:" << total;
    }
}

QVector3D VoxelDataAdaptor::origin() {
    return object->getFirstVoxelPosition();
}
void VoxelDataAdaptor::setOrigin(QVector3D value) {
    object->setFirstVoxelPos(value);
}

QVector3D VoxelDataAdaptor::spacing() {
    return object->getSpacing();
}

voxie::scripting::IntVector3 VoxelDataAdaptor::size() {
    return object->getDimensions();
}

void VoxelDataAdaptor::UpdateFromBuffer(const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptingContainerBase::checkOptions(options);
        emit object->changed();
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
    }
}

voxie::scripting::Array3Info VoxelDataAdaptor::GetDataReadonly () {
    try {
        return object->dataFd (false);
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::Array3Info();
    }
}
voxie::scripting::Array3Info VoxelDataAdaptor::GetDataWritable () {
    try {
        return object->dataFd (true);
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::Array3Info();
    }
}

void VoxelDataAdaptor::ExtractSlice (const QVector3D& origin, const QQuaternion& rotation, const voxie::scripting::IntVector2& outputSize, const QVector2D& pixelSize, QDBusObjectPath outputImage, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptingContainerBase::checkOptions(options, "Interpolation");

        InterpolationMethod interpolation = linear;
        auto interpolationVal = options.find ("Interpolation");
        if (interpolationVal != options.end ()) {
            if (*interpolationVal == "NearestNeighbor")
                interpolation = nearestNeighbor;
            else if (*interpolationVal == "Linear")
                interpolation = linear;
            else
                throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.InvalidOptionValue", "Invalid value for 'Interpolation' option");
        }

        QSharedPointer<voxie::scripting::ScriptingContainer> obj = voxie::scripting::ScriptingContainer::lookupObject(outputImage);
        if (!obj)
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.ObjectNotFound", "Object " + outputImage.path() + " not found");
        auto image = qSharedPointerCast<Image> (obj);
        if (!image)
            throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.InvalidObjectType", "Object " + outputImage.path() + " is not an image");
    
        object->extractSlice (origin, rotation, QSize (outputSize.x, outputSize.y), pixelSize.x(), pixelSize.y(), interpolation, image->image());
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
