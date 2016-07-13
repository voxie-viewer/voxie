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

template <typename T, typename... U>
static inline QSharedPointer<T> createQSharedPointer(U&&... par) {
    // QSharedPointer<T>::create() is broken: If the constructor throws an
    // exception, it will call the objects destructor.
    // https://bugreports.qt.io/browse/QTBUG-49824
    return QSharedPointer<T>(new T(std::forward<U>(par)...));
}

typedef float Voxel;

struct Error : public QException {
    QString msg;
    Error(const QString& msg) : msg(msg) {
    }
    virtual ~Error() {
    }
};

Q_NORETURN static void error(const QString& str) {
    QTextStream(stderr) << str << endl << flush;

    throw Error(str);
}

static inline void msg(const QString& str) {
    QTextStream(stderr) << str << endl << flush;
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

class ClaimedOperation {
public:
    QSharedPointer<de::uni_stuttgart::Voxie::Client> client;
    de::uni_stuttgart::Voxie::ExternalOperation exop;
    de::uni_stuttgart::Voxie::ExternalOperationLoad exop_l;

    ClaimedOperation(const QString& name, const QString& path, const QDBusConnection& connection, const QSharedPointer<de::uni_stuttgart::Voxie::Client>& client) :
        client(client),
        exop(name, path, connection),
        exop_l(name, path, connection)
    {
        if (!exop.isValid())
            error("Error while getting external operation object: " + exop.lastError().name() + ": " + exop.lastError().message());
        if (!exop_l.isValid())
            error("Error while getting external operation load object: " + exop_l.lastError().name() + ": " + exop_l.lastError().message());

        exop.ClaimOperation(QDBusObjectPath(client->path()));
    }
    ~ClaimedOperation() {
        client->DecRefCount(QDBusObjectPath(exop.path()));
    }
};

class RefCountedObject {
public:
    QSharedPointer<de::uni_stuttgart::Voxie::Client> client;
    QDBusObjectPath path;

    RefCountedObject(const QSharedPointer<de::uni_stuttgart::Voxie::Client>& client, const QDBusObjectPath& path) :
        client(client),
        path(path)
    {
    }
    ~RefCountedObject() {
        client->DecRefCount(path);
    }
};

struct VgiValue {
    QString rawValue;

    VgiValue(const QString&& rawValue = "") : rawValue(rawValue) {
    }
    VgiValue(const QString& rawValue) : rawValue(rawValue) {
    }

    static qint32 myToInt32(const QString& str) {
        bool ok = false;
        auto res = str.toInt(&ok);
        if (!ok)
            error("Failed to convert '" + str + "' to int32");
        return res;
    }
    static qint64 myToInt64(const QString& str) {
        bool ok = false;
        auto res = str.toLongLong(&ok);
        if (!ok)
            error("Failed to convert '" + str + "' to int64");
        return res;
    }
    static double myToDouble(const QString& str) {
        bool ok = false;
        auto res = str.toDouble(&ok);
        if (!ok)
            error("Failed to convert '" + str + "' to double");
        return res;
    }
    
    QString lowercaseStringTrimmed() const {
      return rawValue.toLower().trimmed();
    }

    int integer() const {
        return myToInt32(rawValue.trimmed());
    }

    QVector<qint32> intArray() const {
        auto values = rawValue.trimmed().split(" ", QString::SkipEmptyParts);
        QVector<qint32> result(values.length());
        for (int i = 0; i < values.length(); i++)
            result[i] = myToInt32(values[i]);
        return result;
    }
  
    long longValue() const {
        return myToInt64(rawValue.trimmed());
    }
  
    QVector<qint64> longArray() const {
        auto values = rawValue.trimmed().split(" ", QString::SkipEmptyParts);
        QVector<qint64> result(values.length());
        for (int i = 0; i < values.length(); i++)
            result[i] = myToInt64(values[i]);
        return result;
    }
  
    double doubleValue() const {
        return myToDouble(rawValue.trimmed());
    }
  
    QVector<double> doubleArray() const {
        auto values = rawValue.trimmed().split(" ", QString::SkipEmptyParts);
        QVector<double> result(values.length());
        for (int i = 0; i < values.length(); i++)
            result[i] = myToDouble(values[i]);
        return result;
    }
};

struct VgiGroup {
    QMap<QString, VgiValue> values;

    VgiGroup(const QMap<QString, VgiValue>&& values = (QMap<QString, VgiValue>())) : values(values) {
    }
    VgiGroup(const QMap<QString, VgiValue>& values) : values(values) {
    }

    const VgiValue& operator[] (const QString& s) const {
        auto it = values.find(s);
        if (it == values.end())
            error("Could not find VGI value " + s);
        return *it;
    }
};

struct VgiSection {
    QMap<QString, VgiGroup> groups;

    VgiSection(const QMap<QString, VgiGroup>&& groups = (QMap<QString, VgiGroup>())) : groups(groups) {
    }
    VgiSection(const QMap<QString, VgiGroup>& groups) : groups(groups) {
    }

    const VgiGroup& operator[] (const QString& s) const {
        auto it = groups.find(s);
        if (it == groups.end())
            error("Could not find VGI group " + s);
        return *it;
    }
};

#define VGI_ASSERT_EQUAL(val1, val2)                                    \
    do {                                                                \
        auto _v1 = (val1);                                              \
        auto _v2 = (val2);                                              \
        if (_v1 != _v2)                                                 \
            error("Error in VGI file: " #val1 "(" + QString::number(_v1) + ") != " #val2 " (" + QString::number(_v2) + ")"); \
    } while(0)

struct VgiFile {
    QMap<QString, VgiSection> sections;
    QString directory;

    VgiFile(const QMap<QString, VgiSection>&& sections, const QString& directory) : sections(sections), directory(directory) {
    }
    VgiFile(const QMap<QString, VgiSection>& sections, const QString& directory) : sections(sections), directory(directory) {
    }

    struct Volume {
        QVector<qint64> repSize;
        qint32 repBitsPerElement;
        QString repDataType;
        double repDataRangeMin;
        double repDataRangeMax;

        QVector<qint64> size;
        qint32 bitsPerElement;
        QString dataType;
        double dataRangeMin;
        double dataRangeMax;

        long skipHeader;
        bool hasSkipHeader;
        QString fileName;
        QString orientation;
        QVector<qint32> mirror;

        QString relativeFileName;
    
        Volume (const VgiSection& section, const QString& directory) {
            repSize = section["representation"]["size"].longArray();
            VGI_ASSERT_EQUAL(repSize.length(), 3);
            repBitsPerElement = section["representation"]["bitsperelement"].integer();
            repDataType = section["representation"]["datatype"].lowercaseStringTrimmed();
            auto reprange = section["representation"]["datarange"].doubleArray();
            VGI_ASSERT_EQUAL (reprange.length(), 2);
            repDataRangeMin = reprange[0];
            repDataRangeMax = reprange[1];

            const auto& file1 = section["file1"];
      
            size = file1["size"].longArray();
            VGI_ASSERT_EQUAL (size.length(), 3);
            bitsPerElement = file1["bitsperelement"].integer();
            dataType = file1["datatype"].lowercaseStringTrimmed();
            auto range = file1["datarange"].doubleArray();
            VGI_ASSERT_EQUAL (range.length(), 2);
            dataRangeMin = range[0];
            dataRangeMax = range[1];

            if (file1.values.contains ("skipheader")) {
                skipHeader = file1["skipheader"].longValue();
                hasSkipHeader = true;
            } else {
                skipHeader = 0;
                hasSkipHeader = false;
            }
            fileName = file1["name"].rawValue;
            if (file1.values.contains ("orientation"))
                orientation = file1["orientation"].lowercaseStringTrimmed();
            else
                orientation = "xyz";
            if (file1.values.contains ("mirror"))
                mirror = file1["mirror"].intArray();
            else
                mirror = QVector<qint32>({ 0, 0, 0 });
            VGI_ASSERT_EQUAL (mirror.length(), 3);

            if (section.groups.contains("file2"))
                error("section.groups.contains(\"file2\")");

            QString fn = fileName;
            fn = fn.replace('\\', '/');
            int index = fn.lastIndexOf ("/");
            if (index != -1)
                fn = fn.mid(index + 1);
            relativeFileName = directory + "/" + fn;
        }
    };
    struct VolumePrimitive {
        QVector<double> resolution;
        QString unit;
        QSharedPointer<Volume> volume;
        QString description;
    
        VolumePrimitive (const VgiSection& section, const QMap<qint32, QSharedPointer<Volume>>& volumes) {
            resolution = section["geometry"]["resolution"].doubleArray();
            VGI_ASSERT_EQUAL (resolution.length(), 3);

            unit = section["geometry"]["unit"].lowercaseStringTrimmed();

            auto volumeName = section["volume"]["volume"].lowercaseStringTrimmed();
            if (!volumeName.startsWith("volume"))
                error ("Volume name '" + volumeName + "' does not start with 'volume'");
            auto volumeIndex = volumeName.mid(strlen("volume"));
            volume = volumes[VgiValue::myToInt32(volumeIndex)];
            if (!volume)
                error("Could not find volume '" + volumeName + "'");

            description = section["description"]["text"].rawValue;
        }
    };

    QMap<qint32, QSharedPointer<Volume>> volumes;
    QMap<qint32, QSharedPointer<VolumePrimitive>> volumePrimitives;

    QSharedPointer<VolumePrimitive> firstVolumePrimitive;

    static QSharedPointer<VgiFile> parse(const QString& filename) {
        QFile file(filename);
        if(!file.open(QIODevice::ReadOnly))
            error("Could not open file for reading");
        QTextStream in(&file);

        QMap<QString, VgiSection> sections;
        QString currentSection;
        QMap<QString, VgiGroup> groups;
        QString currentGroup;
        QMap<QString, VgiValue> values;
        while (!in.atEnd()) {
            auto line = in.readLine();
            //qDebug() << line;
            auto trimmed = line.trimmed();
            if (trimmed == "") {
            } else if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
                if (!values.isEmpty()) {
                    groups[currentGroup] = VgiGroup(std::move(values));
                    values.clear();
                    currentGroup = "";
                }
                if (!groups.isEmpty()) {
                    sections[currentSection] = VgiSection(std::move(groups));
                    groups.clear();
                    currentSection = "";
                }
                currentSection = trimmed.mid(1, trimmed.length() - 2).toLower();
                if (sections.contains(currentSection))
                    error("Got section name '" + currentSection + "' multiple times");
            } else if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                if (!values.isEmpty()) {
                    groups[currentGroup] = VgiGroup(std::move(values));
                    values.clear();
                    currentGroup = "";
                }
                currentGroup = trimmed.mid(1, trimmed.length() - 2).toLower();
                if (groups.contains(currentGroup))
                    error("Got group name '" + currentGroup + "' multiple times");
            } else {
                if (currentGroup == "")
                    error("Got value outside group: " + trimmed);
                int pos = line.indexOf("=");
                if (pos == -1)
                    error("Got line without '=' in VGI file: " + trimmed);
                auto key = line.left(pos).trimmed().toLower();
                auto value = line.mid(pos + 1);
                if (value.startsWith (" "))
                    value = value.mid(1);
                //qDebug() << key << value;
                if (values.contains(key))
                    error("Got value name '" + currentGroup + "' multiple times");
                values[key] = VgiValue(std::move(value));
            }
        }
        if (!values.isEmpty()) {
            groups[currentGroup] = VgiGroup(std::move(values));
            values.clear();
            currentGroup = "";
        }
        if (!groups.isEmpty()) {
            sections[currentSection] = VgiSection(std::move(groups));
            groups.clear();
            currentSection = "";
        }

        auto res = createQSharedPointer<VgiFile>(std::move(sections), QFileInfo(filename).dir().path());

        for (auto key : sections.keys()) {
            if (!key.startsWith ("volume"))
                continue;
            auto id = key.mid(strlen("volume"));
            if (id == "" || !id[0].isDigit())
                continue;
            auto nr = VgiValue::myToInt32(id);
            res->volumes.insert(nr, createQSharedPointer<Volume>(sections[key], res->directory));
        }

        qint32 firstId = std::numeric_limits<qint32>::min();
        for (auto key : sections.keys()) {
            if (!key.startsWith ("volumeprimitive"))
                continue;
            auto id = key.mid(strlen("volumeprimitive"));
            if (id == "" || !id[0].isDigit())
                continue;
            auto nr = VgiValue::myToInt32(id);
            auto primitive = createQSharedPointer<VolumePrimitive>(sections[key], res->volumes);
            res->volumePrimitives.insert(nr, primitive);
            if (firstId <= nr) {
                res->firstVolumePrimitive = primitive;
                firstId = nr;
            }
        }

        if (!res->firstVolumePrimitive)
            error("No VolumePrimitive found");
        if (res->volumePrimitives.size() != 1)
            qWarning() << "Warning:" << res->volumePrimitives.size() << "VolumePrimitives found";

        return res;
    }
};

