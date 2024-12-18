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

// QDBusConnection should be included as early as possible:
// https://bugreports.qt.io/browse/QTBUG-48351 /
// https://bugreports.qt.io/browse/QTBUG-48377
#include <QtDBus/QDBusConnection>

#include "Instance.hpp"

#include <Main/AllDebugOptions.hpp>
#include <Main/DebugOptionDBus.hpp>
#include <Main/IO/Load.hpp>
#include <Main/Root.hpp>
#include <Main/Utilities.hpp>
#include <Main/Version.hpp>

#include <Main/IO/RunAllFilterOperation.hpp>

#include <VoxieBackend/Data/BlockJpegImplementation.hpp>
#include <VoxieBackend/Data/Buffer.hpp>
#include <VoxieBackend/Data/BufferType.hpp>
#include <VoxieBackend/Data/DataProperty.hpp>
#include <VoxieBackend/Data/FileData.hpp>
#include <VoxieBackend/Data/GeometricPrimitiveData.hpp>
#include <VoxieBackend/Data/ImageDataPixel.hpp>
#include <VoxieBackend/Data/SeriesData.hpp>
#include <VoxieBackend/Data/SeriesDimension.hpp>
#include <VoxieBackend/Data/TomographyRawData2DRegular.hpp>
#include <VoxieBackend/Data/VolumeSeriesData.hpp>

#include <VoxieBackend/IO/OperationImport.hpp>
#include <VoxieBackend/IO/OperationResult.hpp>

#include <Voxie/Data/ContainerData.hpp>
#include <Voxie/Data/TableData.hpp>

#include <Voxie/Node/DataNode.hpp>
#include <Voxie/Node/Types.hpp>

#include <Voxie/Component/Plugin.hpp>
#include <VoxieBackend/Component/ComponentContainer.hpp>

#include <VoxieClient/ObjectExport/BusManager.hpp>
#include <VoxieClient/ObjectExport/Client.hpp>
#include <VoxieClient/ObjectExport/DBusCallUtil.hpp>

#include <VoxieBackend/Data/EventListDataAccessor.hpp>
#include <VoxieBackend/Data/EventListDataBuffer.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>
#include <VoxieBackend/Data/TomographyRawData2DAccessor.hpp>
#include <VoxieBackend/Data/VolumeDataBlockJpeg.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>

#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/JsonDBus.hpp>
#include <VoxieClient/JsonUtil.hpp>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusArgument>

#if defined(INSTANCE_CPP_ADDITIONAL_INCLUDE)
#include INSTANCE_CPP_ADDITIONAL_INCLUDE
#endif

using namespace vx;
using namespace vx::io;
using namespace vx::gui;
using namespace vx;
using namespace vx::plugin;
using namespace vx;

Instance::Instance(Root* root)
    : ExportedObject("", nullptr, true), root_(root) {
  new InstanceAdaptorImpl(this);

  this->utilities_ = makeSharedQObject<Utilities>(root);

#if defined(INSTANCE_ADDITIONAL)
  INSTANCE_ADDITIONAL
#endif
}
Instance::~Instance() {}

InstanceAdaptorImpl::InstanceAdaptorImpl(Instance* object)
    : InstanceAdaptor(object), object(object), root(object->root()) {}
InstanceAdaptorImpl::~InstanceAdaptorImpl() {}

QDBusObjectPath InstanceAdaptorImpl::gui() const {
  if (root->mainWindow())
    return ExportedObject::getPath(root->mainWindow()->getGuiDBusObject());
  else
    return QDBusObjectPath("/");
}

QDBusObjectPath InstanceAdaptorImpl::utilities() const {
  return ExportedObject::getPath(object->utilities());
}

QDBusObjectPath InstanceAdaptorImpl::components() const {
  return ExportedObject::getPath(root->components());
}

