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

#include "TomographyRawData2DAccessor.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

#include <VoxieBackend/DBus/ClientWrapper.hpp>
#include <VoxieBackend/DBus/ObjectWrapper.hpp>

#include <VoxieBackend/Data/ImageDataPixelInst.hpp>
#include <VoxieBackend/Data/TomographyRawData2DRegular.hpp>

#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/JsonDBus.hpp>

#include <VoxieClient/ObjectExport/BusManager.hpp>
#include <VoxieClient/ObjectExport/DBusCallUtil.hpp>

using namespace vx;

GeometryEntry::GeometryEntry(GeometryEntry* parent, int pos,
                             const QString& name,
                             const QJsonObject& localGeometryData)
    : parent_(parent),
      pos_(pos),
      name_(name),
      localGeometryData_(localGeometryData) {
  if (!parent) {
    fullName_ = "";
    nameComponents_ = {};
  } else {
    if (parent->fullName() == "")
      fullName_ = "";
    else
      fullName_ = parent->fullName() + " / ";
    nameComponents_ = parent->nameComponents();

    if (dynamic_cast<GeometryEntryArray*>(parent)) {
      fullName_ += QString::number(pos);
      nameComponents_ << pos;
    } else {
      fullName_ += "'" + name + "'";
      nameComponents_ << name;
    }
  }
}
GeometryEntry::~GeometryEntry() {}

int GeometryEntry::pos() {
  if (!dynamic_cast<GeometryEntryArray*>(parent()))
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "Attempting to call GeometryEntry::pos() on child of non-array");
  return pos_;
}
QString GeometryEntry::name() {
  if (!dynamic_cast<GeometryEntryObject*>(parent()))
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "Attempting to call GeometryEntry::pos() on child of non-object");
  return name_;
}

QSharedPointer<GeometryEntry> GeometryEntry::parse(const QJsonValue& json,
                                                   GeometryEntry* parent,
                                                   int pos,
                                                   const QString& name) {
  // qDebug() << "GeometryEntry::parse" << json.isArray() << json.isObject()
  //          << parent << pos << name;
  if (json.isArray()) {
    return createQSharedPointer<GeometryEntryArray>(json.toArray(), parent, pos,
                                                    name);
  } else if (json.isObject()) {
    auto obj = json.toObject();
    if (obj.contains("ImageReference"))
      return createQSharedPointer<GeometryEntryImage>(obj, parent, pos, name);
    else
      return createQSharedPointer<GeometryEntryObject>(obj, parent, pos, name);
  } else {
    return QSharedPointer<GeometryEntry>();
  }
}

GeometryEntryObject::GeometryEntryObject(const QJsonObject& json,
                                         GeometryEntry* parent, int pos,
                                         const QString& name)
    : GeometryEntry(parent, pos, name, json["ProjectionGeometry"].toObject()) {
  for (const auto& key : json.keys()) {
    if (key == "ImageReference") continue;
    auto entry = GeometryEntry::parse(json[key], this, -1, key);
    if (!entry) continue;
    children_[key] = entry;
  }
}

GeometryEntry* GeometryEntryObject::operator[](const QString& name) {
  if (!children_.contains(name))
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Could not find entry in GeometryEntryObject: " + name);
  return children_[name].data();
}

void GeometryEntryObject::createImageLists(
    const QSharedPointer<GeometryEntry>& root,
    QList<QSharedPointer<GeometryImageList>>& outLists) {
  for (const auto& key : children_.keys()) {
    children_[key]->createImageLists(root, outLists);
  }
}

GeometryEntryArray::GeometryEntryArray(const QJsonArray& json,
                                       GeometryEntry* parent, int pos,
                                       const QString& name)
    : GeometryEntry(parent, pos, name, QJsonObject{}) {
  hasImageChildren_ = false;
  for (int i = 0; i < json.count(); i++) {
    auto entry = GeometryEntry::parse(json[i], this, i, "");
    // Also add nullptr entries
    children_.append(entry);
    if (dynamic_cast<GeometryEntryImage*>(entry.data()))
      hasImageChildren_ = true;
  }
}