template <typename T, bool swapEndian>
class DataConverterImpl;

class DataConverter {
public:
    DataConverter() {
    }
    virtual ~DataConverter() {
    }

    virtual void convert(const QVector<quint8>& input, Voxel* output, quint64 sizeX, quint64 sizeY, qint64 strideX, qint64 strideY, double scale, double offset) = 0;

    static QSharedPointer<DataConverter> create(const QString& dataType, qint32 bitsPerElement) {
        bool swapEndian;
        if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::BigEndian) {
            swapEndian = true;
        } else if (QSysInfo::Endian::ByteOrder == QSysInfo::Endian::LittleEndian) {
            swapEndian = false;
        } else {
            error("Unkown endianess");
        }

#define T(Ty) do {                                                      \
            if (bitsPerElement == sizeof (Ty) * 8) {                    \
                if (swapEndian)                                         \
                    return createQSharedPointer<DataConverterImpl<Ty, true>>(); \
                else                                                    \
                    return createQSharedPointer<DataConverterImpl<Ty, false>>(); \
            }                                                           \
        } while (0)
        if (dataType == "unsigned integer" || dataType == "int") {
            T(quint8); T(quint16); T(quint32); T(quint64);
        } else if (dataType == "signed integer") {
            T(qint8); T(qint16); T(qint32); T(qint64);
        } else if (dataType == "float") {
            T(float); T(double);
        }
        error("Unknown type: '" + dataType + "' / " + QString::number(bitsPerElement) + " bits per element");
