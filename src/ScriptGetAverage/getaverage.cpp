#include <Main/dbusproxies.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

#include <QtGui/QWindow>

#ifndef Q_OS_WIN
#include <sys/mman.h>

struct MMapHandle {
    void* ptr;
    size_t bytes;
    MMapHandle (void* ptr, size_t bytes) : ptr(ptr), bytes(bytes) {}
    ~MMapHandle () {
        if (ptr != MAP_FAILED)
            munmap(ptr, bytes);
    }
};
#else
#include <windows.h>
#undef interface

struct HandleHolder {
    HANDLE handle;
    HandleHolder(HANDLE handle) : handle(handle) {}
    ~HandleHolder() {
        if (handle)
            CloseHandle(handle);
    }
};
struct MapHolder {
    void* data;
    MapHolder(void* data) : data(data) {}
    ~MapHolder() {
        if (data)
            UnmapViewOfFile(data);
    }
};
#endif

typedef float Voxel;

struct Error {};

WId voxieWindowID = 0;
void setTransientParent(QWidget* widget) {
    auto voxieWindow = voxieWindowID ? QWindow::fromWinId(voxieWindowID) : nullptr;
    widget->winId(); // Trigger creation of native window
    auto window = widget->windowHandle();
    if (voxieWindow && window)
        window->setTransientParent(voxieWindow);
}

void error(const QString& str) {
    qCritical("%s", str.toUtf8().data());

    auto box = new QMessageBox(QMessageBox::Critical, "ScriptGetAverage", str);
    setTransientParent(box);
    box->setText(str);
    box->exec();

    throw Error();
}

void msg(const QString& str) {
    qDebug("%s", str.toUtf8().data());

    auto box = new QMessageBox(QMessageBox::Information, "ScriptGetAverage", str);
    setTransientParent(box);
    box->setText(str);
    box->exec();
}

template <typename T>
inline T handleDBusReply (QDBusPendingReply<T> reply, const QString& additional = "") {
    reply.waitForFinished();
    if (reply.isError())
        error("Error during DBus call " + additional + ": " + reply.error().name() + ": " + reply.error().message());
    return reply.value();
}
template <>
inline void handleDBusReply<void> (QDBusPendingReply<> reply, const QString& additional) {
    reply.waitForFinished();
    if (reply.isError())
        error("Error during DBus call " + additional + ": " + reply.error().name() + ": " + reply.error().message());
}
template <typename T>
inline T handleDBusReply (QDBusReply<T> reply, const QString& additional = "") {
    if (!reply.isValid())
        error("Error during DBus call " + additional + ": " + reply.error().name() + ": " + reply.error().message());
    return reply.value();
}
template <>
inline void handleDBusReply<void> (QDBusReply<void> reply, const QString& additional) {
    if (!reply.isValid())
        error("Error during DBus call " + additional + ": " + reply.error().name() + ": " + reply.error().message());
}
#define HANDLEDBUSPENDINGREPLY(call) handleDBusReply(call, #call)