QList<QDBusObjectPath> InstanceAdaptorImpl::ListPrototypes(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    QList<QDBusObjectPath> paths;

    QList<QSharedPointer<NodePrototype>> factories = root->factories();

    for (auto fac : factories) {
      paths.append(vx::ExportedObject::getPath(fac.data()));
    }

    return paths;
  } catch (Exception& e) {
    e.handle(object);
    return QList<QDBusObjectPath>();
  }
}

// TODO: Remove?
QDBusObjectPath InstanceAdaptorImpl::GetComponent(
    const QString& componentType, const QString& name,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options, "AllowCompatibilityNames",
                                 "AllowComponentTypeCompatibilityNames");

    auto componentType_ = root->components()->lookupType(
        componentType,
        ExportedObject::getOptionValueOrDefault<bool>(
            options, "AllowComponentTypeCompatibilityNames", true));

    return ExportedObject::getPath(root->components()->getComponent(
        componentType_, name,
        ExportedObject::getOptionValueOrDefault<bool>(
            options, "AllowCompatibilityNames", true)));
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QList<QDBusObjectPath> InstanceAdaptorImpl::ListPlugins(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    QList<QDBusObjectPath> paths;

    for (const auto& plugin : root->plugins()) {
      paths.append(ExportedObject::getPath(plugin));
    }

    return paths;
  } catch (Exception& e) {
    return QList<QDBusObjectPath>();
  }
}

