/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define TINYPLY_IMPLEMENTATION
#include <ExtFilePly/tinyply.h>

#include <VoxieClient/Array.hpp>
#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Exceptions.hpp>
#include <VoxieClient/MappedBuffer.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <VoxieClient/RefCountHolder.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QString>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusPendingReply>

#include <fstream>

// http://gamma.cs.unc.edu/POWERPLANT/papers/ply.pdf
// https://en.wikipedia.org/w/index.php?title=PLY_(file_format)&oldid=717727643

QList<std::tuple<QString, QString>> voxieSurfaceAttributes{
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color.0", "red"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color.1", "green"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color.2", "blue"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color.3", "alpha"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside.0",
                    "back_red"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside.1",
                    "back_green"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside.2",
                    "back_blue"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside.3",
                    "back_alpha"),
};

QList<std::tuple<QString, QString>> voxieVertexAttributes{
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color.0", "red"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color.1", "green"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color.2", "blue"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.Color.3", "alpha"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside.0",
                    "back_red"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside.1",
                    "back_green"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside.2",
                    "back_blue"),
    std::make_tuple("de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside.3",
                    "back_alpha"),
};

Q_NORETURN static void error(const QString& str) {
  throw vx::Exception("de.uni_stuttgart.Voxie.ExtPly.Error", str);
}

template <typename T, typename F>
void switchPlyTypeHelper(tinyply::Buffer* buffer, const F& f) {
  auto ptr = (const T*)buffer->get();
  if (buffer->size_bytes() % sizeof(T) != 0)
    error("buffer->size_bytes() % sizeof(T) != 0");
  size_t count = buffer->size_bytes() / sizeof(T);
  f(ptr, count);
}
// Call f with a pointer to the data (properly typed) and the number of values
template <typename F>
void switchPlyType(const std::shared_ptr<tinyply::PlyData>& data, const F& f) {
  switch (data->t) {
#define TYPE(TinyplyType, CppType)                     \
  case tinyply::Type::TinyplyType:                     \
    switchPlyTypeHelper<CppType, F>(&data->buffer, f); \
    break
    TYPE(INT8, qint8);
    TYPE(INT16, qint16);
    TYPE(INT32, qint32);
    TYPE(UINT8, quint8);
    TYPE(UINT16, quint16);
    TYPE(UINT32, quint32);
    TYPE(FLOAT32, float);
    TYPE(FLOAT64, double);
#undef TYPE
    case tinyply::Type::INVALID:
      error("invalid ply property");
    default:
      error("Got unknown tinyply type");
  }
}
template <typename F>
void switchPlyTypeSimple(tinyply::Type t, const F& f) {
  switch (t) {
#define TYPE(TinyplyType, CppType) \
  case tinyply::Type::TinyplyType: \
    f((CppType)0);                 \
    break
    TYPE(INT8, qint8);
    TYPE(INT16, qint16);
    TYPE(INT32, qint32);
    TYPE(UINT8, quint8);
    TYPE(UINT16, quint16);
    TYPE(UINT32, quint32);
    TYPE(FLOAT32, float);
    TYPE(FLOAT64, double);
#undef TYPE
    case tinyply::Type::INVALID:
      error("invalid ply property");
    default:
      error("Got unknown tinyply type");
  }
}

static std::tuple<QString, quint32, QString> getVoxieType(tinyply::Type t) {
  switch (t) {
#define TYPE(TinyplyType, VoxieType, BitCount) \
  case tinyply::Type::TinyplyType:             \
    return std::make_tuple(VoxieType, BitCount, "native");
    TYPE(INT8, "int", 8);
    TYPE(INT16, "int", 16);
    TYPE(INT32, "int", 32);
    TYPE(UINT8, "uint", 8);
    TYPE(UINT16, "uint", 16);
    TYPE(UINT32, "uint", 32);
    TYPE(FLOAT32, "float", 32);
    TYPE(FLOAT64, "float", 64);
#undef TYPE
    case tinyply::Type::INVALID:
      error("invalid ply type");
    default:
      error("Got unknown tinyply type");
  }
}

