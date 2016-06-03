#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/data/floatimage.hpp>
#include <Voxie/data/interpolationmethod.hpp>
#include <Voxie/data/sharedmemory.hpp>

#include <Voxie/opencl/clinstance.hpp>

#include <Voxie/scripting/scriptingcontainer.hpp>

#include <functional>

#include <inttypes.h>

#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QtGlobal>
#include <QtCore/QTime>
#include <QtCore/QVariant>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusUnixFileDescriptor>

#include <QtGui/QVector3D>

/**
 * @ingroup data
 * @author Andreas Korge
 * @version 1.0
 */

namespace voxie { namespace data {
    typedef float Voxel;

    //forward declaration
    class Slice;
    struct Plane;

/**
 * The VoxelData class is a 3 dimensional dataset (image) that contains float
 * values.
 * @brief The VoxelData class
 */
class VOXIECORESHARED_EXPORT VoxelData : public voxie::scripting::ScriptingContainer, public QDBusContext
{
    Q_OBJECT

    private:
        SharedMemory dataSH;
        size_t size;
        voxie::scripting::IntVector3 dimensions;
        QVector3D dimensionsMetric;
        QVector3D spacing;
        QVector3D firstVoxelPos;
        cl::Image3D clImage;
        bool isImageInit = false;
        QPair<Voxel,Voxel> minMax;
        bool minMaxValid = false;
        bool clImageValid = false;
        QFileInfo fileInfo;

        bool isInBounds(size_t x, size_t y, size_t z, voxie::scripting::IntVector3 dimensions) const
        {
            return x < dimensions.x &&
                   y < dimensions.y &&
                   z < dimensions.z;
        }

    public:
        VoxelData(size_t width, size_t height, size_t depth, QObject *parent);
        ~VoxelData();

        /**
         * @return voxel value at the given position. x,y,z outside the dimensions will cause a crash.
         */
        inline Voxel getVoxel(size_t x, size_t y, size_t z) const
        {
            return this->getData()[x + y * this->dimensions.x + z * this->dimensions.x * this->dimensions.y];
        }

        /**
         * @return voxel value at the given position. x,y,z outside the dimensions will return NAN.
         */
        inline Voxel getVoxelSafe(size_t x, size_t y, size_t z) const
        {
            if(isInBounds(x, y, z, this->getDimensions())){
                return this->getData()[x + y * this->dimensions.x + z * this->dimensions.x * this->dimensions.y];
            } else {
                return NAN;
            }
        }

        /**
         * @return voxel value at the given position. pos outside the dimensions will cause a crash.
         */
        inline Voxel getVoxel(const voxie::scripting::IntVector3& pos) const
        {
            return getVoxel(pos.x,pos.y,pos.z);
        }

        /**
         * @return voxel value at the given position. pos outside the dimensions will return NAN.
         */
        inline virtual Voxel getVoxelSafe(const voxie::scripting::IntVector3& pos) const
        {
            return getVoxelSafe(pos.x,pos.y,pos.z);
        }

        /**
         * @param method the interpolation method: nearestNeighbor or linear (=default)
         * @return voxel value at the interpolated metric position.
         */
        Voxel getVoxelMetric(qreal x, qreal y, qreal z, InterpolationMethod method = linear) const;

        /**
         * @brief setVoxel sets the value of the voxel at the given position to voxel.
         * x,y,z outside the dimensions will crash.
         */
        inline void setVoxel(size_t x, size_t y, size_t z, Voxel voxel)
        {
            this->getData()[x + y * this->dimensions.x + z * this->dimensions.x * this->dimensions.y] = voxel;
        }

        /**
         * @brief setVoxel sets the value of the voxel at the given position to voxel.
         * x,y,z outside the dimensions will do nothing.
         */
        inline void setVoxelSafe(size_t x, size_t y, size_t z, Voxel voxel)
        {
            if(isInBounds(x, y, z, this->getDimensions())){
                this->getData()[x + y * this->dimensions.x + z * this->dimensions.x * this->dimensions.y] = voxel;
            }
        }