QDBusObjectPath InstanceAdaptorImpl::GetPluginByName(
    const QString& name, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    return ExportedObject::getPath(root->getPluginByName(name));
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QList<QDBusObjectPath> InstanceAdaptorImpl::ListNodes(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    QList<QDBusObjectPath> paths;
    for (const auto& node : root->nodes()) {
      paths.append(ExportedObject::getPath(node));
    }
    return paths;
  } catch (Exception& e) {
    e.handle(object);
    return QList<QDBusObjectPath>();
  }
}
QList<QDBusObjectPath> InstanceAdaptorImpl::ListObjects(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    QList<QDBusObjectPath> paths;
    for (const auto& node : root->nodes()) {
      paths.append(ExportedObject::getPath(node));
    }
    return paths;
  } catch (Exception& e) {
    e.handle(object);
    return QList<QDBusObjectPath>();
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateBuffer(
    const QDBusObjectPath& client, qint64 offsetBytes, const QDBusVariant& type,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    if (offsetBytes != 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.NotImplemented",
                          "CreateBuffer: Non-zero offsets not implemented");
    auto typeVal = vx::BufferTypeBase::fromJson(dbusToJson(type));
    auto typeArray = qSharedPointerDynamicCast<vx::BufferTypeArray>(typeVal);
    if (!typeArray)
      throw vx::Exception("de.uni_stuttgart.Voxie.NotImplemented",
                          "CreateBuffer: Non-array types not implemented");
    if (typeArray->dim() != 1)
      throw vx::Exception("de.uni_stuttgart.Voxie.NotImplemented",
                          "CreateBuffer: Arrays with dim != 1 not implemented");
    auto count = typeArray->shape()[0];
    auto strideBytes = typeArray->stridesBytes()[0];
    auto innerType = typeArray->elementType();

    // TODO: Should this check whether there is a matching buffer type? Or
    // should it accept any valid type? Or any type?
    QSharedPointer<BufferType> bufferType;
    for (const auto& bt :
         vx::voxieRoot().components()->listComponentsTyped<BufferType>()) {
      // qDebug() << "Compare" << bt->type()->toJson() << "and"
      //          << innerType->toJson();
      if (bt->type()->equals(innerType.data())) {
        bufferType = bt;
        break;
      }
    }
    if (!bufferType)
      throw vx::Exception("de.uni_stuttgart.Voxie.NotImplemented",
                          "CreateBuffer: Unknown data type");

    // TODO: Should this be checked here? This will not work if there are
    // multiple BufferTypes with the same members and offsets but with a
    // different overall size (because of padding).
    if (strideBytes < 0 || (quint64)strideBytes != bufferType->sizeBytes())
      throw vx::Exception("de.uni_stuttgart.Voxie.NotImplemented",
                          "CreateBuffer: Invalid array stride");

    auto buffer = Buffer::create(innerType, count, strideBytes);
    clientPtr->incRefCount(buffer);
    return ExportedObject::getPath(buffer.data());
  } catch (Exception& e) {
    return e.handle(object);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateImage(
    const QDBusObjectPath& client, const vx::TupleVector<quint64, 2>& size,
    quint64 componentCount,
    const std::tuple<QString, quint32, QString>& dataType,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto type = parseDataTypeStruct(dataType);

    auto image = ImageDataPixel::createInst(
        std::get<0>(size), std::get<1>(size), componentCount, type,
        true /*TODO: should this be writable from DBus?*/);
    clientPtr->incRefCount(image);
    return ExportedObject::getPath(image.data());
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateSurfaceDataTriangleIndexed(
    const QDBusObjectPath& client, qulonglong triangleCount,
    qulonglong vertexCount, const QDBusObjectPath& triangleSource,
    bool triangleWritable, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options, "Attributes");

    auto attributes = ExportedObject::getOptionValueOrDefault<QList<std::tuple<
        QString, QString, quint64, std::tuple<QString, quint32, QString>,
        QString, QMap<QString, QDBusVariant>, QMap<QString, QDBusVariant>>>>(
        options, "Attributes", {});

    auto clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr)
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");

    auto triangleSourcePtr =
        SurfaceDataTriangleIndexed::lookupOptional(triangleSource);

    auto surface = SurfaceDataTriangleIndexed::create(
        triangleCount, vertexCount, triangleSourcePtr, triangleWritable,
        attributes);

    clientPtr->incRefCount(surface);
    return ExportedObject::getPath(surface);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateVolumeDataVoxel(
    const QDBusObjectPath& client, const vx::TupleVector<quint64, 3>& size,
    const std::tuple<QString, quint32, QString>& dataType,
    const vx::TupleVector<double, 3>& gridOrigin,
    const vx::TupleVector<double, 3>& gridSpacing,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);
    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto datatype = parseDataTypeStruct(dataType);

    if (std::get<0>(size) > std::numeric_limits<size_t>::max() ||
        std::get<1>(size) > std::numeric_limits<size_t>::max() ||
        std::get<2>(size) > std::numeric_limits<size_t>::max())
      throw Exception("de.uni_stuttgart.Voxie.Overflow",
                      "Volume dimensions too large");
    auto data = VolumeDataVoxel::createVolume(
        vectorCastNarrow<size_t>(toVector(size)), datatype,
        toVector(gridOrigin), toVector(gridSpacing));
    clientPtr->incRefCount(data);
    return ExportedObject::getPath(data.data());
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateVolumeDataBlockJpeg(
    const QDBusObjectPath& client,
    const std::tuple<quint64, quint64, quint64>& arrayShape,
    const std::tuple<quint64, quint64, quint64>& blockShape,
    const std::tuple<double, double, double>& volumeOrigin,
    const std::tuple<double, double, double>& gridSpacing, double valueOffset,
    double valueScalingFactor, uint samplePrecision,
    const QList<QByteArray>& huffmanTableDC,
    const QList<QByteArray>& huffmanTableAC,
    const QList<QList<quint16>>& quantizationTable,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);
    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    if (std::get<0>(arrayShape) > std::numeric_limits<size_t>::max() ||
        std::get<1>(arrayShape) > std::numeric_limits<size_t>::max() ||
        std::get<2>(arrayShape) > std::numeric_limits<size_t>::max())
      throw Exception("de.uni_stuttgart.Voxie.Overflow",
                      "Volume dimensions too large");
    auto arrayShapeC = vectorCastNarrow<size_t>(toVector(arrayShape));

    if (std::get<0>(blockShape) > std::numeric_limits<size_t>::max() ||
        std::get<1>(blockShape) > std::numeric_limits<size_t>::max() ||
        std::get<2>(blockShape) > std::numeric_limits<size_t>::max())
      throw Exception("de.uni_stuttgart.Voxie.Overflow",
                      "Volume dimensions too large");
    auto blockShapeC = vectorCastNarrow<size_t>(toVector(blockShape));

    auto possibleImplementations =
        voxieRoot()
            .components()
            ->listComponentsTyped<BlockJpegImplementation>();

    auto data = VolumeDataBlockJpeg::create(
        arrayShapeC, blockShapeC, toVector(volumeOrigin), toVector(gridSpacing),
        valueOffset, valueScalingFactor, samplePrecision,
        fromByteArray(huffmanTableDC), fromByteArray(huffmanTableAC),
        quantizationTable, possibleImplementations);

    clientPtr->incRefCount(data);
    return ExportedObject::getPath(data.data());
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateTomographyRawData2DRegular(
    const QDBusObjectPath& client,
    const std::tuple<quint64, quint64>& imageShape, qulonglong imageCount,
    const std::tuple<QString, quint32, QString>& dataType,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);
    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto datatype = parseDataTypeStruct(dataType);

    auto data = TomographyRawData2DRegular::createInst(
        std::get<0>(imageShape), std::get<1>(imageShape), imageCount, datatype);

    clientPtr->incRefCount(data);
    return ExportedObject::getPath(data.data());
  } catch (Exception& e) {
    e.handle(object);
    return vx::dbusDefaultReturnValue();
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateTomographyRawData2DAccessor(
    const QDBusObjectPath& client,
    const std::tuple<QString, QDBusObjectPath>& provider,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    auto connection = getBusManager()->findConnection(object->connection());

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto data = TomographyRawData2DAccessorDBus::create(connection, provider);

    clientPtr->incRefCount(data);
    return ExportedObject::getPath(data);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateEventListDataBuffer(
    const QDBusObjectPath& client, qulonglong capacity,
    const QList<std::tuple<QString, std::tuple<QString, quint32, QString>,
                           QString, QMap<QString, QDBusVariant>,
                           QMap<QString, QDBusVariant>>>& attributes,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto eventList = vx::EventListDataBuffer::create(capacity, attributes);
    clientPtr->incRefCount(eventList);
    return ExportedObject::getPath(eventList.data());
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateEventListDataAccessor(
    const QDBusObjectPath& client,
    const std::tuple<QString, QDBusObjectPath>& backend,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    auto connection = getBusManager()->findConnection(object->connection());

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto data = EventListDataAccessorDBus::create(connection, backend);

    clientPtr->incRefCount(data);
    return ExportedObject::getPath(data);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateGeometricPrimitiveData(
    const QDBusObjectPath& client, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);
    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto data = GeometricPrimitiveData::create();

    clientPtr->incRefCount(data);
    return ExportedObject::getPath(data);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateFileDataByteStream(
    const QDBusObjectPath& client, const QString& mediaType,
    quint64 lengthBytes, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);
    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    if (lengthBytes > std::numeric_limits<std::size_t>::max())
      throw vx::Exception("de.uni_stuttgart.Voxie.OutOfMemory",
                          "Overflow for file size");
    // TODO: Do allocation on background thread?
    auto data = FileDataByteStream::create(mediaType, lengthBytes);

    clientPtr->incRefCount(data);
    return ExportedObject::getPath(data);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateTableData(
    const QDBusObjectPath& client,
    const QList<std::tuple<QString, QDBusObjectPath, QString,
                           QMap<QString, QDBusVariant>,
                           QMap<QString, QDBusVariant>>>& columns,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);
    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    QList<TableColumn> columnsList;
    for (const auto& column : columns) {
      QString name = std::get<0>(column);

      auto typePath = std::get<1>(column);
      // TODO: This should be able to look up the QSharedPointer directly
      auto typeWeak = qobject_cast<PropertyType*>(
          ExportedObject::lookupWeakObject(typePath));
      if (!typeWeak)
        throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                        "Cannot find type object '" + typePath.path() + "'");
      auto type = vx::voxieRoot().components()->getComponentTyped<PropertyType>(
          typeWeak->name(), false);
      if (!TableColumn::isAllowedType(type))
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                            "Type '" + type->name() +
                                "' is not a valid type for table columns");

      QString displayName = std::get<2>(column);

      QMap<QString, QDBusVariant> metadata = std::get<3>(column);

      ExportedObject::checkOptions(std::get<4>(column));

      columnsList << TableColumn(name, type, displayName, metadata);
    }
    auto data = TableData::create(columnsList);

    clientPtr->incRefCount(data);
    return ExportedObject::getPath(data);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateContainerData(
    const QDBusObjectPath& client, const QString& name,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);
    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto containerData = ContainerData::create(name);

    clientPtr->incRefCount(containerData);
    return ExportedObject::getPath(containerData);
  } catch (Exception& e) {
    e.handle(object);
    return ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateVolumeSeriesData(
    const QDBusObjectPath& client, const QList<QDBusObjectPath>& dimensions,
    const vx::TupleVector<double, 3>& volumeOrigin,
    const vx::TupleVector<double, 3>& volumeSize,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    QList<QSharedPointer<SeriesDimension>> dimensionsObj;
    for (const auto& dimension : dimensions)
      dimensionsObj << SeriesDimension::lookup(dimension);

    auto seriesData = VolumeSeriesData::create(
        dimensionsObj, toVector(volumeOrigin), toVector(volumeSize));

    clientPtr->incRefCount(seriesData);
    return ExportedObject::getPath(seriesData);
  } catch (Exception& e) {
    return e.handle(object);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateDataProperty(
    const QDBusObjectPath& client, const QString& name,
    const QDBusVariant& json, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    // TODO: Allow passing in the type object directly?

    // TODO: Check whether this type is allowed here?

    auto dataProperty = DataProperty::create(
        name, expectObject(dbusToJson(json)), vx::voxieRoot().components());

    clientPtr->incRefCount(dataProperty);
    return ExportedObject::getPath(dataProperty);
  } catch (Exception& e) {
    return e.handle(object);
  }
}

QDBusObjectPath InstanceAdaptorImpl::CreateSeriesDimension(
    const QDBusObjectPath& client, const QDBusObjectPath& property,
    const QDBusVariant& entries, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto propertyObj = DataProperty::lookup(property);

    QList<QVariant> entriesRaw = propertyObj->type()->dbusToRawList(entries);

    auto seriesDimension = SeriesDimension::create(propertyObj, entriesRaw);

    clientPtr->incRefCount(seriesDimension);
    return ExportedObject::getPath(seriesDimension);
  } catch (Exception& e) {
    return e.handle(object);
  }
}

void InstanceAdaptorImpl::Quit(const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options, "AskForConfirmation");

    bool askForConfirmation = ExportedObject::getOptionValueOrDefault<bool>(
        options, "AskForConfirmation", true);

    root->quit(askForConfirmation);
  } catch (Exception& e) {
    e.handle(object);
  }
}

