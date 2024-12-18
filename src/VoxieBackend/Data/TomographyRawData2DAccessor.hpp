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

#pragma once

#include <VoxieBackend/Data/Data.hpp>

#include <VoxieBackend/DBus/DBusTypes.hpp>

#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/DBusTypeList.hpp>

namespace vx {
class ImageDataPixel;
class TomographyRawData2DRegular;

class ClientWrapper;
class ObjectWrapper;

class BusConnection;

class GeometryImageList;
class VOXIEBACKEND_EXPORT GeometryEntry {
  GeometryEntry* parent_;
  int pos_;
  QString name_;
  QString fullName_;
  QJsonArray nameComponents_;
  QJsonObject localGeometryData_;

 protected:
  GeometryEntry(GeometryEntry* parent, int pos, const QString& name,
                const QJsonObject& localGeometryData);
  virtual ~GeometryEntry();

 public:
  GeometryEntry* parent() const { return parent_; }
  int pos();
  QString name();
  const QString& fullName() { return fullName_; }
  const QJsonArray& nameComponents() { return nameComponents_; }
  const QJsonObject& localGeometryData() { return localGeometryData_; }

  static QSharedPointer<GeometryEntry> parse(const QJsonValue& json,
                                             GeometryEntry* parent, int pos,
                                             const QString& name);

  virtual void createImageLists(
      const QSharedPointer<GeometryEntry>& root,
      QList<QSharedPointer<GeometryImageList>>& outLists) = 0;
};

class VOXIEBACKEND_EXPORT GeometryEntryObject : public GeometryEntry {
  QMap<QString, QSharedPointer<GeometryEntry>> children_;

 public:
  GeometryEntryObject(const QJsonObject& json, GeometryEntry* parent, int pos,
                      const QString& name);

  QList<QString> keys() { return children_.keys(); }
  GeometryEntry* operator[](const QString& name);  // can return nullptr

  void createImageLists(
      const QSharedPointer<GeometryEntry>& root,
      QList<QSharedPointer<GeometryImageList>>& outLists) override;
};

class VOXIEBACKEND_EXPORT GeometryEntryArray : public GeometryEntry {
  QList<QSharedPointer<GeometryEntry>> children_;
  bool hasImageChildren_;

 public:
  GeometryEntryArray(const QJsonArray& json, GeometryEntry* parent, int pos,
                     const QString& name);

  bool hasImageChildren() const { return hasImageChildren_; }
  int count() { return children_.count(); }
  GeometryEntry* operator[](int pos);

  void createImageLists(
      const QSharedPointer<GeometryEntry>& root,
      QList<QSharedPointer<GeometryImageList>>& outLists) override;
};

class VOXIEBACKEND_EXPORT GeometryEntryImage : public GeometryEntryObject {
  QString stream_;
  uint64_t id_;

 public:
  GeometryEntryImage(const QJsonObject& json, GeometryEntry* parent, int pos,
                     const QString& name);

  const QString& stream() const { return stream_; }
  uint64_t id() const { return id_; }

  QJsonObject geometry();

  void createImageLists(
      const QSharedPointer<GeometryEntry>& root,
      QList<QSharedPointer<GeometryImageList>>& outLists) override;
};

class VOXIEBACKEND_EXPORT GeometryImageList {
  QSharedPointer<GeometryEntry> root;
  GeometryEntry* entry_;

 public:
  GeometryImageList(const QSharedPointer<GeometryEntry>& root,
                    GeometryEntry* entry);
  virtual ~GeometryImageList();

  GeometryEntry* entry() const { return entry_; }
  const QString& fullName() const { return entry()->fullName(); }

  virtual int count() = 0;
  virtual GeometryEntryImage* operator[](int pos) = 0;  // can return nullptr
};

class VOXIEBACKEND_EXPORT GeometryImageListSingle : public GeometryImageList {
  GeometryEntryImage* image_;

 public:
  GeometryImageListSingle(const QSharedPointer<GeometryEntry>& root,
                          GeometryEntryImage* image);

  int count() override;
  GeometryEntryImage* operator[](int pos) override;
};

class VOXIEBACKEND_EXPORT GeometryImageListArray : public GeometryImageList {
  GeometryEntryArray* array_;

 public:
  GeometryImageListArray(const QSharedPointer<GeometryEntry>& root,
                         GeometryEntryArray* array);

  int count() override;
  GeometryEntryImage* operator[](int pos) override;
};

class VOXIEBACKEND_EXPORT TomographyRawData2DAccessor : public Data {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  Q_NORETURN void throwMissingPerImageMetadata(const QString& stream,
                                               uint64_t id,
                                               const QString& name);
  Q_NORETURN void throwInvalidPerImageMetadata(const QString& stream,
                                               uint64_t id, const QString& name,
                                               const QString& expected,
                                               const QString& actual);

  QMutex mutex;
  QMap<QString, QList<QSharedPointer<GeometryImageList>>>
      imageListCache;  // protected by mutex
  QSharedPointer<QMap<std::tuple<QString, quint64>,
                      QList<std::tuple<QString, GeometryEntryImage*>>>>
      streamToGeometryCache;  // protected by mutex