        /**
         * @brief setVoxel sets the value of the voxel at the given position to voxel.
         * pos outside the dimensions will crash.
         */
        inline void setVoxel(const voxie::scripting::IntVector3& pos, Voxel voxel)
        {
            setVoxel(pos.x, pos.y, pos.z, voxel);
        }

        /**
         * @brief setVoxel sets the value of the voxel at the given position to voxel.
         * pos outside the dimensions will crash.
         */
        inline void setVoxelSafe(const voxie::scripting::IntVector3& pos, Voxel voxel)
        {
            setVoxel(pos.x, pos.y, pos.z, voxel);
        }

        /**
         * @return number of Voxels in dataset
         */
        inline size_t getSize() const
        {
            return this->size;
        }

        /**
         * @return size of the voxel data set in bytes.
         */
        inline size_t getByteSize() const
        {
            return this->size * sizeof(Voxel);
        }

        /**
         * @return x, y, z dimensions of the dataset
         */
        inline const voxie::scripting::IntVector3& getDimensions() const
        {
            return this->dimensions;
        }

        /**
         * @return x, y, z dimensions in meters of the dataset
         */
        Q_INVOKABLE inline const QVector3D& getDimensionsMetric() const
        {
            return this->dimensionsMetric;
        }

        /**
         * @return voxel data as an array; x increases fastest, then y, then z.
         */
        inline Voxel* getData() const
        {
            //return this->data;
            return (Voxel*) this->dataSH.getData();
        }

        /**
         * @return spacing/scaling of the dataset.
         */
        Q_INVOKABLE inline const QVector3D& getSpacing() const
        {
            return this->spacing;
        }

        /**
         * @return Metric position of first voxel relative to origin.
         * @see getOrigin()
         */
        Q_INVOKABLE inline const QVector3D& getFirstVoxelPosition() const
        {
            return this->firstVoxelPos;
        }

        /**
         * @return Metric position of Origin relative to first voxel.
         * @see getFirstVoxelPosition()
         */
        Q_INVOKABLE inline const QVector3D getOrigin() const
        {
            return -this->firstVoxelPos;
        }

        /**
         * @brief setSpacing sets the spacing/scaling of the data set.
         */
        inline void setSpacing(QVector3D spacing)
        {
            this->spacing = spacing;
            this->dimensionsMetric = this->dimensions.toQVector3D() * this->spacing;
        }

        /**
         * @brief sets the position of the first voxel relative to the origin
         * @param pos
         * @see setOrigin()
         */
        inline void setFirstVoxelPos(QVector3D pos)
        {
            if(pos != this->firstVoxelPos){
                this->firstVoxelPos = pos;
                emit this->changed();
            }
        }

        /**
         * @brief sets the position of origin relative to first voxel of dataset
         * @param origin
         * @see setFirstVoxelPos()
         */
        inline void setOrigin(QVector3D origin)
        {
            this->setFirstVoxelPos(-origin);
        }

        /**
         * @brief sets the position of origin relative to first voxel of dataset
         * @see setOrigin()
         */
        Q_INVOKABLE inline void setOrigin(qreal x, qreal y, qreal z)
        {
            this->setOrigin(QVector3D(x,y,z));
        }


        /**
         * This does not copy any references, except for parent.
         * @return a copy of the origin data set.
         */
        VoxelData* clone() const;

        VoxelData* reducedSize(uint xStepsize = 2, uint yStepsize = 2, uint zStepsize = 2) const;

        cl::Image3D &getCLImage();