GeometryEntry* GeometryEntryArray::operator[](int pos) {
  if (pos < 0 || pos >= count())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "Index out of range for GeometryEntryArray: " + QString::number(pos));
  return children_[pos].data();
}

void GeometryEntryArray::createImageLists(
    const QSharedPointer<GeometryEntry>& root,
    QList<QSharedPointer<GeometryImageList>>& outLists) {
  if (hasImageChildren())
    outLists.append(createQSharedPointer<GeometryImageListArray>(root, this));

  for (const auto& entry : children_) {
    if (entry) entry->createImageLists(root, outLists);
  }
}

GeometryEntryImage::GeometryEntryImage(const QJsonObject& json,
                                       GeometryEntry* parent, int pos,
                                       const QString& name)
    : GeometryEntryObject(json, parent, pos, name) {
  if (!json.contains("ImageReference"))
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "GeometryEntryImage::GeometryEntryImage(): No ImageReference member");
  auto ir = json["ImageReference"].toObject();

  if (!ir.contains("Stream"))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "GeometryEntryImage::GeometryEntryImage(): Got "
                        "ImageReference without Stream member");
  if (!ir.contains("ImageID"))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "GeometryEntryImage::GeometryEntryImage(): Got "
                        "ImageReference without ImageID member");
  stream_ = ir["Stream"].toString();
  id_ = ir["ImageID"].toInt();
}

QJsonObject GeometryEntryImage::geometry() {
  QJsonObject result{};

  for (GeometryEntry* obj = this; obj; obj = obj->parent()) {
    for (const auto& key : obj->localGeometryData().keys()) {
      if (result.contains(key)) continue;
      result[key] = obj->localGeometryData()[key];
    }
  }

  return result;
}

void GeometryEntryImage::createImageLists(
    const QSharedPointer<GeometryEntry>& root,
    QList<QSharedPointer<GeometryImageList>>& outLists) {
  if (!dynamic_cast<GeometryEntryArray*>(parent())) {
    // Only add this if parent is not an array
    outLists.append(createQSharedPointer<GeometryImageListSingle>(root, this));
  }

  GeometryEntryObject::createImageLists(root, outLists);
}

GeometryImageList::GeometryImageList(const QSharedPointer<GeometryEntry>& root,
                                     GeometryEntry* entry)
    : root(root), entry_(entry) {}
GeometryImageList::~GeometryImageList() {}

GeometryImageListSingle::GeometryImageListSingle(
    const QSharedPointer<GeometryEntry>& root, GeometryEntryImage* entry)
    : GeometryImageList(root, entry), image_(entry) {}

int GeometryImageListSingle::count() { return 1; }
GeometryEntryImage* GeometryImageListSingle::operator[](int pos) {
  if (pos != 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "Index out of range for GeometryImageListSingle: " +
                            QString::number(pos));
  return image_;
}

GeometryImageListArray::GeometryImageListArray(
    const QSharedPointer<GeometryEntry>& root, GeometryEntryArray* array)
    : GeometryImageList(root, array), array_(array) {}

int GeometryImageListArray::count() { return array_->count(); }
GeometryEntryImage* GeometryImageListArray::operator[](int pos) {
  return dynamic_cast<GeometryEntryImage*>((*array_)[pos]);
}