QDBusObjectPath InstanceAdaptorImpl::OpenFile(
    const QString& fileName, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    // auto operationResult = object->import(fileName);
    auto operationResult = vx::io::Load::openFile(root, fileName, true);

    return operationResult->dbusDelayedReturn(
        object,
        [](const QSharedPointer<vx::io::Operation::ResultSuccess>& result) {
          auto resultImport =
              qSharedPointerDynamicCast<OperationImport::Result>(result);
          if (!resultImport)
            throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                                "Unable to cast to OperationImport::Result");
          auto obj = resultImport->dataNode();
          if (!obj)
            throw vx::Exception(
                "de.uni_stuttgart.Voxie.Error",
                "Load operation got unexpected object / dataNode not set");

          // clientPtr->incRefCount(obj);
          return ExportedObject::getPath(obj);
        });
  } catch (Exception& e) {
    e.handle(object);
    return vx::ExportedObject::getPath(nullptr);
  }
}

QDBusObjectPath InstanceAdaptorImpl::Import(
    const QDBusObjectPath& client, const QString& fileName,
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    // auto operationResult = object->import(fileName);
    auto operationResult = vx::io::Load::openFile(root, fileName, false);

    return operationResult->dbusDelayedReturn(
        object,
        [client](
            const QSharedPointer<vx::io::Operation::ResultSuccess>& result) {
          // TODO: Do this earlier? But then Client might be destroyed
          Client* clientPtr =
              qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
          if (!clientPtr) {
            throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                            "Cannot find client object");
          }

          auto resultImport =
              qSharedPointerDynamicCast<OperationImport::Result>(result);
          if (!resultImport)
            throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                                "Unable to cast to OperationImport::Result");
          auto data = resultImport->data();

          clientPtr->incRefCount(data);
          return ExportedObject::getPath(data);
        });
  } catch (Exception& e) {
    e.handle(object);
    return vx::ExportedObject::getPath(nullptr);
  }
}