int main(int argc, char *argv[]) {
    try {
        if (argc < 1)
            error("argc is smaller than 1");

        QCommandLineParser parser;
        parser.setApplicationDescription("Voxie script for getting the average");
        parser.addHelpOption();
        parser.addVersionOption();
    
        QCommandLineOption voxieBusAddress("voxie-bus-address", "Address of the bus to use to connect to Voxie.", "bus-address");
        parser.addOption(voxieBusAddress);
        QCommandLineOption voxieBusName("voxie-bus-name", "Bus name of voxie.", "bus-name");
        parser.addOption(voxieBusName);

        QStringList args;
        for (char** arg = argv; *arg; arg++)
            args.push_back(*arg);
        int argc0 = 1;
        char* args0[2] = { argv[0], NULL };
        //QCoreApplication app(argc0, args0);
        QApplication app(argc0, args0);
        parser.process(args);

        voxie::scripting::initDBusTypes();

        QDBusConnection connection = parser.isSet(voxieBusAddress) ? QDBusConnection::connectToBus(parser.value(voxieBusAddress), "voxie") : QDBusConnection::sessionBus();
        if (!connection.isConnected())
            error("Connecting to DBus failed: " + connection.lastError().name() + ": " + connection.lastError().message());

        QString name;
        if (parser.isSet(voxieBusName)) {
            name = parser.value(voxieBusName);
            if (!name.startsWith(":"))
                name = HANDLEDBUSPENDINGREPLY(connection.interface()->serviceOwner(name));
        } else {
            name = HANDLEDBUSPENDINGREPLY(connection.interface()->serviceOwner("de.uni_stuttgart.Voxie"));
        }

        de::uni_stuttgart::Voxie::Voxie voxie(name, "/de/uni_stuttgart/Voxie", connection);
        if (!voxie.isValid())
            error("Error while getting voxie object: " + voxie.lastError().name() + ": " + voxie.lastError().message());

        QDBusObjectPath guiPath = voxie.gui();
        if (voxie.lastError().isValid())
            error("Error while getting gui path: " + voxie.lastError().name() + ": " + voxie.lastError().message());
        if (guiPath == QDBusObjectPath("/"))
            error("No remote GUI object found");
        de::uni_stuttgart::Voxie::Gui gui(name, guiPath.path(), connection);
        if (!gui.isValid())
            error("Error while getting gui object: " + gui.lastError().name() + ": " + gui.lastError().message());

        voxieWindowID = gui.GetMainWindowID();
        if (gui.lastError().isValid())
            error("Error while getting main window ID: " + gui.lastError().name() + ": " + gui.lastError().message());

        QDBusObjectPath activeVisualizerPath = gui.activeVisualizer();
        if (gui.lastError().isValid())
            error("Error while getting activeVisualizer path: " + gui.lastError().name() + ": " + gui.lastError().message());
        if (activeVisualizerPath == QDBusObjectPath("/"))
            error("No visualizer selected");
        de::uni_stuttgart::Voxie::VolumeDataVisualizer activeVisualizer(name, activeVisualizerPath.path(), connection);
        if (!activeVisualizer.isValid())
            error("Error while getting activeVisualizer object: " + activeVisualizer.lastError().name() + ": " + activeVisualizer.lastError().message());

        QDBusObjectPath dataSetPath = activeVisualizer.dataSet();
        if (activeVisualizer.lastError().isValid())
            error("Error while getting dataSet path: " + activeVisualizer.lastError().name() + ": " + activeVisualizer.lastError().message());
        if (dataSetPath == QDBusObjectPath("/"))
            error("Could not get dataset path");
        de::uni_stuttgart::Voxie::DataSet dataSet(name, dataSetPath.path(), connection);
        if (!dataSet.isValid())
            error("Error while getting dataSet object: " + dataSet.lastError().name() + ": " + dataSet.lastError().message());

        QDBusObjectPath voxelDataPath = dataSet.filteredData();
        if (dataSet.lastError().isValid())
            error("Error while getting voxelData path: " + dataSet.lastError().name() + ": " + dataSet.lastError().message());
        de::uni_stuttgart::Voxie::VoxelData voxelData(name, voxelDataPath.path(), connection);
        if (!voxelData.isValid())
            error("Error while getting voxelData object: " + voxelData.lastError().name() + ": " + voxelData.lastError().message());

        voxie::scripting::Array3Info info = HANDLEDBUSPENDINGREPLY(voxelData.GetDataReadonly());

        QString byteorder;
        if (sizeof (Voxel) == 1) {
            byteorder = "none";
        } else {
            if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
                byteorder = "big";
            } else if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
                byteorder = "little";
            } else {
                byteorder = "unknown";
            }
        }
        if (info.dataType != "float"
            || info.dataTypeSize != sizeof (Voxel) * 8
            || info.byteorder != byteorder)
            error("Unknown data type");

        size_t bytes = info.sizeX * info.sizeY * info.sizeZ * sizeof (Voxel);
        if (bytes / info.sizeX / info.sizeY / info.sizeZ != sizeof (Voxel))
            error("Overflow while calculating size");

        if (bytes == 0)
            error("Empty dataset");

        if (info.offset != 0
            || info.strideX != (ptrdiff_t) (sizeof (Voxel))
            || info.strideY != (ptrdiff_t) (info.sizeX * sizeof (Voxel))
            || info.strideZ != (ptrdiff_t) (info.sizeX * info.sizeY * sizeof (Voxel)))
            error("Unexpected values for offset or strides");

        QString type = info.handle["Type"].variant().toString();

#ifndef Q_OS_WIN
        if (type != "UnixFileDescriptor")
            error("Got unknown handle type '" + type + "'");
        QDBusUnixFileDescriptor fd = info.handle["FileDescriptor"].variant().value<QDBusUnixFileDescriptor> ();
        if (!fd.isValid())
            error("Got invalid file descriptor");

        MMapHandle map(mmap (NULL, bytes, PROT_READ, MAP_SHARED, fd.fileDescriptor(), 0), bytes);
        if (map.ptr == MAP_FAILED)
            error("mmap failed: " + QString(strerror(errno)));

        const Voxel* data = (const Voxel*) map.ptr;
#else
        if (type != "WindowsNamedFileMapping")
            error("Got unknown handle type '" + type + "'");

        QString localMachineIDVoxie = info.handle["LocalMachineID"].variant().toString();
        QString localMachineIDLocal = QString::fromUtf8(QDBusConnection::localMachineId());
        if (localMachineIDVoxie != localMachineIDLocal)
            error ("Machine ID mismatch: '" + localMachineIDVoxie + "' (Voxie) != '" + localMachineIDLocal + "' (local)");

        DWORD pid = GetCurrentProcessId();
        DWORD sessionIDLocal;
        if (!ProcessIdToSessionId(pid, &sessionIDLocal))
            error("Call to ProcessIdToSessionId() failed");
        quint32 sessionIDVoxie = info.handle["SessionID"].variant().value<quint32>();
        if (sessionIDVoxie != sessionIDLocal)
            error (QString("Session ID mismatch: '%1' (Voxie) != '%2' (local)").arg(sessionIDVoxie).arg(sessionIDLocal));

        QString objectName = info.handle["MappingObjectName"].variant().toString();

        HandleHolder handle(OpenFileMappingA(FILE_MAP_READ, false, objectName.toUtf8().data()));
        if (!handle.handle)
            error (QString("OpenFileMappingA failed: %1").arg(GetLastError()));

        MapHolder map(MapViewOfFile(handle.handle, FILE_MAP_READ, 0, 0, bytes));
        if (!map.data)
            error (QString("MapViewOfFile failed: %1").arg(GetLastError()));

        const Voxel* data = (const Voxel*) map.data;
#endif

        size_t count = bytes / sizeof (Voxel);

        double sum = 0;
        for (size_t i = 0; i < count; i++)
            sum += data[i];
        double avg = sum / count;

        msg(QString("The average of %1 values is %2").arg(count).arg(avg));

        return 0;
    } catch (Error& error) {
        return 1;
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