#undef T
    }
};

template <typename T, bool swapEndian>
struct LittleEndianReader;
template <typename T>
struct LittleEndianReader<T, false> {
    static inline const T& read(const T* ptr, ptrdiff_t index) {
        return ptr[index];
    }
};
template <typename T>
struct LittleEndianReader<T, true> {
    static inline T read(const T* ptr, ptrdiff_t index) {
        T ret;
        for (size_t i = 0; i < sizeof(T); i++)
            ((char*) &ret)[sizeof(T) - i - 1] = ((char*) (ptr + index))[i];
        return ret;
    }
};

template <typename T, bool swapEndian>
class DataConverterImpl : public DataConverter {
public:
    DataConverterImpl() {
    }
    virtual ~DataConverterImpl() {
    }

    void convert(const QVector<quint8>& input, Voxel* output, quint64 sizeX, quint64 sizeY, qint64 strideX, qint64 strideY, double scale, double offset) override {
        if (sizeX * sizeY * sizeof(T) > (size_t) input.size())
            error("sizeX * sizeY * sizeof(T) > (size_t) input.size()");
        const T* data = (const T*) input.data();
        for (size_t y = 0; y < sizeY; y++) {
            for (size_t x = 0; x < sizeX; x++) {
                *(Voxel*)((char*)output + strideX * x + strideY * y) = LittleEndianReader<T, swapEndian>::read(data, x + sizeX * y) * scale + offset;
            }
        }
    }
};