QMap<QString, QDBusVariant> InstanceAdaptorImpl::versionInformation() const {
  QMap<QString, QDBusVariant> info;
  info["VersionNoSuffix"] = dbusMakeVariant<QString>(vx::getVersionNoSuffix());
  info["VersionSuffix"] = dbusMakeVariant<QString>(vx::getVersionSuffix());
  info["Version"] = dbusMakeVariant<QString>(vx::getVersion());
  info["VersionString"] = dbusMakeVariant<QString>(vx::getVersionString());
  info["GitRevision"] = dbusMakeVariant<QString>(vx::getGitRevision());
  info["GitRevisionDescribe"] =
      dbusMakeVariant<QString>(vx::getGitRevisionDescribe());
  info["QtVersion"] = dbusMakeVariant<QString>(vx::getQtVersion());
  return info;
}

QDBusObjectPath InstanceAdaptorImpl::RunAllFilters(
    const QDBusObjectPath& client, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    Client* clientPtr =
        qobject_cast<Client*>(ExportedObject::lookupWeakObject(client));
    if (!clientPtr) {
      throw Exception("de.uni_stuttgart.Voxie.ObjectNotFound",
                      "Cannot find client object");
    }

    auto op = RunAllFilterOperation::create();
    auto result = OperationResult::create(op);

    op->runAll();

    clientPtr->incRefCount(result);
    return ExportedObject::getPath(result);
  } catch (Exception& e) {
    return e.handle(object);
  }
}