  QSharedPointer<QMap<std::tuple<QString, quint64>,
                      QList<std::tuple<QString, GeometryEntryImage*>>>>
  getStreamToGeometryMap();

 public:
  TomographyRawData2DAccessor();
  ~TomographyRawData2DAccessor();

  QList<QString> supportedDBusInterfaces() override;

  virtual uint64_t numberOfImages(const QString& stream) = 0;
  virtual QJsonObject metadata() const = 0;
  virtual QList<QJsonObject> availableImageKinds() const = 0;
  virtual QList<QString> availableStreams() = 0;
  virtual bool hasStream(const QString& stream) = 0;
  virtual QList<QString> availableGeometryTypes() = 0;
  virtual vx::VectorSizeT2 imageSize(const QString& stream, uint64_t id) = 0;
  virtual QJsonObject getPerImageMetadata(const QString& stream,
                                          uint64_t id) = 0;
  virtual QJsonObject getGeometryData(const QString& geometryType) = 0;

  void readImage(const QString& stream, uint64_t id,
                 const QSharedPointer<ImageDataPixel>& image,
                 const QJsonObject& imageKind,
                 const vx::VectorSizeT2& inputRegionStart,
                 const vx::VectorSizeT2& outputRegionStart,
                 const vx::VectorSizeT2& regionSize,
                 bool allowIncompleteData = false);
  // TODO: Should this return a QSharedPointer<DataVersion>?
  virtual QString readImages(
      const QJsonObject& imageKind,
      const QList<std::tuple<QString, quint64>>& images,
      const std::tuple<quint64, quint64>& inputRegionStart,
      const QSharedPointer<TomographyRawData2DRegular>& output,
      qulonglong firstOutputImageId,
      const std::tuple<quint64, quint64>& outputRegionStart,
      const std::tuple<quint64, quint64>& regionSize,
      bool allowIncompleteData = false) = 0;

  QList<QSharedPointer<GeometryImageList>> getImageLists(
      const QString& geometryType);

  QSharedPointer<GeometryImageList> getImageList(const QString& geometryType,
                                                 const QJsonArray& path);

  QList<std::tuple<QString, GeometryEntryImage*>> mapStreamImageToGeometryImage(
      const QString& stream, uint64_t id);

  QList<std::tuple<QString, QJsonObject>> availableImageLists();
};

class VOXIEBACKEND_EXPORT TomographyRawData2DAccessorDBus
    : public TomographyRawData2DAccessor {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

  QSharedPointer<BusConnection> connection_;
  std::tuple<QString, QDBusObjectPath> provider_;
  QSharedPointer<
      de::uni_stuttgart::Voxie::TomographyRawData2DAccessorOperations>
      proxy;
  QSharedPointer<ObjectWrapper> wrapper;

  QMap<QString, uint64_t> numberOfImages_;
  // TODO: Convert metadata to JSON
  QJsonObject metadata_;
  QList<QJsonObject> availableImageKinds_;
  QMap<std::tuple<QString, uint64_t>, vx::VectorSizeT2> imageSizes_;
  QMap<QString, QMap<uint64_t, QJsonObject>> perImageMetadata_;
  QList<QString> availableStreams_;
  QList<QString> availableGeometryTypes_;

  QMutex mutex;
  QMap<QString, QJsonObject> geometryDataChache;  // protected by mutex

  TomographyRawData2DAccessorDBus(
      const QSharedPointer<BusConnection>& connection,
      const std::tuple<QString, QDBusObjectPath>& provider);
  virtual ~TomographyRawData2DAccessorDBus();

  // Allow the ctor to be called using create()
  friend class vx::RefCountedObject;

 public:
  uint64_t numberOfImages(const QString& stream) override;
  QJsonObject metadata() const override { return metadata_; }
  QList<QJsonObject> availableImageKinds() const override {
    return availableImageKinds_;
  }
  QList<QString> availableStreams() override { return availableStreams_; }
  virtual bool hasStream(const QString& stream) override {
    return numberOfImages_.contains(stream);
  }
  QList<QString> availableGeometryTypes() override {
    return availableGeometryTypes_;
  }
  const QMap<std::tuple<QString, uint64_t>, vx::VectorSizeT2>& imageSizes()
      const {
    return imageSizes_;
  }
  const QMap<QString, QMap<uint64_t, QJsonObject>>& perImageMetadata() const {
    return perImageMetadata_;
  }
  vx::VectorSizeT2 imageSize(const QString& stream, uint64_t id) override;
  QJsonObject getPerImageMetadata(const QString& stream, uint64_t id) override;
  QJsonObject getGeometryData(const QString& geometryType) override;

  QString readImages(const QJsonObject& imageKind,
                     const QList<std::tuple<QString, quint64>>& images,
                     const std::tuple<quint64, quint64>& inputRegionStart,
                     const QSharedPointer<TomographyRawData2DRegular>& output,
                     qulonglong firstOutputImageId,
                     const std::tuple<quint64, quint64>& outputRegionStart,
                     const std::tuple<quint64, quint64>& regionSize,
                     bool allowIncompleteData = false) override;

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;
};

}  // namespace vx

Q_DECLARE_METATYPE(vx::TomographyRawData2DAccessor*)
Q_DECLARE_METATYPE(vx::TomographyRawData2DAccessorDBus*)