int main(int argc, char *argv[]) {
    try {
        if (argc < 1)
            error("argc is smaller than 1");

        QCommandLineParser parser;
        parser.setApplicationDescription("Voxie script for loading VGI files");
        parser.addHelpOption();
        parser.addVersionOption();
    
        QCommandLineOption voxieBusAddress("voxie-bus-address", "Address of the bus to use to connect to Voxie.", "bus-address");
        parser.addOption(voxieBusAddress);
        QCommandLineOption voxieBusName("voxie-bus-name", "Bus name of voxie.", "bus-name");
        parser.addOption(voxieBusName);
        QCommandLineOption voxieAction("voxie-action", "Action to execute.", "action");
        parser.addOption(voxieAction);
        QCommandLineOption voxieOperation("voxie-operation", "Operation object to use.", "operation");
        parser.addOption(voxieOperation);
        QCommandLineOption voxieLoadFilename("voxie-load-filename", "File to load.", "filename");
        parser.addOption(voxieLoadFilename);

        QStringList args;
        for (char** arg = argv; *arg; arg++)
            args.push_back(*arg);
        int argc0 = 1;
        char* args0[2] = { argv[0], NULL };
        //QCoreApplication app(argc0, args0);
        QApplication app(argc0, args0);
        parser.process(args);

        if (parser.isSet(voxieAction) && parser.value(voxieAction) != "Load")
            error("--voxie-action is not 'Load'");

        if (!parser.isSet(voxieOperation))
            error("--voxie-operation is not set");
        QString operationPath = parser.value(voxieOperation);
        if (!parser.isSet(voxieLoadFilename))
            error("--voxie-load-filename is not set");
        QString filename = parser.value(voxieLoadFilename);

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

        auto clientPath = HANDLEDBUSPENDINGREPLY(voxie.CreateClient(QVariantMap()));
        auto client = createQSharedPointer<de::uni_stuttgart::Voxie::Client>(name, clientPath.path(), connection);
        if (!client->isValid())
            error("Error while getting client object: " + client->lastError().name() + ": " + client->lastError().message());

        ClaimedOperation op(name, operationPath, connection, client);

        try {
            auto vgi = VgiFile::parse(filename);

            auto vp = vgi->firstVolumePrimitive;
            auto vol = vp->volume;

            //qDebug() << vol->relativeFileName;
            //qDebug() << vol->skipHeader << vol->size[2] << vol->size[1] << vol->size[0] << vol->bitsPerElement;
            auto raw = vol->relativeFileName;
            auto expectedSize = vol->skipHeader + vol->size[2] * vol->size[1] * vol->size[0] * (vol->bitsPerElement / 8);
            if (!vol->hasSkipHeader) {
                VGI_ASSERT_EQUAL(expectedSize, QFileInfo(raw).size());
            } else {
                if (QFileInfo(raw).size() < expectedSize)
                    error("QFileInfo(raw).size() < expectedSize");
            }

            quint64 resSize[3] = {};
            bool used[3] = { false, false, false };
            VGI_ASSERT_EQUAL(vol->orientation.length(), 3);
            for (int i = 0; i < 3; i++) {
                int dim = vol->orientation[i].unicode() - 'x';
                if (dim < 0 || dim >= 3)
                    error("dim < 0 || dim >= 3");
                if (used[dim])
                    error("dimension used twice");
                resSize[i] = vol->size[dim];
            }

            auto unit = vp->unit;
            double unitFactor;
            if (unit == "m")
                unitFactor = 1;
            else if (unit == "mm")
                unitFactor = 1e-3;
            else
                error("Unknown unit: " + unit);

            QVector3D gridSpacing(vp->resolution[0], vp->resolution[1], vp->resolution[2]);
            gridSpacing *= unitFactor;

            QVector3D gridOrigin(-(resSize[0] * gridSpacing[0] / 2.0f), -(resSize[1] * gridSpacing[1] / 2.0f), -(resSize[2] * gridSpacing[2] / 2.0f));

            QVariantMap options;
            options["Spacing"] = gridSpacing;
            options["Origin"] = gridOrigin;

            voxie::scripting::IntVector3 resSizeVector(resSize[0], resSize[1], resSize[2]);
            QDBusObjectPath voxelDataPath = HANDLEDBUSPENDINGREPLY(voxie.CreateVoxelData(QDBusObjectPath(client->path()), resSizeVector, options));
            RefCountedObject voxelDataRefCount(client, voxelDataPath);
            
            de::uni_stuttgart::Voxie::VoxelData voxelData(name, voxelDataPath.path(), connection);
            if (!voxelData.isValid())
                error("Error while getting voxelData object: " + voxelData.lastError().name() + ": " + voxelData.lastError().message());

            voxie::scripting::Array3Info info = HANDLEDBUSPENDINGREPLY(voxelData.GetDataWritable());

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

            if (info.sizeX != (quint64) resSize[0]
                || info.sizeY != (quint64) resSize[1]
                || info.sizeZ != (quint64) resSize[2])
                error("Did not get requested size");

            size_t bytes = info.sizeX * info.sizeY * info.sizeZ * sizeof (Voxel);
            if (bytes / info.sizeX / info.sizeY / info.sizeZ != sizeof (Voxel))
                error("Overflow while calculating size");

            if (bytes == 0)
                error("Empty dataset");

            qint64 resStrides[3] = { info.strideX, info.strideY, info.strideZ };
            qint64 strides[3] = {};
            qint64 offset = 0;
            for (int i = 0; i < 3; i++) {
                int dim = vol->orientation[i].unicode() - 'x';
                if (dim < 0 || dim >= 3)
                    error("dim < 0 || dim >= 3");
                strides[dim] = resStrides[i];
                if (vol->mirror[dim]) {
                    if (resSize[i] > 0)
                        offset += strides[dim] * (resSize[i] - 1);
                    strides[dim] = -strides[dim];
                }
            }

            double scale;
            double offsetValue;
            scale = (vol->repDataRangeMax - vol->repDataRangeMin) / (vol->dataRangeMax - vol->dataRangeMin);
            offsetValue = vol->repDataRangeMin - scale * vol->dataRangeMin;

            QString type = info.handle["Type"].variant().toString();

#ifndef Q_OS_WIN
            if (type != "UnixFileDescriptor")
                error("Got unknown handle type '" + type + "'");
            QDBusUnixFileDescriptor fd = info.handle["FileDescriptor"].variant().value<QDBusUnixFileDescriptor> ();
            if (!fd.isValid())
                error("Got invalid file descriptor");

            MMapHandle map(mmap (NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd.fileDescriptor(), 0), bytes);
            if (map.ptr == MAP_FAILED)
                error("mmap failed: " + QString(strerror(errno)));

            Voxel* data = (Voxel*) map.ptr;
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

            HandleHolder handle(OpenFileMappingA(FILE_MAP_WRITE, false, objectName.toUtf8().data()));
            if (!handle.handle)
                error (QString("OpenFileMappingA failed: %1").arg(GetLastError()));

            MapHolder map(MapViewOfFile(handle.handle, FILE_MAP_WRITE, 0, 0, bytes));
            if (!map.data)
                error (QString("MapViewOfFile failed: %1").arg(GetLastError()));

            Voxel* data = (Voxel*) map.data;
#endif

            QFile rawFile(raw);
            if (!rawFile.open(QIODevice::ReadOnly))
                error("Could not open raw file for reading");
            if (!rawFile.seek(vol->skipHeader))
                error("Could not seek in raw file");

            QVector<quint8> buffer(vol->size[0] * vol->size[1] * (vol->bitsPerElement / 8));
            auto converter = DataConverter::create(vol->dataType, vol->bitsPerElement);
            //qDebug() << vol->size[0] << vol->size[1] << vol->size[2];
            //qDebug() << resSize[0] << resSize[1] << resSize[2];
            //qDebug() << strides[0] << strides[1] << strides[2];
            for (size_t z = 0; z < (quint64) vol->size[2]; z++) {
                qint64 pos = 0;
                while (pos < buffer.size()) {
                    qint64 res = rawFile.read((char*) (buffer.data() + pos), buffer.size() - pos);
                    if (res < 0)
                        error("Error reading from raw file");
                    if (res == 0)
                        error("Got EOF in raw file");
                    pos += res;
                }
                converter->convert(buffer, (Voxel*) ((char*)data + offset + strides[2] * z), vol->size[0], vol->size[1], strides[0], strides[1], scale, offsetValue);
            }

            op.exop_l.Finish(QDBusObjectPath(voxelData.path()));
            return 0;
        } catch (Error& error) {
            HANDLEDBUSPENDINGREPLY(op.exop.FinishError("de.uni_stuttgart.Voxie.ExtLoadVgi.Error", error.msg));
        }
    } catch (Error& error) {
        return 1;
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