        /**
         * @return calculates min and max of the values in this dataset
         * and retunspair(min,max). This is an expensive operation
         * @see getMinMaxValue()
         */
        inline QPair<Voxel,Voxel> calcMinMaxValue() const
        {
            //qDebug() << "calcminmax";
            Voxel min = std::numeric_limits<Voxel>::max();
            Voxel max = std::numeric_limits<Voxel>::lowest();
            Voxel* values = this->getData();
            for(size_t i = 0; i < this->size; i++){
                if(min > values[i])
                    min = values[i];
                if(max < values[i])
                    max = values[i];
            }
            return QPair<Voxel,Voxel>(min, max);
        }

        /**
         * @return cached pair(min,max) of the values in this dataset. Calls
         * calcMinMax on demand when changes have been made to the data and
         * cached minmax is no longer valid.
         */
        inline const QPair<Voxel,Voxel>& getMinMaxValue()
        {
            if(!this->minMaxValid){
                this->minMax = calcMinMaxValue();
                this->minMaxValid = true;
            }
            return this->minMax;
        }

        /**
         * @return cached pair(min,max) of the values in this dataset.
         * @param valid is set to true when the returned value is valid. this may
         * not be the case when changes have been made to this datasets data.
         */
        const QPair<Voxel,Voxel>& getMinMaxValueConst(bool* valid = nullptr) const
        {
            if(valid != nullptr)
                *valid = this->minMaxValid;
            return this->minMax;
        }

        /**
         * @return whether cached minmax values are valid
         */
        bool isMinMaxValid() const {return this->minMaxValid;}

        /**
         * @return  whether climage of this dataset is valid
         * (in sync with current data)
         */
        bool isClImageValid() const {return this->clImageValid;}

        /**
         * @brief Transform all voxels of the data set inplace.
         * @param f The function that transforms the voxels.
         * @remarks The transformation runs with thread pooling.
         */
        void transform(const std::function<Voxel(Voxel)> &f);

        /**
         * @brief Transform all voxels of the data set inplace with given voxel coordinates.
         * @param f The function that transforms the voxels.
         * @remarks The transformation runs with thread pooling.
         */
        void transformCoordinate(const std::function<Voxel(size_t,size_t,size_t,Voxel)> &f);
    public:
        voxie::scripting::Array3Info dataFd (bool rw);

    signals:
        void changed();

    public slots:
        /**
         * meant to be called by Filter3D or other isntances that make changes to
         * this datasets data. Sets minMaxvalid and clImageValid to false
         */
        void invalidate()
        {this->minMaxValid = this->clImageValid = false;}

    public:
        /**
         * updates clImage of this dataset. need to call this if clImage is not
         * valid due to changes to the datasets data.
         * @see isClImageValid()
         */
        void updateClImage();

        // throws ScriptingException
        void extractSlice(const QVector3D& origin, const QQuaternion& rotation, const QSize& outputSize, double pixelSizeX, double pixelSizeY, InterpolationMethod interpolation, FloatImage& outputImage);
};

namespace internal {
class VoxelDataAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.VoxelData")

    VoxelData* object;

public:
    VoxelDataAdaptor (VoxelData* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~VoxelDataAdaptor () {}

    Q_PROPERTY (QVector3D Origin READ origin) // TODO: make writable?
    //Q_PROPERTY (QVector3D Origin READ origin WRITE setOrigin)
    QVector3D origin();
    void setOrigin(QVector3D value);

    Q_PROPERTY (QVector3D Spacing READ spacing)
    QVector3D spacing();

    // type needs to be fully qualified for moc
    Q_PROPERTY (voxie::scripting::IntVector3 Size READ size)
    voxie::scripting::IntVector3 size();

public slots:
    void UpdateFromBuffer(const QMap<QString, QVariant>& options);

    voxie::scripting::Array3Info GetDataReadonly ();
    voxie::scripting::Array3Info GetDataWritable ();

    void ExtractSlice (const QVector3D& origin, const QQuaternion& rotation, const voxie::scripting::IntVector2& outputSize, const QVector2D& pixelSize, QDBusObjectPath outputImage, const QMap<QString, QVariant>& options);
};
}

}}

Q_DECLARE_METATYPE(voxie::data::VoxelData*)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