static tinyply::Type fromVoxieType(
    const std::tuple<QString, quint32, QString>& t) {
  // Note: This assumes that the endianness is always correct
#define TYPE(TinyplyType, VoxieType, BitCount)                   \
  if (std::get<0>(t) == VoxieType && std::get<1>(t) == BitCount) \
    return tinyply::Type::TinyplyType;
  TYPE(INT8, "int", 8);
  TYPE(INT16, "int", 16);
  TYPE(INT32, "int", 32);
  TYPE(UINT8, "uint", 8);
  TYPE(UINT16, "uint", 16);
  TYPE(UINT32, "uint", 32);
  TYPE(FLOAT32, "float", 32);
  TYPE(FLOAT64, "float", 64);
#undef TYPE
  qWarning() << "Got unknown type for PLY export:" << std::get<0>(t)
             << std::get<1>(t) << std::get<2>(t);
  return tinyply::Type::INVALID;
}

template <typename T>
static bool addVertices(tinyply::PlyData*, const vx::Array2<float>& vertices);
template <typename T>
static bool addFaces(tinyply::PlyData*, const vx::Array2<quint32>& triangles);

QCommandLineOption voxieImportFilename("voxie-import-filename", "File to load.",
                                       "filename");
// QCommandLineOption voxieExportFilename("voxie-export-filename", "File to
// save.",
//                                        "filename");