static QList<DebugOptionDBus*> getDebugOptionList() {
  QList<DebugOptionDBus*> result;

  auto options = allDebugOptions();
  for (const auto& entry : *options) {
    result << new DebugOptionDBus(entry);
  }

  return result;
}
static QSharedPointer<QList<DebugOptionDBus*>> debugOptionList() {
  static QSharedPointer<QList<DebugOptionDBus*>> cache =
      createQSharedPointer<QList<DebugOptionDBus*>>(getDebugOptionList());
  return cache;
}

QDBusObjectPath InstanceAdaptorImpl::GetDebugOption(
    const QString& name, const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    // TODO: create QMap?
    auto list = debugOptionList();
    for (const auto& entry : *list) {
      if (entry->option()->name() == name)
        return vx::ExportedObject::getPath(entry);
    }

    throw Exception("de.uni_stuttgart.Voxie.DebugOptionNotFound",
                    "Cannot find debug option " + name);
  } catch (Exception& e) {
    return e.handle(object);
  }
}
QList<QDBusObjectPath> InstanceAdaptorImpl::ListDebugOptions(
    const QMap<QString, QDBusVariant>& options) {
  try {
    ExportedObject::checkOptions(options);

    QList<QDBusObjectPath> paths;

    auto list = debugOptionList();
    for (const auto& entry : *list) {
      paths.append(vx::ExportedObject::getPath(entry));
    }

    return paths;

  } catch (Exception& e) {
    return e.handle(object);
  }
}
