#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/scripting/scriptingcontainer.hpp>

#include <functional>

#include <inttypes.h>

#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QScopedPointer>
#include <QtCore/QtGlobal>
#include <QtCore/QTime>
#include <QtCore/QVariant>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusContext>

#include <QtGui/QVector3D>

/**
 * @ingroup data
 */

namespace voxie { namespace data {
class VoxelData;
class Slice;

class VOXIECORESHARED_EXPORT DataSet : public voxie::scripting::ScriptingContainer, public QDBusContext
{
    Q_OBJECT

    private:
        VoxelData* originalDataSet = nullptr;
        VoxelData* filteredDataSet = nullptr;
        QFileInfo fileInfo;

    public:
        explicit DataSet(QScopedPointer<VoxelData>& data, QObject *parent = nullptr);
        ~DataSet();


        inline QFileInfo getFileInfo()
        {
            return this->fileInfo;
        }

        inline void setFileInfo(QFileInfo fileInfo)
        {
            this->fileInfo = fileInfo;
        }

        /**
         * @return a list containing all slices that are associated with this voxel data set.
         */
        QList<Slice*> getSlices();

        /**
         * @brief resetData Resets the voxel data to the original, unfiltered data.
         * Should be called at the beginning of the 3d filter chain.
         * When resetData is called the first time, a clone of the original data is created.
         */
        void resetData();

        /**
         * creates a new slice and registers it with voxie::root
         * @return Name of the slice child object
         */
        Slice* createSlice();

        /**
         * @brief creates a new slice and registers it with voxie::root
         * @param sliceName wanted name of the slice.
         * @return Name of the slice child object
         */
        Q_INVOKABLE QString createSlice(QString sliceName);
    public:
        static DataSet* getTestDataSet();

        VoxelData* originalData() const { return originalDataSet; }
        VoxelData* filteredData() const { return filteredDataSet ? filteredDataSet : originalDataSet; }

        QVector3D origin() const;
        QVector3D size() const;
        QVector3D volumeCenter() const;
        float diagonalSize() const;

    signals:
        /**
         * @brief changed will be signalled after changes to the data set are complete (it then will be rerendered).
         */
        void changed();
        void sliceChanged(Slice* slice);
};

namespace internal {
class DataSetAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.DataSet")

    DataSet* object;

public:
    DataSetAdaptor (DataSet* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~DataSetAdaptor () {}

    Q_PROPERTY (QDBusObjectPath OriginalData READ originalData)
    QDBusObjectPath originalData ();

    // The object returned by this property changes the first time a filter is applied
    Q_PROPERTY (QDBusObjectPath FilteredData READ filteredData)
    QDBusObjectPath filteredData ();

    Q_PROPERTY (QString DisplayName READ displayName)
    QString displayName ();

public slots:
    QDBusObjectPath CreateSlice(const QMap<QString, QVariant>& options);

    QList<QDBusObjectPath> ListSlices ();
};
}

}}

Q_DECLARE_METATYPE(voxie::data::DataSet*)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