static void import(const QCommandLineParser& parser) {
  if (!parser.isSet(vx::ClaimedOperationBase::voxieOperationOption()))
    error("--voxie-operation is not set");
  QString operationPath =
      parser.value(vx::ClaimedOperationBase::voxieOperationOption());
  if (!parser.isSet(voxieImportFilename))
    error("--voxie-import-filename is not set");
  QString filename = parser.value(voxieImportFilename);

  vx::initDBusTypes();

  vx::DBusClient dbusClient(parser);

  vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport> op(
      dbusClient, QDBusObjectPath(operationPath));

  op.forwardExc([&]() {
    // Step 1: load ply file using tinyply
    std::ifstream ss(filename.toStdString(), std::ios::binary);
    if (ss.fail()) {
      throw vx::Exception("de.uni_stuttgart.Voxie.PLYImporter.FileNotFound",
                          "File not found");
    }

    tinyply::PlyFile ply_file;
    std::shared_ptr<tinyply::PlyData> vertices, faces;
    QMap<QString,
         std::tuple<tinyply::Type, std::shared_ptr<tinyply::PlyData>, size_t>>
        vertexPropertiesData;
    QMap<QString,
         std::tuple<tinyply::Type, std::shared_ptr<tinyply::PlyData>, size_t>>
        facePropertiesData;
    try {
      ply_file.parse_header(ss);

      QString vertexIndexName = "vertex_indices";

      auto elements = ply_file.get_elements();
      const tinyply::PlyElement* vertex = nullptr;
      const tinyply::PlyElement* face = nullptr;
      for (const auto& elem : elements) {
        if (elem.name == "vertex") {
          if (vertex) error("Multiple 'vertex' elements");
          vertex = &elem;
        }
        if (elem.name == "face") {
          if (face) error("Multiple 'face' elements");
          face = &elem;
        }
        /*
        qDebug() << "E" << QString(elem.name.c_str()) << elem.size;
        for (const auto& prop : elem.properties) {
          qDebug() << " P" << QString(prop.name.c_str())
                   << (int)prop.propertyType << prop.isList
                   << (int)prop.listType << prop.listCount;
        }
        */
      }
      if (!vertex) error("No 'vertex' element found");
      if (!face) error("No 'face' element found");

      QList<QString> vertexProperties;
      QMap<QString, const tinyply::PlyProperty*> vertexPropertyMap;
      QList<QString> vertexPropertiesComp;
      for (const auto& prop : vertex->properties) {
        if (prop.name == "x" || prop.name == "y" || prop.name == "z") {
          // Do nothing
        } else {
          QString origName = prop.name.c_str();

          if (prop.isList) {
            qWarning() << "Ignoring list vertex property" << origName;
            continue;
          }

          QString name = origName;
          if (!origName.contains('.'))
            "de.uni_stuttgart.Voxie.FileFormat.Ply.VertexAttribute." + origName;
          for (const auto& pair : voxieVertexAttributes)
            if (std::get<1>(pair) == origName) name = std::get<0>(pair);
          // qDebug() << "PROP" << origName << name;
          vertexProperties << name;
          vertexPropertyMap[name] = &prop;
          if (name.endsWith(".0"))
            vertexPropertiesComp << name.left(name.length() - 2);
        }
      }

      QString vertexIndicesName = "";
      QList<QString> faceProperties;
      QMap<QString, const tinyply::PlyProperty*> facePropertyMap;
      QList<QString> facePropertiesComp;
      for (const auto& prop : face->properties) {
        if (prop.name == "vertex_indices" || prop.name == "vertex_index") {
          vertexIndicesName = prop.name.c_str();
        } else {
          QString origName = prop.name.c_str();

          if (prop.isList) {
            qWarning() << "Ignoring list face property" << origName;
            continue;
          }

          QString name = origName;
          if (!origName.contains('.'))
            "de.uni_stuttgart.Voxie.FileFormat.Ply.FaceAttribute." + origName;
          for (const auto& pair : voxieSurfaceAttributes)
            if (std::get<1>(pair) == origName) name = std::get<0>(pair);
          // qDebug() << "PROP" << origName << name;
          faceProperties << name;
          facePropertyMap[name] = &prop;
          if (name.endsWith(".0"))
            facePropertiesComp << name.left(name.length() - 2);
        }
      }
      if (vertexIndicesName == "")
        error("No 'vertex_indices' or 'vertex_index' face property found");

      vertices =
          ply_file.request_properties_from_element("vertex", {"x", "y", "z"});
      // TODO: 'vertex_indices' might also be 'vertex_index'
      faces = ply_file.request_properties_from_element(
          "face", {vertexIndicesName.toStdString()});

      // TODO: Reduce code duplication for vertex and face attributes?

      // Compound attributes
      for (const auto& name : vertexPropertiesComp) {
        auto prop0 = vertexPropertyMap[name + ".0"];
        if (!prop0) error("!prop0");
        auto type = prop0->propertyType;
        size_t count = 0;
        while (vertexPropertyMap.contains(name + "." + QString::number(count)))
          count++;
        bool haveError = false;
        std::vector<std::string> keys;
        for (size_t i = 0; i < count; i++) {
          auto pname = name + "." + QString::number(i);
          auto prop = vertexPropertyMap[pname];
          if (!prop) error("!prop");
          if (prop->propertyType != type) {
            qWarning() << "Got type mismatch between" << (name + ".0") << "and"
                       << pname;
            haveError = true;
          }
          keys.push_back(prop->name);
        }
        if (haveError) continue;
        for (size_t i = 0; i < count; i++) {
          auto pname = name + "." + QString::number(i);
          if (!vertexProperties.removeOne(pname))
            error("!vertexProperties.removeOne(pname)");
        }
        // qDebug() << name << count << (int)type;
        auto data = ply_file.request_properties_from_element("vertex", keys);
        if (type != data->t) error("type != data->t");
        vertexPropertiesData.insert(name, std::make_tuple(type, data, count));
      }
      // Non-compound attributes
      for (const auto& name : vertexProperties) {
        auto prop = vertexPropertyMap[name];
        if (!prop) error("!prop");
        auto type = prop->propertyType;
        // qDebug() << name << (int)type;
        auto data =
            ply_file.request_properties_from_element("vertex", {prop->name});
        if (type != data->t) error("type != data->t");
        vertexPropertiesData.insert(name, std::make_tuple(type, data, 1));
      }

      // Compound attributes
      for (const auto& name : facePropertiesComp) {
        auto prop0 = facePropertyMap[name + ".0"];
        if (!prop0) error("!prop0");
        auto type = prop0->propertyType;
        size_t count = 0;
        while (facePropertyMap.contains(name + "." + QString::number(count)))
          count++;
        bool haveError = false;
        std::vector<std::string> keys;
        for (size_t i = 0; i < count; i++) {
          auto pname = name + "." + QString::number(i);
          auto prop = facePropertyMap[pname];
          if (!prop) error("!prop");
          if (prop->propertyType != type) {
            qWarning() << "Got type mismatch between" << (name + ".0") << "and"
                       << pname;
            haveError = true;
          }
          keys.push_back(prop->name);
        }
        if (haveError) continue;
        for (size_t i = 0; i < count; i++) {
          auto pname = name + "." + QString::number(i);
          if (!faceProperties.removeOne(pname))
            error("!faceProperties.removeOne(pname)");
        }
        // qDebug() << name << count << (int)type;
        auto data = ply_file.request_properties_from_element("face", keys);
        if (type != data->t) error("type != data->t");
        facePropertiesData.insert(name, std::make_tuple(type, data, count));
      }
      // Non-compound attributes
      for (const auto& name : faceProperties) {
        auto prop = facePropertyMap[name];
        if (!prop) error("!prop");
        auto type = prop->propertyType;
        // qDebug() << name << (int)type;
        auto data =
            ply_file.request_properties_from_element("face", {prop->name});
        if (type != data->t) error("type != data->t");
        facePropertiesData.insert(name, std::make_tuple(type, data, 1));
      }

      ply_file.read(ss);
    } catch (const std::runtime_error& e) {
      error(QString() + "tinyply exception: " + e.what());
    } catch (const std::invalid_argument& e) {
      error(QString() + "tinyply exception: " + e.what());
    }

    // Step 2: add faces (triangles) into surface
    vx::RefObjWrapper<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>
        srf2t(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(dbusClient->CreateSurfaceDataTriangleIndexed(
                dbusClient.clientPath(), faces->count, 0, QDBusObjectPath("/"),
                true, QMap<QString, QDBusVariant>())));
    auto srf2t_data = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
        dbusClient.uniqueName(), srf2t.path().path(), dbusClient.connection());

    {
      vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
          dbusClient,
          HANDLEDBUSPENDINGREPLY(srf2t_data->CreateUpdate(
              dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      vx::Array2<uint32_t> srf2t_triangles(
          HANDLEDBUSPENDINGREPLY(srf2t->GetTrianglesWritable(
              update.path(), QMap<QString, QDBusVariant>())));

      switchPlyType(faces, [&](const auto* ptr, size_t count) {
        // Note: tinyply does not seem to indicate mixed lengths in any way,
        // just hope that it is triangle data if it has faces->count*3
        // entries
        if (count % 3 != 0)
          error(
              "Number of vertex indicies not multiple of 3, not triangle "
              "data?");
        if (count / 3 != faces->count)
          error(
              "Number of vertex indicies not 3x number of faces, not "
              "triangle data?");
        for (size_t i = 0; i < faces->count; i++)
          for (size_t j = 0; j < 3; j++) srf2t_triangles(i, j) = ptr[i * 3 + j];
      });

      vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion> version(
          dbusClient,
          HANDLEDBUSPENDINGREPLY(update->Finish(
              dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
    }

    // Step 3: add vertices and attributes into surface
    // Note: CreateSurfaceDataTriangleIndexed() will throw if there are
    // invalid vertex indices
    QList<std::tuple<QString, QString, quint64,
                     std::tuple<QString, quint32, QString>, QString,
                     QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>
        attributes;
    for (const auto& name : vertexPropertiesData.keys()) {
      auto value = vertexPropertiesData[name];
      auto data = std::get<1>(value);
      auto count = std::get<2>(value);
      attributes << std::make_tuple(
          name, "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex", count,
          getVoxieType(data->t), name /*TODO*/, QMap<QString, QDBusVariant>(),
          QMap<QString, QDBusVariant>());
    }
    for (const auto& name : facePropertiesData.keys()) {
      auto value = facePropertiesData[name];
      auto data = std::get<1>(value);
      auto count = std::get<2>(value);
      attributes << std::make_tuple(
          name, "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle", count,
          getVoxieType(data->t), name /*TODO*/, QMap<QString, QDBusVariant>(),
          QMap<QString, QDBusVariant>());
    }
    QMap<QString, QDBusVariant> options;
    options.insert("Attributes", vx::dbusMakeVariant(attributes));
    vx::RefObjWrapper<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>
        srf2(
            dbusClient,
            HANDLEDBUSPENDINGREPLY(dbusClient->CreateSurfaceDataTriangleIndexed(
                dbusClient.clientPath(), faces->count, vertices->count,
                srf2t.path(), false, options)));
    auto srf2_data = makeSharedQObject<de::uni_stuttgart::Voxie::Data>(
        dbusClient.uniqueName(), srf2.path().path(), dbusClient.connection());
    QSharedPointer<vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
        srf2_version;
    {
      vx::RefObjWrapper<de::uni_stuttgart::Voxie::ExternalDataUpdate> update(
          dbusClient,
          HANDLEDBUSPENDINGREPLY(srf2_data->CreateUpdate(
              dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
      vx::Array2<float> srf2_vertices(
          HANDLEDBUSPENDINGREPLY(srf2->GetVerticesWritable(
              update.path(), QMap<QString, QDBusVariant>())));

      switchPlyType(vertices, [&](const auto* ptr, size_t count) {
        // Should never happen (not a list)
        if (count % 3 != 0) error("Number of entries not multiple of 3");
        if (count / 3 != vertices->count)
          error("Number of entries not 3x number of vertices");
        for (size_t i = 0; i < vertices->count; i++)
          for (size_t j = 0; j < 3; j++) srf2_vertices(i, j) = ptr[i * 3 + j];
      });

      for (const auto& name : vertexPropertiesData.keys()) {
        auto value = vertexPropertiesData[name];
        auto data = std::get<1>(value);
        auto componentCount = std::get<2>(value);
        switchPlyType(data, [&](const auto* ptr, size_t count) {
          typedef typename std::remove_cv<
              typename std::remove_reference<decltype(*ptr)>::type>::type T;
          vx::Array2<T> attrData(
              HANDLEDBUSPENDINGREPLY(srf2->GetAttributeWritable(
                  update.path(), name, QMap<QString, QDBusVariant>())));

          if (attrData.template size<0>() * componentCount != count)
            error("attrData.template size<0>() * componentCount != count");
          if (attrData.template size<1>() != componentCount)
            error("attrData.template size<1>() != componentCount");

          for (size_t i = 0; i < attrData.template size<0>(); i++)
            for (size_t j = 0; j < attrData.template size<1>(); j++)
              attrData(i, j) = ptr[i * componentCount + j];
        });
      }

      for (const auto& name : facePropertiesData.keys()) {
        auto value = facePropertiesData[name];
        auto data = std::get<1>(value);
        auto componentCount = std::get<2>(value);
        switchPlyType(data, [&](const auto* ptr, size_t count) {
          typedef typename std::remove_cv<
              typename std::remove_reference<decltype(*ptr)>::type>::type T;
          vx::Array2<T> attrData(
              HANDLEDBUSPENDINGREPLY(srf2->GetAttributeWritable(
                  update.path(), name, QMap<QString, QDBusVariant>())));

          if (attrData.template size<0>() * componentCount != count)
            error("attrData.template size<0>() * componentCount != count");
          if (attrData.template size<1>() != componentCount)
            error("attrData.template size<1>() != componentCount");

          for (size_t i = 0; i < attrData.template size<0>(); i++)
            for (size_t j = 0; j < attrData.template size<1>(); j++)
              attrData(i, j) = ptr[i * componentCount + j];
        });
      }

      srf2_version = createQSharedPointer<
          vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>(
          dbusClient,
          HANDLEDBUSPENDINGREPLY(update->Finish(
              dbusClient.clientPath(), QMap<QString, QDBusVariant>())));
    }

    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(1.0, vx::emptyOptions()));

    HANDLEDBUSPENDINGREPLY(op.op().Finish(QDBusObjectPath(srf2.path()),
                                          QDBusObjectPath(srf2_version->path()),
                                          vx::emptyOptions()));
  });
}

static void exportFile(const QCommandLineParser& parser) {
  if (!parser.isSet(vx::ClaimedOperationBase::voxieOperationOption()))
    error("--voxie-operation is not set");
  QString operationPath =
      parser.value(vx::ClaimedOperationBase::voxieOperationOption());
  // if (!parser.isSet(voxieExportFilename))
  //   error("--voxie-export-filename is not set");
  // QString filename = parser.value(voxieExportFilename);

  vx::initDBusTypes();

  vx::DBusClient dbusClient(parser);

  vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationExport> op(
      dbusClient, QDBusObjectPath(operationPath));

  op.forwardExc([&]() {
    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(0.1, vx::emptyOptions()));

    QString filename = op.op().filename();
    auto inputDataPath = op.op().data();

    auto surface =
        makeSharedQObject<de::uni_stuttgart::Voxie::SurfaceDataTriangleIndexed>(
            dbusClient.uniqueName(), inputDataPath.path(),
            dbusClient.connection());
    auto surfaceBase = makeSharedQObject<de::uni_stuttgart::Voxie::SurfaceData>(
        dbusClient.uniqueName(), inputDataPath.path(), dbusClient.connection());

    vx::Array2<const uint32_t> triangles(HANDLEDBUSPENDINGREPLY(
        surface->GetTrianglesReadonly(QMap<QString, QDBusVariant>())));
    vx::Array2<const float> vertices(HANDLEDBUSPENDINGREPLY(
        surface->GetVerticesReadonly(QMap<QString, QDBusVariant>())));
    auto attributes = surfaceBase->attributes();

    // TODO: Make this configurable?
    bool binary = true;

    std::filebuf fb;
    if (!fb.open(filename.toStdString(),
                 binary ? (std::ios::out | std::ios::binary) : std::ios::out))
      error("Failed to open '" + filename + "'");
    std::ostream stream(&fb);
    if (!stream.good()) error("Opening stream failed");

    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(0.2, vx::emptyOptions()));

    QList<std::shared_ptr<void>> attributeData;

    tinyply::PlyFile plyFile;
    try {
      plyFile.get_comments().push_back("Generated by Voxie");

      if (vertices.strideBytes<0>() != 3 * sizeof(float))
        error("vertices.strideBytes<0>() != 3 * sizeof(float)");
      if (vertices.strideBytes<1>() != sizeof(float))
        error("vertices.strideBytes<1>() != sizeof(float)");
      // Note: The const_cast should be ok as long as the buffer is not modified
      plyFile.add_properties_to_element(
          "vertex", {"x", "y", "z"}, tinyply::Type::FLOAT32, vertices.size<0>(),
          const_cast<uint8_t*>(
              reinterpret_cast<const uint8_t*>(vertices.data())),
          tinyply::Type::INVALID, 0);

      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(0.25, vx::emptyOptions()));

      if (triangles.strideBytes<0>() != 3 * sizeof(uint32_t))
        error("triangles.strideBytes<0>() != 3 * sizeof(uint32_t)");
      if (triangles.strideBytes<1>() != sizeof(uint32_t))
        error("triangles.strideBytes<1>() != sizeof(uint32_t)");
      // Note: The const_cast should be ok as long as the buffer is not modified
      plyFile.add_properties_to_element(
          "face", {"vertex_indices"}, tinyply::Type::UINT32,
          triangles.size<0>(),
          const_cast<uint8_t*>(
              reinterpret_cast<const uint8_t*>(triangles.data())),
          tinyply::Type::UINT8, 3);

      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(0.3, vx::emptyOptions()));

      for (const auto& attribute : attributes) {
        auto name = std::get<0>(attribute);
        auto kind = std::get<1>(attribute);
        auto componentCount = std::get<2>(attribute);
        auto voxieType = std::get<3>(attribute);

        bool isFace;
        if (kind == "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Triangle") {
          isFace = true;
        } else if (kind ==
                   "de.uni_stuttgart.Voxie.SurfaceAttributeKind.Vertex") {
          isFace = false;
        } else {
          qDebug() << "Ply exportFile(): Unknown surface attribute kind:"
                   << kind;
          continue;
        }

        auto type = fromVoxieType(voxieType);

        std::vector<std::string> keys;
        for (size_t i = 0; i < componentCount; i++) {
          QString oname;
          if (componentCount == 1)
            oname = name;
          else
            oname = name + "." + QString::number(i);

          QString plyName = oname;
          const char* prefix;
          if (isFace)
            prefix = "de.uni_stuttgart.Voxie.FileFormat.Ply.FaceAttribute.";
          else
            prefix = "de.uni_stuttgart.Voxie.FileFormat.Ply.VertexAttribute.";
          if (oname.startsWith(prefix)) plyName = oname.mid(strlen(prefix));
          for (const auto& pair : voxieSurfaceAttributes)
            if (std::get<0>(pair) == oname) plyName = std::get<1>(pair);

          keys.push_back(plyName.toStdString());
        }

        // For some reason MSVC2015 complains that "error C2065:
        // 'componentCount': undeclared identifier" when componentCount is not
        // listed explicitly
        switchPlyTypeSimple(type, [&, componentCount](auto val) {
          typedef typename std::remove_cv<
              typename std::remove_reference<decltype(val)>::type>::type T;

          auto data = std::make_shared<vx::Array2<const T>>(
              HANDLEDBUSPENDINGREPLY(surface->GetAttributeReadonly(
                  name, QMap<QString, QDBusVariant>())));
          attributeData << data;  // Keep the array alive

          if (data->template size<1>() != componentCount)
            error("data->template size<1>() != componentCount");

          if (data->template strideBytes<0>() !=
              (ptrdiff_t)componentCount * (ptrdiff_t)sizeof(T))
            error(
                "data->template strideBytes<0>() != (ptrdiff_t)componentCount "
                "* (ptrdiff_t)sizeof(T)");
          if (data->template strideBytes<1>() != (ptrdiff_t)sizeof(T))
            error("data->template strideBytes<1>() != (ptrdiff_t)sizeof(T)");

          // Note: The const_cast should be ok as long as the buffer is not
          // modified
          plyFile.add_properties_to_element(
              isFace ? "face" : "vertex", keys, type, data->template size<0>(),
              const_cast<uint8_t*>(
                  reinterpret_cast<const uint8_t*>(data->data())),
              tinyply::Type::INVALID, 0);
        });
      }

      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(0.4, vx::emptyOptions()));

      plyFile.write(stream, binary);

      HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(0.9, vx::emptyOptions()));

      if (!stream.good()) error("Writing to stream failed");
    } catch (const std::runtime_error& e) {
      error(QString() + "tinyply exception: " + e.what());
    } catch (const std::invalid_argument& e) {
      error(QString() + "tinyply exception: " + e.what());
    }

    HANDLEDBUSPENDINGREPLY(op.op().Finish(vx::emptyOptions()));
  });
}

int main(int argc, char* argv[]) {
  try {
    if (argc < 1) error("argc is smaller than 1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Voxie script for loading PLY files");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions(vx::DBusClient::options());
    parser.addOptions(vx::ClaimedOperationBase::options());
    parser.addOption(voxieImportFilename);

    QStringList args;
    for (char** arg = argv; *arg; arg++) args.push_back(*arg);
    int argc0 = 1;
    char* args0[2] = {argv[0], NULL};
    QCoreApplication app(argc0, args0);
    parser.process(args);

    QString action = "";
    if (parser.isSet(vx::ClaimedOperationBase::voxieActionOption()))
      action = parser.value(vx::ClaimedOperationBase::voxieActionOption());
    if (action == "Import")
      import(parser);
    else if (action == "Export")
      exportFile(parser);
    else
      error("--voxie-action is not 'Import' or 'Export'");

    return 0;
  } catch (vx::Exception& error) {
    QTextStream(stderr) << error.name() << ": " << error.message() << endl
                        << flush;
    return 1;
  }
}