class TomographyRawData2DAccessorOperationsImpl
    : public TomographyRawData2DAccessorOperationsAdaptor {
  TomographyRawData2DAccessor* object;

 public:
  TomographyRawData2DAccessorOperationsImpl(TomographyRawData2DAccessor* object)
      : TomographyRawData2DAccessorOperationsAdaptor(object), object(object) {}

  QStringList GetAvailableGeometryTypes(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return vx::handleDBusCallOnBackgroundThreadOne<QStringList>(
          object, [self = object->thisShared()]() {
            return self->availableGeometryTypes();
          });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
  QList<QMap<QString, QDBusVariant>> GetAvailableImageKinds(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return vx::handleDBusCallOnBackgroundThreadOne<
          QList<QMap<QString, QDBusVariant>>>(
          object, [self = object->thisShared()]() {
            QList<QMap<QString, QDBusVariant>> result;
            for (const auto& kind : self->availableImageKinds())
              result << vx::jsonToDBus(kind);
            return result;
          });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
  QStringList GetAvailableStreams(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return vx::handleDBusCallOnBackgroundThreadOne<QStringList>(
          object,
          [self = object->thisShared()]() { return self->availableStreams(); });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
  QMap<QString, QDBusVariant> GetGeometryData(
      const QString& geometryType,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return vx::handleDBusCallOnBackgroundThreadOne<
          QMap<QString, QDBusVariant>>(
          object, [self = object->thisShared(), geometryType]() {
            return vx::jsonToDBus(self->getGeometryData(geometryType));
          });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
  std::tuple<quint64, quint64> GetImageShape(
      const QString& stream, qulonglong id,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return vx::handleDBusCallOnBackgroundThreadOne<
          std::tuple<quint64, quint64>>(
          object, [self = object->thisShared(), stream, id]() {
            return self->imageSize(stream, id).toTupleVector();
          });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
  QMap<QString, QDBusVariant> GetMetadata(
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return vx::handleDBusCallOnBackgroundThreadOne<
          QMap<QString, QDBusVariant>>(object, [self = object->thisShared()]() {
        return vx::jsonToDBus(self->metadata());
      });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
  quint64 GetNumberOfImages(
      const QString& stream,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return vx::handleDBusCallOnBackgroundThreadOne<quint64>(
          object, [self = object->thisShared(), stream]() {
            return self->numberOfImages(stream);
          });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
  QMap<QString, QDBusVariant> GetPerImageMetadata(
      const QString& stream, qulonglong id,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options);

      return vx::handleDBusCallOnBackgroundThreadOne<
          QMap<QString, QDBusVariant>>(
          object, [self = object->thisShared(), stream, id]() {
            return vx::jsonToDBus(self->getPerImageMetadata(stream, id));
          });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
  QString ReadImages(const QMap<QString, QDBusVariant>& imageKind,
                     const QList<std::tuple<QString, quint64>>& images,
                     const std::tuple<quint64, quint64>& inputRegionStart,
                     const std::tuple<QString, QDBusObjectPath>& output,
                     qulonglong firstOutputImageId,
                     const std::tuple<quint64, quint64>& outputRegionStart,
                     const std::tuple<quint64, quint64>& regionSize,
                     const QMap<QString, QDBusVariant>& options) override {
    try {
      ExportedObject::checkOptions(options, "AllowIncompleteData");

      auto allowIncompleteData = ExportedObject::getOptionValueOrDefault<bool>(
          options, "AllowIncompleteData", false);

      auto serviceVoxie = object->connection().baseService();

      if (std::get<0>(output) != serviceVoxie)
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            "Got invalid image reference: Expected reference to service '" +
                serviceVoxie + "', got '" + std::get<0>(output) + "'");
      auto outputObj =
          vx::TomographyRawData2DRegular::lookup(std::get<1>(output));

      return vx::handleDBusCallOnBackgroundThreadOne<QString>(
          object, [self = object->thisShared(), imageKind, images,
                   inputRegionStart, outputObj, firstOutputImageId,
                   outputRegionStart, regionSize, allowIncompleteData]() {
            return self->readImages(vx::dbusToJson(imageKind), images,
                                    inputRegionStart, outputObj,
                                    firstOutputImageId, outputRegionStart,
                                    regionSize, allowIncompleteData);
          });
    } catch (Exception& e) {
      e.handle(object);
      return vx::dbusDefaultReturnValue();
    }
  }
};

TomographyRawData2DAccessor::TomographyRawData2DAccessor() {
  new TomographyRawData2DAccessorOperationsImpl(this);
}
TomographyRawData2DAccessor::~TomographyRawData2DAccessor() {}

QList<QString> TomographyRawData2DAccessor::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.TomographyRawData2DAccessor",
      "de.uni_stuttgart.Voxie.TomographyRawDataAccessor",
      "de.uni_stuttgart.Voxie.TomographyRawDataBase",
  };
}

void TomographyRawData2DAccessor::readImage(
    const QString& stream, uint64_t id,
    const QSharedPointer<ImageDataPixel>& image, const QJsonObject& imageKind,
    const vx::VectorSizeT2& inputRegionStart,
    const vx::VectorSizeT2& outputRegionStart,
    const vx::VectorSizeT2& regionSize, bool allowIncompleteData) {
  this->readImages(imageKind,
                   {
                       std::make_tuple(stream, id),
                   },
                   inputRegionStart.toTupleVector(),
                   image->fakeTomographyRawData2DRegular(), 0,
                   outputRegionStart.toTupleVector(),
                   regionSize.toTupleVector(), allowIncompleteData);
}

QList<QSharedPointer<GeometryImageList>>
TomographyRawData2DAccessor::getImageLists(const QString& geometryType) {
  {
    QMutexLocker locker(&mutex);
    if (imageListCache.contains(geometryType))
      return imageListCache[geometryType];
  }

  auto geometry = getGeometryData(geometryType);
  auto root = GeometryEntry::parse(geometry, nullptr, -1, "");
  QList<QSharedPointer<GeometryImageList>> lists;
  if (root) root->createImageLists(root, lists);

  {
    QMutexLocker locker(&mutex);
    // Note: Another thread might have inserted a value
    if (imageListCache.contains(geometryType))
      return imageListCache[geometryType];
    imageListCache[geometryType] = lists;
    return imageListCache[geometryType];
  }
}

QSharedPointer<GeometryImageList> TomographyRawData2DAccessor::getImageList(
    const QString& geometryType, const QJsonArray& path) {
  // TODO: Avoid loop here?
  for (const auto& list : getImageLists(geometryType)) {
    if (list->entry()->nameComponents() == path) return list;
  }
  return QSharedPointer<GeometryImageList>();
}

QSharedPointer<QMap<std::tuple<QString, quint64>,
                    QList<std::tuple<QString, GeometryEntryImage*>>>>
TomographyRawData2DAccessor::getStreamToGeometryMap() {
  {
    QMutexLocker locker(&mutex);
    if (streamToGeometryCache) return streamToGeometryCache;
  }

  auto result = createQSharedPointer<
      QMap<std::tuple<QString, quint64>,
           QList<std::tuple<QString, GeometryEntryImage*>>>>();
  for (const auto& geometryType : availableGeometryTypes()) {
    // TODO: Probably should not use lists here but GeometryEntrys directly
    auto lists = getImageLists(geometryType);
    for (const auto& list : lists) {
      for (int i = 0; i < list->count(); i++) {
        auto entry = (*list)[i];
        if (!entry) continue;
        std::tuple<QString, quint64> key =
            std::make_tuple(entry->stream(), entry->id());
        (*result)[key].append(std::make_tuple(geometryType, entry));
      }
    }
  }

  {
    QMutexLocker locker(&mutex);
    // Note: Another thread might have inserted a value
    if (streamToGeometryCache) return streamToGeometryCache;
    streamToGeometryCache = result;
    return streamToGeometryCache;
  }
}

QList<std::tuple<QString, GeometryEntryImage*>>
TomographyRawData2DAccessor::mapStreamImageToGeometryImage(
    const QString& stream, uint64_t id) {
  auto map = getStreamToGeometryMap();
  std::tuple<QString, quint64> key = std::make_tuple(stream, id);
  if ((*map).contains(key))
    return (*map)[key];
  else
    return {};
}

QList<std::tuple<QString, QJsonObject>>
TomographyRawData2DAccessor::availableImageLists() {
  QList<std::tuple<QString, QJsonObject>> result;

  for (const auto& name : availableStreams()) {
    result << std::make_tuple(
        "de.uni_stuttgart.Voxie.TomographyRawDataImageListType.ImageStream",
        QJsonObject{
            {"StreamName", name},
        });
  }

  for (const auto& geometryType : availableGeometryTypes()) {
    for (const auto& list : getImageLists(geometryType)) {
      result << std::make_tuple(
          "de.uni_stuttgart.Voxie.TomographyRawDataImageListType."
          "GeometryImageList",
          QJsonObject{
              {"GeometryType", geometryType},
              {"Path", list->entry()->nameComponents()},
          });
    }
  }

  return result;
}

TomographyRawData2DAccessorDBus::TomographyRawData2DAccessorDBus(
    const QSharedPointer<BusConnection>& connection,
    const std::tuple<QString, QDBusObjectPath>& provider)
    : connection_(connection), provider_(provider) {
  auto client =
      createQSharedPointer<ClientWrapper>(connection, std::get<0>(provider));
  wrapper =
      createQSharedPointer<ObjectWrapper>(client, std::get<1>(provider), true);
  proxy = makeSharedQObject<
      de::uni_stuttgart::Voxie::TomographyRawData2DAccessorOperations>(
      std::get<0>(provider), std::get<1>(provider).path(),
      connection->connection());

  // TODO: really do this here?
  metadata_ = vx::dbusToJson(HANDLEDBUSPENDINGREPLY(
      proxy->GetMetadata(QMap<QString, QDBusVariant>())));
  for (const auto& kind : HANDLEDBUSPENDINGREPLY(
           proxy->GetAvailableImageKinds(QMap<QString, QDBusVariant>())))
    availableImageKinds_ << vx::dbusToJson(kind);
  availableStreams_ = HANDLEDBUSPENDINGREPLY(
      proxy->GetAvailableStreams(QMap<QString, QDBusVariant>()));
  availableGeometryTypes_ = HANDLEDBUSPENDINGREPLY(
      proxy->GetAvailableGeometryTypes(QMap<QString, QDBusVariant>()));
  for (const auto& stream : availableStreams_) {
    numberOfImages_[stream] = HANDLEDBUSPENDINGREPLY(
        proxy->GetNumberOfImages(stream, QMap<QString, QDBusVariant>()));
    // qDebug() << "numberOfImages" << numberOfImages_;
    for (uint64_t i = 0; i < numberOfImages_[stream]; i++) {
      imageSizes_[std::make_tuple(stream, i)] =
          VectorSizeT2(HANDLEDBUSPENDINGREPLY(
              proxy->GetImageShape(stream, i, QMap<QString, QDBusVariant>())));
    }

    QMap<uint64_t, QJsonObject> data;
    for (uint64_t i = 0; i < numberOfImages_[stream]; i++) {
      data[i] =
          vx::dbusToJson(HANDLEDBUSPENDINGREPLY(proxy->GetPerImageMetadata(
              stream, i, QMap<QString, QDBusVariant>())));
    }
    perImageMetadata_[stream] = data;
  }
  // Populate cache to avoid HANDLEDBUSPENDINGREPLY() / nested main loops later
  // on
  for (const auto& type : availableGeometryTypes_) {
    getGeometryData(type);
  }
}
TomographyRawData2DAccessorDBus::~TomographyRawData2DAccessorDBus() {}

uint64_t TomographyRawData2DAccessorDBus::numberOfImages(
    const QString& stream) {
  if (!numberOfImages_.contains(stream))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Invalid stream");
  return numberOfImages_[stream];
}

QString TomographyRawData2DAccessorDBus::readImages(
    const QJsonObject& imageKind,
    const QList<std::tuple<QString, quint64>>& images,
    const std::tuple<quint64, quint64>& inputRegionStart,
    const QSharedPointer<TomographyRawData2DRegular>& output,
    qulonglong firstOutputImageId,
    const std::tuple<quint64, quint64>& outputRegionStart,
    const std::tuple<quint64, quint64>& regionSize, bool allowIncompleteData) {
  for (const auto& entry : images) {
    const auto& stream = std::get<0>(entry);
    const auto& id = std::get<1>(entry);
    if (!perImageMetadata().contains(stream)) {
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Invalid stream '" + stream + "'");
    }
    if (id >= (uint64_t)perImageMetadata()[stream].size()) {
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Image ID " + QString::number(id) + " in stream '" +
                              stream + "' is out of range");
    }
  }

  QMap<QString, QDBusVariant> options;
  if (allowIncompleteData) {
    options["AllowIncompleteData"] = dbusMakeVariant<bool>(true);
    options["Optional"] =
        dbusMakeVariant<QList<QString>>({"AllowIncompleteData"});
  }

  return HANDLEDBUSPENDINGREPLY(proxy->ReadImages(
      jsonToDBus(imageKind), images, inputRegionStart,
      std::tuple<QString, QDBusObjectPath>(
          connection_->connection().baseService(), output->getPath()),
      firstOutputImageId, outputRegionStart, regionSize, options));
}

vx::VectorSizeT2 TomographyRawData2DAccessorDBus::imageSize(
    const QString& stream, uint64_t id) {
  auto pos = std::make_tuple(stream, id);
  if (!imageSizes().contains(pos))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Image ID " + QString::number(id) +
                            " is out of range for stream '" + stream + "'");
  return imageSizes()[pos];
}

QJsonObject TomographyRawData2DAccessorDBus::getPerImageMetadata(
    const QString& stream, uint64_t id) {
  if (!perImageMetadata_.contains(stream))
    throw vx::Exception("de.uni_stuttgart.Voxie.Error", "Invalid stream");
  const auto& data = perImageMetadata_[stream];
  if (id >= (uint64_t)data.size()) {
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Image ID " + QString::number(id) +
                            " is out of range for stream '" + stream + "'");
  }
  return data[id];
}

QJsonObject TomographyRawData2DAccessorDBus::getGeometryData(
    const QString& geometryType) {
  {
    QMutexLocker locker(&mutex);
    if (geometryDataChache.contains(geometryType))
      return geometryDataChache[geometryType];
  }
  auto data = HANDLEDBUSPENDINGREPLY(
      proxy->GetGeometryData(geometryType, QMap<QString, QDBusVariant>()));
  auto dataJson = dbusToJson(data);
  {
    QMutexLocker locker(&mutex);
    // Note: Another thread might have inserted a value
    if (geometryDataChache.contains(geometryType))
      return geometryDataChache[geometryType];
    geometryDataChache[geometryType] = dataJson;
    return geometryDataChache[geometryType];
  }
}

Q_NORETURN void TomographyRawData2DAccessor::throwMissingPerImageMetadata(
    const QString& stream, uint64_t id, const QString& name) {
  throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                      "No value given for per-image metadata '" + name +
                          "' for image " + QString::number(id) +
                          " in stream '" + stream + "'");
}
Q_NORETURN void TomographyRawData2DAccessor::throwInvalidPerImageMetadata(
    const QString& stream, uint64_t id, const QString& name,
    const QString& expected, const QString& actual) {
  throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                      "Invalid type for per-image metadata '" + name +
                          "' for image" + QString::number(id) + " in stream '" +
                          stream + "': got " + actual + ", expected " +
                          expected);
}

QList<QSharedPointer<SharedMemory>>
TomographyRawData2DAccessorDBus::getSharedMemorySections() {
  return {};
}
