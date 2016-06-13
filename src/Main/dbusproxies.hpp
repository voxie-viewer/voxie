#include <Voxie/scripting/dbustypes.hpp>
/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -p Main/dbusproxies
 *
 * qdbusxml2cpp is Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef VOXIE_DBUSPROXIES_H
#define VOXIE_DBUSPROXIES_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Client
 */
class DeUni_stuttgartVoxieClientInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Client"; }

public:
    DeUni_stuttgartVoxieClientInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieClientInterface();

    Q_PROPERTY(QString UniqueConnectionName READ uniqueConnectionName)
    inline QString uniqueConnectionName() const
    { return qvariant_cast< QString >(property("UniqueConnectionName")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<bool> DecRefCount(const QDBusObjectPath &o)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(o);
        return asyncCallWithArgumentList(QLatin1String("DecRefCount"), argumentList);
    }

    inline QDBusPendingReply<QMap_QDBusObjectPath_quint64> GetReferencedObjects()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetReferencedObjects"), argumentList);
    }

    inline QDBusPendingReply<> IncRefCount(const QDBusObjectPath &o)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(o);
        return asyncCallWithArgumentList(QLatin1String("IncRefCount"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.DataSet
 */
class DeUni_stuttgartVoxieDataSetInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.DataSet"; }

public:
    DeUni_stuttgartVoxieDataSetInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieDataSetInterface();

    Q_PROPERTY(QString DisplayName READ displayName)
    inline QString displayName() const
    { return qvariant_cast< QString >(property("DisplayName")); }

    Q_PROPERTY(QDBusObjectPath FilteredData READ filteredData)
    inline QDBusObjectPath filteredData() const
    { return qvariant_cast< QDBusObjectPath >(property("FilteredData")); }

    Q_PROPERTY(QDBusObjectPath OriginalData READ originalData)
    inline QDBusObjectPath originalData() const
    { return qvariant_cast< QDBusObjectPath >(property("OriginalData")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> CreateSlice(const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("CreateSlice"), argumentList);
    }

    inline QDBusPendingReply<QList<QDBusObjectPath> > ListSlices()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("ListSlices"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Gui
 */
class DeUni_stuttgartVoxieGuiInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Gui"; }

public:
    DeUni_stuttgartVoxieGuiInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieGuiInterface();

    Q_PROPERTY(QDBusObjectPath ActiveVisualizer READ activeVisualizer)
    inline QDBusObjectPath activeVisualizer() const
    { return qvariant_cast< QDBusObjectPath >(property("ActiveVisualizer")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<qulonglong> GetMainWindowID()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetMainWindowID"), argumentList);
    }

    inline QDBusPendingReply<> RaiseWindow(const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("RaiseWindow"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Image
 */
class DeUni_stuttgartVoxieImageInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Image"; }

public:
    DeUni_stuttgartVoxieImageInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieImageInterface();

    Q_PROPERTY(qulonglong Height READ height)
    inline qulonglong height() const
    { return qvariant_cast< qulonglong >(property("Height")); }

    Q_PROPERTY(qulonglong Width READ width)
    inline qulonglong width() const
    { return qvariant_cast< qulonglong >(property("Width")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<voxie::scripting::Array2Info> GetDataReadonly()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetDataReadonly"), argumentList);
    }

    inline QDBusPendingReply<double> GetPixel(qulonglong x, qulonglong y)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(x) << QVariant::fromValue(y);
        return asyncCallWithArgumentList(QLatin1String("GetPixel"), argumentList);
    }

    inline QDBusPendingReply<> SetPixel(qulonglong x, qulonglong y, double val)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(x) << QVariant::fromValue(y) << QVariant::fromValue(val);
        return asyncCallWithArgumentList(QLatin1String("SetPixel"), argumentList);
    }

    inline QDBusPendingReply<> UpdateBuffer(const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("UpdateBuffer"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Importer
 */
class DeUni_stuttgartVoxieImporterInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Importer"; }

public:
    DeUni_stuttgartVoxieImporterInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieImporterInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> Import(const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("Import"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Loader
 */
class DeUni_stuttgartVoxieLoaderInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Loader"; }

public:
    DeUni_stuttgartVoxieLoaderInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieLoaderInterface();

    Q_PROPERTY(QVariantMap Filter READ filter)
    inline QVariantMap filter() const
    { return qvariant_cast< QVariantMap >(property("Filter")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> Load(const QString &fileName, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(fileName) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("Load"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Plugin
 */
class DeUni_stuttgartVoxiePluginInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Plugin"; }

public:
    DeUni_stuttgartVoxiePluginInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxiePluginInterface();

    Q_PROPERTY(QString Name READ name)
    inline QString name() const
    { return qvariant_cast< QString >(property("Name")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> GetMemberByName(const QString &type, const QString &name)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(type) << QVariant::fromValue(name);
        return asyncCallWithArgumentList(QLatin1String("GetMemberByName"), argumentList);
    }

    inline QDBusPendingReply<QList<QDBusObjectPath> > ListMembers(const QString &type)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(type);
        return asyncCallWithArgumentList(QLatin1String("ListMembers"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.PluginMember
 */
class DeUni_stuttgartVoxiePluginMemberInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.PluginMember"; }

public:
    DeUni_stuttgartVoxiePluginMemberInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxiePluginMemberInterface();

    Q_PROPERTY(QString Name READ name)
    inline QString name() const
    { return qvariant_cast< QString >(property("Name")); }

    Q_PROPERTY(QDBusObjectPath Plugin READ plugin)
    inline QDBusObjectPath plugin() const
    { return qvariant_cast< QDBusObjectPath >(property("Plugin")); }

    Q_PROPERTY(QString Type READ type)
    inline QString type() const
    { return qvariant_cast< QString >(property("Type")); }

public Q_SLOTS: // METHODS
Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Plugins.ExamplePlugin.TheSphereGenerator
 */
class DeUni_stuttgartVoxiePluginsExamplePluginTheSphereGeneratorInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Plugins.ExamplePlugin.TheSphereGenerator"; }

public:
    DeUni_stuttgartVoxiePluginsExamplePluginTheSphereGeneratorInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxiePluginsExamplePluginTheSphereGeneratorInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> GenerateSphere(int size, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(size) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("GenerateSphere"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Slice
 */
class DeUni_stuttgartVoxieSliceInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Slice"; }

public:
    DeUni_stuttgartVoxieSliceInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieSliceInterface();

    Q_PROPERTY(QDBusObjectPath DataSet READ dataSet)
    inline QDBusObjectPath dataSet() const
    { return qvariant_cast< QDBusObjectPath >(property("DataSet")); }

    Q_PROPERTY(QString DisplayName READ displayName)
    inline QString displayName() const
    { return qvariant_cast< QString >(property("DisplayName")); }

    Q_PROPERTY(voxie::scripting::Plane Plane READ plane WRITE setPlane)
    inline voxie::scripting::Plane plane() const
    { return qvariant_cast< voxie::scripting::Plane >(property("Plane")); }
    inline void setPlane(voxie::scripting::Plane value)
    { setProperty("Plane", QVariant::fromValue(value)); }

public Q_SLOTS: // METHODS
Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.SliceDataVisualizer
 */
class DeUni_stuttgartVoxieSliceDataVisualizerInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.SliceDataVisualizer"; }

public:
    DeUni_stuttgartVoxieSliceDataVisualizerInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieSliceDataVisualizerInterface();

    Q_PROPERTY(QDBusObjectPath Slice READ slice)
    inline QDBusObjectPath slice() const
    { return qvariant_cast< QDBusObjectPath >(property("Slice")); }

public Q_SLOTS: // METHODS
Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.VisualizerFactory
 */
class DeUni_stuttgartVoxieVisualizerFactoryInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.VisualizerFactory"; }

public:
    DeUni_stuttgartVoxieVisualizerFactoryInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieVisualizerFactoryInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> Create(const QList<QDBusObjectPath> &dataSets, const QList<QDBusObjectPath> &slices, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(dataSets) << QVariant::fromValue(slices) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("Create"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.VolumeDataVisualizer
 */
class DeUni_stuttgartVoxieVolumeDataVisualizerInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.VolumeDataVisualizer"; }

public:
    DeUni_stuttgartVoxieVolumeDataVisualizerInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieVolumeDataVisualizerInterface();

    Q_PROPERTY(QDBusObjectPath DataSet READ dataSet)
    inline QDBusObjectPath dataSet() const
    { return qvariant_cast< QDBusObjectPath >(property("DataSet")); }

public Q_SLOTS: // METHODS
Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.VoxelData
 */
class DeUni_stuttgartVoxieVoxelDataInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.VoxelData"; }

public:
    DeUni_stuttgartVoxieVoxelDataInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieVoxelDataInterface();

    Q_PROPERTY(QVector3D Origin READ origin)
    inline QVector3D origin() const
    { return qvariant_cast< QVector3D >(property("Origin")); }

    Q_PROPERTY(voxie::scripting::IntVector3 Size READ size)
    inline voxie::scripting::IntVector3 size() const
    { return qvariant_cast< voxie::scripting::IntVector3 >(property("Size")); }

    Q_PROPERTY(QVector3D Spacing READ spacing)
    inline QVector3D spacing() const
    { return qvariant_cast< QVector3D >(property("Spacing")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> ExtractSlice(const QVector3D &origin, const QQuaternion &rotation, voxie::scripting::IntVector2 outputSize, const QVector2D &pixelSize, const QDBusObjectPath &outputImage, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(origin) << QVariant::fromValue(rotation) << QVariant::fromValue(outputSize) << QVariant::fromValue(pixelSize) << QVariant::fromValue(outputImage) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("ExtractSlice"), argumentList);
    }

    inline QDBusPendingReply<voxie::scripting::Array3Info> GetDataReadonly()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetDataReadonly"), argumentList);
    }

    inline QDBusPendingReply<voxie::scripting::Array3Info> GetDataWritable()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("GetDataWritable"), argumentList);
    }

    inline QDBusPendingReply<> UpdateFromBuffer(const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("UpdateFromBuffer"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

/*
 * Proxy class for interface de.uni_stuttgart.Voxie.Voxie
 */
class DeUni_stuttgartVoxieVoxieInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "de.uni_stuttgart.Voxie.Voxie"; }

public:
    DeUni_stuttgartVoxieVoxieInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~DeUni_stuttgartVoxieVoxieInterface();

    Q_PROPERTY(QDBusObjectPath Gui READ gui)
    inline QDBusObjectPath gui() const
    { return qvariant_cast< QDBusObjectPath >(property("Gui")); }

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<QDBusObjectPath> CreateClient(const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("CreateClient"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> CreateImage(const QDBusObjectPath &client, voxie::scripting::IntVector2 size, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(client) << QVariant::fromValue(size) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("CreateImage"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> CreateIndependentClient(const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("CreateIndependentClient"), argumentList);
    }

    inline QDBusPendingReply<bool> DestroyClient(const QDBusObjectPath &client, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(client) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("DestroyClient"), argumentList);
    }

    inline QDBusPendingReply<QDBusVariant> ExecuteQScriptCode(const QString &code, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(code) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("ExecuteQScriptCode"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> GetPluginByName(const QString &name)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(name);
        return asyncCallWithArgumentList(QLatin1String("GetPluginByName"), argumentList);
    }

    inline QDBusPendingReply<QList<QDBusObjectPath> > ListDataSets()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("ListDataSets"), argumentList);
    }

    inline QDBusPendingReply<QStringList> ListPluginMemberTypes()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("ListPluginMemberTypes"), argumentList);
    }

    inline QDBusPendingReply<QList<QDBusObjectPath> > ListPlugins()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QLatin1String("ListPlugins"), argumentList);
    }

    inline QDBusPendingReply<QDBusObjectPath> OpenFile(const QString &file, const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(file) << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("OpenFile"), argumentList);
    }

    inline QDBusPendingReply<> Quit(const QVariantMap &options)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(options);
        return asyncCallWithArgumentList(QLatin1String("Quit"), argumentList);
    }

Q_SIGNALS: // SIGNALS
};

namespace de {
  namespace uni_stuttgart {
    namespace Voxie {
      typedef ::DeUni_stuttgartVoxieClientInterface Client;
      typedef ::DeUni_stuttgartVoxieDataSetInterface DataSet;
      typedef ::DeUni_stuttgartVoxieGuiInterface Gui;
      typedef ::DeUni_stuttgartVoxieImageInterface Image;
      typedef ::DeUni_stuttgartVoxieImporterInterface Importer;
      typedef ::DeUni_stuttgartVoxieLoaderInterface Loader;
      typedef ::DeUni_stuttgartVoxiePluginInterface Plugin;
      typedef ::DeUni_stuttgartVoxiePluginMemberInterface PluginMember;
      namespace Plugins {
        namespace ExamplePlugin {
          typedef ::DeUni_stuttgartVoxiePluginsExamplePluginTheSphereGeneratorInterface TheSphereGenerator;
        }
      }
      typedef ::DeUni_stuttgartVoxieSliceInterface Slice;
      typedef ::DeUni_stuttgartVoxieSliceDataVisualizerInterface SliceDataVisualizer;
      typedef ::DeUni_stuttgartVoxieVisualizerFactoryInterface VisualizerFactory;
      typedef ::DeUni_stuttgartVoxieVolumeDataVisualizerInterface VolumeDataVisualizer;
      typedef ::DeUni_stuttgartVoxieVoxelDataInterface VoxelData;
      typedef ::DeUni_stuttgartVoxieVoxieInterface Voxie;
    }
  }
}
#endif
