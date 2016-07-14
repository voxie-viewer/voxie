#include "scriptwrapper.hpp"

#include <Voxie/scripting/dbustypes.hpp>
#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMetaMethod>

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusMetaType>

ScriptWrapper::ScriptWrapper(QScriptEngine* engine) : engine (engine) {
}
ScriptWrapper::~ScriptWrapper() {
}

struct AdditionalQScriptData {
    QSharedPointer<std::function<QScriptValue(QScriptContext*, QScriptEngine*)>> ptr;
    AdditionalQScriptData(){
        //qDebug() << "AdditionalQScriptData ctor" << this;
    }
    AdditionalQScriptData(const AdditionalQScriptData& other) : ptr(other.ptr) {
        //qDebug() << "AdditionalQScriptData cctor" << this << &other;
    }
    ~AdditionalQScriptData() {
        //qDebug() << "AdditionalQScriptData dtor" << this;
    }
    AdditionalQScriptData(const QSharedPointer<std::function<QScriptValue(QScriptContext*, QScriptEngine*)>>& ptr) : ptr (ptr) {
        //qDebug() << "AdditionalQScriptData ctor2" << this;
    }
};
Q_DECLARE_METATYPE(AdditionalQScriptData);
QDBusArgument &operator<<(QDBusArgument &argument, const AdditionalQScriptData& value) {
  argument.beginStructure();
  if (!value.ptr)
      argument << QString("(null)");
  else
      argument << QString(value.ptr->target_type().name());
  argument.endStructure();
  return argument;
}
const QDBusArgument &operator>>(const QDBusArgument &argument, AdditionalQScriptData& value) {
  argument.beginStructure();
  argument.endStructure();
  value = AdditionalQScriptData();
  return argument;
}

QScriptValue callScriptFunction(QScriptContext* context, QScriptEngine* engine, void* data) {
    auto data2 = (std::function<QScriptValue(QScriptContext*, QScriptEngine*)>*) data;
    return (*data2) (context, engine);
}

void ScriptWrapper::addScriptFunction (QScriptValue obj, const QString& name, const QScriptValue::PropertyFlags& flags, const std::function<QScriptValue(QScriptContext*, QScriptEngine*)>& fun) {
    qDBusRegisterMetaType<AdditionalQScriptData> ();

    QScriptEngine* engine = obj.engine();

    QSharedPointer<std::function<QScriptValue(QScriptContext*, QScriptEngine*)>> ptr(new std::function<QScriptValue(QScriptContext*, QScriptEngine*)>(fun));

    QScriptValue func = engine->newFunction(callScriptFunction, ptr.data());

    //QScriptValue addBaseObj = obj;
    QScriptValue addBaseObj = func;

    QScriptValue addobj = addBaseObj.data().property("additional");
    if (!addobj.isValid()) {
        if (!addobj.data().isValid()) {
            addBaseObj.setData(engine->newObject());
        }
        addobj = engine->newObject();
        addobj.setProperty("count", 0);
        addBaseObj.data().setProperty("additional", addobj);
    }
    quint32 count = addobj.property("count").toUInt32();
    QString pname = QString("data%1").arg(count);
    if (addobj.property(pname).isValid()) {
        qCritical() << "Warning: Additional data already set";
        return;
    }
    addobj.setProperty(pname, engine->newVariant(QVariant::fromValue(AdditionalQScriptData(ptr))));
    count++;
    addobj.setProperty("count", count);

    if (!addBaseObj.data().property("additional").property(pname).isValid()) {
        qCritical() << "Warning: Setting additional data failed";
        return;
    }

    obj.setProperty(name, func, flags);
}

QVariant ScriptWrapper::fromScriptValue(const QScriptValue& value) {
    QScriptValue dbusPath = value.data().property("dbus_path");
    if (dbusPath.isValid())
        return QVariant::fromValue(QDBusObjectPath(dbusPath.toString()));
    return value.toVariant();
}

static bool fromScriptValue(QVariant& variant, QScriptContext* context, int type, const QScriptValue& value, const QString& info) {
    variant = ScriptWrapper::fromScriptValue(value);
    //qDebug() << type << QMetaType::typeName(type) << variant.userType() << QMetaType::typeName(variant.userType());

    if (type == qMetaTypeId<QVector2D>()) {
        if (variant.type() != QVariant::List) {
            context->throwError(QString("Error while converting %1: Expected a list, got a %2").arg(info).arg(QMetaType::typeName(variant.userType())));
            return false;
        }
        QList<QVariant> list = variant.toList();
        if (list.size() != 2) {
            context->throwError(QString("Error while converting %1: Expected a list with 2 entries, got %2").arg(info).arg(list.size()));
            return false;
        }
        if (!list[0].convert(qMetaTypeId<double>())
            || !list[1].convert(qMetaTypeId<double>())) {
            context->throwError(QString("Error while converting %1: Could not convert values to double").arg(info));
            return false;
        }
        variant = QVector2D(list[0].value<double>(), list[1].value<double>());
        return true;
    }
    if (type == qMetaTypeId<QVector3D>()) {
        if (variant.type() != QVariant::List) {
            context->throwError(QString("Error while converting %1: Expected a list, got a %2").arg(info).arg(QMetaType::typeName(variant.userType())));
            return false;
        }
        QList<QVariant> list = variant.toList();
        if (list.size() != 3) {
            context->throwError(QString("Error while converting %1: Expected a list with 3 entries, got %2").arg(info).arg(list.size()));
            return false;
        }
        if (!list[0].convert(qMetaTypeId<double>())
            || !list[1].convert(qMetaTypeId<double>())
            || !list[2].convert(qMetaTypeId<double>())) {
            context->throwError(QString("Error while converting %1: Could not convert values to double").arg(info));
            return false;
        }
        variant = QVector3D(list[0].value<double>(), list[1].value<double>(), list[2].value<double>());
        return true;
    }
    if (type == qMetaTypeId<QQuaternion>()) {
        if (variant.type() != QVariant::List) {
            context->throwError(QString("Error while converting %1: Expected a list, got a %2").arg(info).arg(QMetaType::typeName(variant.userType())));
            return false;
        }
        QList<QVariant> list = variant.toList();
        if (list.size() != 4) {
            context->throwError(QString("Error while converting %1: Expected a list with 4 entries, got %2").arg(info).arg(list.size()));
            return false;
        }
        if (!list[0].convert(qMetaTypeId<double>())
            || !list[1].convert(qMetaTypeId<double>())
            || !list[2].convert(qMetaTypeId<double>())
            || !list[3].convert(qMetaTypeId<double>())) {
            context->throwError(QString("Error while converting %1: Could not convert values to double").arg(info));
            return false;
        }
        variant = QQuaternion(list[0].value<double>(), list[1].value<double>(), list[2].value<double>(), list[3].value<double>());
        return true;
    }

    if (type == qMetaTypeId<voxie::scripting::IntVector2>()) {
        if (variant.type() != QVariant::List) {
            context->throwError(QString("Error while converting %1: Expected a list, got a %2").arg(info).arg(QMetaType::typeName(variant.userType())));
            return false;
        }
        QList<QVariant> list = variant.toList();
        if (list.size() != 2) {
            context->throwError(QString("Error while converting %1: Expected a list with 2 entries, got %2").arg(info).arg(list.size()));
            return false;
        }
        if (!list[0].convert(qMetaTypeId<quint64>())
            || !list[1].convert(qMetaTypeId<quint64>())) {
            context->throwError(QString("Error while converting %1: Could not convert values to quint64").arg(info));
            return false;
        }
        variant = QVariant::fromValue(voxie::scripting::IntVector2(list[0].value<quint64>(), list[1].value<quint64>()));
        return true;
    }
    if (type == qMetaTypeId<voxie::scripting::IntVector3>()) {
        if (variant.type() != QVariant::List) {
            context->throwError(QString("Error while converting %1: Expected a list, got a %2").arg(info).arg(QMetaType::typeName(variant.userType())));
            return false;
        }
        QList<QVariant> list = variant.toList();
        if (list.size() != 3) {
            context->throwError(QString("Error while converting %1: Expected a list with 3 entries, got %2").arg(info).arg(list.size()));
            return false;
        }
        if (!list[0].convert(qMetaTypeId<quint64>())
            || !list[1].convert(qMetaTypeId<quint64>())
            || !list[2].convert(qMetaTypeId<quint64>())) {
            context->throwError(QString("Error while converting %1: Could not convert values to quint64").arg(info));
            return false;
        }
        variant = QVariant::fromValue(voxie::scripting::IntVector3(list[0].value<quint64>(), list[1].value<quint64>(), list[2].value<quint64>()));
        return true;
    }

    if (type == qMetaTypeId<voxie::scripting::Plane>()) {
        QVariant origin;
        QVariant rotation;
        if (!fromScriptValue(origin, context, qMetaTypeId<QVector3D>(), value.property("Origin"), "property origin of " + info)
            || !fromScriptValue(rotation, context, qMetaTypeId<QQuaternion>(), value.property("Rotation"), "property rotation of " + info))
            return false;
        voxie::scripting::Plane plane;
        plane.origin = origin.value<QVector3D>();
        plane.rotation = rotation.value<QQuaternion>();
        variant = QVariant::fromValue(plane);
    }

    if (type == qMetaTypeId<QVector<QDBusObjectPath>>()) {
        if (variant.type() != QVariant::List) {
            context->throwError(QString("Error while converting %1: Expected a list, got a %2").arg(info).arg(QMetaType::typeName(variant.userType())));
            return false;
        }
        int size = variant.toList().size();
        QVector<QDBusObjectPath> result;
        for (int i = 0; i < size; i++) {
            QVariant element;
            if (!fromScriptValue(element, context, qMetaTypeId<QDBusObjectPath>(), value.property(i), QString("element %1 of %2").arg(i).arg(info)))
                return false;
            result << element.value<QDBusObjectPath>();
        }
        variant = QVariant::fromValue(result);
    }

    int oldType = variant.userType();
    if (!variant.convert(type)) {
        context->throwError(QString("Error while converting %1 from type %2 to type %3").arg(info).arg(QMetaType::typeName(oldType)).arg(QMetaType::typeName(type)));
        return false;
    }
    return true;
}

QScriptValue ScriptWrapper::toScriptValue(const QVariant& value) {
    QString typeName = QMetaType::typeName(value.userType());

    /*
    if (typeName.startsWith("QList<") && typeName.endsWith(">")) {
        QString innerType = typeName.mid (6, typeName.length() - 7);
        int innerTypeId = qRegisterMetaType(innerType.toUtf8().data());
        return engine->toScriptValue(innerTypeId);
    }
    */

    //qDebug() << typeName;
    if (value.userType() == qMetaTypeId<QList<QDBusObjectPath>>()) {
        const auto& list = value.value<QList<QDBusObjectPath>>();
        QScriptValue array = engine->newArray(list.length());
        for (int i = 0; i < list.length(); i++)
            array.setProperty(i, toScriptValue(QVariant::fromValue(list[i])));
        return array;
    }
    //qDebug() << value.userType() << qMetaTypeId<QVector<QDBusObjectPath>>();
    //qDebug() << QMetaType::typeName(qMetaTypeId<QVector<QDBusObjectPath>>());
    if (value.userType() == qMetaTypeId<QVector<QDBusObjectPath>>()) {
        const auto& list = value.value<QVector<QDBusObjectPath>>();
        QScriptValue array = engine->newArray(list.length());
        for (int i = 0; i < list.length(); i++)
            array.setProperty(i, toScriptValue(QVariant::fromValue(list[i])));
        return array;
    }

    if (value.userType() == qMetaTypeId<QDBusObjectPath>()) {
        const auto& val = value.value<QDBusObjectPath>();
        if (val.path() == "/")
            return engine->nullValue();
        voxie::scripting::ScriptableObject* obj = voxie::scripting::ScriptableObject::lookupWeakObject(val);
        if (!obj) {
            // Should not happen because this path was just returned by some
            // function and there wasn't any opportunity yet to destroy the
            // object
            qCritical() << "Could not find returned DBus object";
            return engine->nullValue();
        }
        return getWrapper(obj);
    }

    if (value.userType() == qMetaTypeId<voxie::scripting::IntVector2>()) {
        const auto& val = value.value<voxie::scripting::IntVector2>();
        auto ret = engine->newArray(2);
        ret.setProperty(0, (double) val.x);
        ret.setProperty(1, (double) val.y);
        return ret;
    }
    if (value.userType() == qMetaTypeId<voxie::scripting::IntVector3>()) {
        const auto& val = value.value<voxie::scripting::IntVector3>();
        auto ret = engine->newArray(3);
        ret.setProperty(0, (double) val.x);
        ret.setProperty(1, (double) val.y);
        ret.setProperty(2, (double) val.z);
        return ret;
    }

    if (value.userType() == qMetaTypeId<QVector2D>()) {
        const auto& val = value.value<QVector2D>();
        auto ret = engine->newArray(2);
        ret.setProperty(0, val.x());
        ret.setProperty(1, val.y());
        return ret;
    }
    if (value.userType() == qMetaTypeId<QVector3D>()) {
        const auto& val = value.value<QVector3D>();
        auto ret = engine->newArray(3);
        ret.setProperty(0, val.x());
        ret.setProperty(1, val.y());
        ret.setProperty(2, val.z());
        return ret;
    }
    if (value.userType() == qMetaTypeId<QQuaternion>()) {
        const auto& val = value.value<QQuaternion>();
        auto ret = engine->newArray(4);
        ret.setProperty(0, val.scalar());
        ret.setProperty(1, val.x());
        ret.setProperty(2, val.y());
        ret.setProperty(3, val.z());
        return ret;
    }

    if (value.userType() == qMetaTypeId<QMap_QDBusObjectPath_quint64>()) {
        const auto& val = value.value<QMap_QDBusObjectPath_quint64>();
        auto ret = engine->newObject();
        for (const auto& key : val.keys())
            ret.setProperty(key.path(), (double) val[key]);
        return ret;
    }

    if (value.userType() == qMetaTypeId<voxie::scripting::Plane>()) {
        const auto& val = value.value<voxie::scripting::Plane>();
        auto ret = engine->newObject();
        ret.setProperty("Origin", toScriptValue(val.origin));
        ret.setProperty("Rotation", toScriptValue(val.rotation));
        return ret;
    }
    // No support for Array2Info / Array3Info

    if (value.type() == QVariant::UserType)
        qWarning() << "Warning: Unknown type for QScript:" << typeName;
    return engine->toScriptValue(value);
}

static QScriptValue errorToString(QScriptContext* context, QScriptEngine* engine) {
    Q_UNUSED(engine);
    return context->thisObject().property("name").toString() + ": " + context->thisObject().property("message").toString();
}

void ScriptWrapper::addPropertyWrapper(const QScriptValue& obj, const QMetaObject* type, const QMetaProperty& property, QString& propertyDesc, const QString& objectChildId, const QMetaObject* parentType) {
    QString desc;
    desc += QString("%1 : %2").arg(QString(property.name())).arg(property.typeName());
    propertyDesc = desc;
    addScriptFunction(obj, property.name(), QScriptValue::ReadOnly | QScriptValue::Undeletable | QScriptValue::PropertyGetter | QScriptValue::PropertySetter, [this, type, property, objectChildId, parentType] (QScriptContext* context, QScriptEngine* engine) {
            QScriptValue pathStr = context->thisObject().data().property("dbus_path");
            if (!pathStr.isValid())
                return context->throwError(QString("Error while getting this object: Expected a voxie object"));
            QObject* obj = voxie::scripting::ScriptableObject::lookupWeakObject(QDBusObjectPath(pathStr.toString()));
            if (!obj)
                return context->throwError(QString("Could not find this object, object probably destroyed"));
            if (parentType) {
                if (obj->metaObject() != parentType)
                    return context->throwError(QString("Invalid type for this object (parent), expected %1, got %2").arg(type->className()).arg(obj->metaObject()->className()));
                bool found = false;
                for (auto child : obj->findChildren<QDBusAbstractAdaptor*>(QString(), Qt::FindDirectChildrenOnly)) {
                    const QMetaObject* adptrMetaObject = child->metaObject();
                    if (objectChildId == adptrMetaObject->className()) {
                        if (found)
                            return context->throwError(QString("Got multiple children with ID %1").arg(objectChildId));
                        obj = child;
                        found = true;
                    }
                }
                if (!found)
                    return context->throwError(QString("Did not find child with ID %1").arg(objectChildId));
            }
            if (obj->metaObject() != type)
                return context->throwError(QString("Invalid type for this object, expected %1, got %2").arg(type->className()).arg(obj->metaObject()->className()));

            if (context->argumentCount() == 0) {
                if (!property.isReadable())
                    return context->throwError(QString("Property %1 is not readable").arg(property.name()));
                QVariant value = property.read(obj);
                return toScriptValue(value);
            } else if (context->argumentCount() == 1) {
                if (!property.isWritable())
                    return context->throwError(QString("Property %1 is not writable").arg(property.name()));
                QVariant variant;
                if (!::fromScriptValue(variant, context, property.userType(), context->argument(0), QString ("value for property %1").arg(QString(property.name()))))
                    return engine->nullValue();
                if (!property.write(obj, variant))
                    return context->throwError(QString("Error while setting %1 property").arg(property.name()));
                return toScriptValue(42);
                return toScriptValue(variant);
            } else {
                return context->throwError(QString("Got %1 arguments for property method").arg(context->argumentCount()));
            }
        });
}

void ScriptWrapper::addMethodWrapper(const QScriptValue& obj, const QMetaObject* type, const QMetaMethod& method, QString& methodDesc, const QString& objectChildId, const QMetaObject* parentType) {
    // Ignore methods returning Array*Info
    if (method.returnType() == qMetaTypeId<voxie::scripting::Array2Info>()
        || method.returnType() == qMetaTypeId<voxie::scripting::Array3Info>())
        return;

    QString desc;
    desc += QString("%1 (").arg(QString(method.name()));
    for (int i = 0; i < method.parameterCount(); i++) {
        if (i != 0)
            desc += ", ";
        bool optional = i + 1 == method.parameterCount() && method.parameterType(i) == qMetaTypeId<QMap<QString, QVariant>>();
        if (optional)
            desc += "[";
        desc += QString("%1 %2")
            .arg(QMetaType::typeName(method.parameterType(i)))
            .arg(QString(method.parameterNames()[i]));
        if (optional)
            desc += "]";
    }
    desc += QString(") -> %1").arg(QMetaType::typeName(method.returnType()));
    methodDesc = desc;
    bool lastArgOptional = method.parameterCount() > 0 && method.parameterType(method.parameterCount() - 1) == qMetaTypeId<QMap<QString, QVariant>>();
    addScriptFunction(obj, method.name(), QScriptValue::ReadOnly | QScriptValue::Undeletable, [this, type, method, lastArgOptional, objectChildId, parentType] (QScriptContext* context, QScriptEngine* engine) {
            QScriptValue pathStr = context->thisObject().data().property("dbus_path");
            if (!pathStr.isValid())
                return context->throwError(QString("Error while getting this object: Expected a voxie object"));
            QObject* obj = voxie::scripting::ScriptableObject::lookupWeakObject(QDBusObjectPath(pathStr.toString()));
            if (!obj)
                return context->throwError(QString("Could not find this object, object probably destroyed"));
            if (parentType) {
                if (obj->metaObject() != parentType)
                    return context->throwError(QString("Invalid type for this object (parent), expected %1, got %2").arg(type->className()).arg(obj->metaObject()->className()));
                bool found = false;
                for (auto child : obj->findChildren<QDBusAbstractAdaptor*>(QString(), Qt::FindDirectChildrenOnly)) {
                    const QMetaObject* adptrMetaObject = child->metaObject();
                    if (objectChildId == adptrMetaObject->className()) {
                        if (found)
                            return context->throwError(QString("Got multiple children with ID %1").arg(objectChildId));
                        obj = child;
                        found = true;
                    }
                }
                if (!found)
                    return context->throwError(QString("Did not find child with ID %1").arg(objectChildId));
            }
            if (obj->metaObject() != type)
                return context->throwError(QString("Invalid type for this object, expected %1, got %2").arg(type->className()).arg(obj->metaObject()->className()));

            if (context->argumentCount() != method.parameterCount()
                && (context->argumentCount() + 1 != method.parameterCount() || !lastArgOptional)) {
                return context->throwError(QString("Invalid number of parameters (%1%2 expected, %3 given)").arg(lastArgOptional ? QString("%1 or ").arg(method.parameterCount() - 1) : "").arg(method.parameterCount()).arg(context->argumentCount()));
            }
            if (method.parameterCount() > 10)
                return context->throwError(QString("More than 10 parameters"));
            QList<QGenericArgument> params;
            for (int i = 0; i < 10; i++)
                params.push_back(QGenericArgument());
            QList<QVariant> list;
            for (int i = 0; i < method.parameterCount(); i++) {
                QScriptValue value;
                if (i < context->argumentCount())
                    value = context->argument(i);
                else
                    value = engine->newObject(); // last parameter is defaulted to {} if it is of type QMap<QString, QVariant>
                QVariant variant;
                if (!::fromScriptValue(variant, context, method.parameterType(i), value, QString ("argument %2 for method %1").arg(QString(method.name())).arg(i)))
                    return engine->nullValue();
                size_t pos = list.size();
                list.append(variant);
                params[i] = QGenericArgument(list[pos].typeName(), list[pos].data());
            }
            QVariant retValue(method.returnType() == qMetaTypeId<void>() ? qMetaTypeId<int>() : method.returnType(), nullptr);
            QGenericReturnArgument retArg(retValue.typeName(), retValue.data());
            if (method.returnType() == qMetaTypeId<void>())
                retArg = QGenericReturnArgument();
            bool ret;
            QScriptValue error;
            voxie::scripting::ScriptingException::executeWithHandler([&] {
                    ret = method.invoke(obj, Qt::DirectConnection,
                                        retArg,
                                        //Q_RETURN_ARG(int, i),
                                        params[0], params[1], params[2],
                                        params[3], params[4], params[5],
                                        params[6], params[7], params[8],
                                        params[9]);
                }, [&] (const voxie::scripting::ScriptingException& e) {
                    //error = context->throwError(QString("%1: %2").arg(e.name()).arg(e.message()));
                    QScriptValue errorObject = engine->newObject();
                    errorObject.setProperty("name", e.name());
                    errorObject.setProperty("message", e.message());
                    errorObject.setProperty("toString", engine->newFunction(errorToString));
                    error = context->throwValue(errorObject);
                });
            if (error.isValid())
                return error;
            if (!ret)
                return context->throwError(QString("Call to %1 failed").arg(QString(method.name())));
            //return engine->newObject();
            //return QScriptValue(ret);
            //return engine->newVariant(retValue);
            if (method.returnType() == qMetaTypeId<void>())
                return engine->undefinedValue();
            return toScriptValue(retValue);
        });
}


// The obj->exportScriptable() value and the set of QDBusAbstractAdaptor children must be the same for every object with the same metaObject
QScriptValue ScriptWrapper::getWrapperPrototype(voxie::scripting::ScriptableObject* obj) {
    const QMetaObject* metaObject = obj->metaObject();

    QMutexLocker locker(&prototypeMutex); // Keep locked until function returns
    if (prototypes.contains(metaObject))
        return prototypes[metaObject];

    QScriptValue protoData = engine->newObject();

    QScriptValue proto = engine->newObject();
    proto.setData(protoData);

    QList<QString> methodDesc;
    QList<QString> propertyDesc;

    addScriptFunction(proto, "toString", QScriptValue::ReadOnly | QScriptValue::Undeletable, [metaObject] (QScriptContext* context, QScriptEngine* engine) {
            Q_UNUSED(context);
            Q_UNUSED(engine);
            return QString ("[%1 %2]").arg(metaObject->className()).arg(context->thisObject().data().property("dbus_path").toString());
        });

    int firstProperty = QObject::staticMetaObject.propertyCount();
    int firstMethod = QObject::staticMetaObject.methodCount();

    if (obj->exportScriptable()) {
        for (int i = firstProperty; i < metaObject->propertyCount(); i++) {
            QMetaProperty property = metaObject->property(i);
            if (!property.isScriptable())
                continue;
            //qDebug() << "Property" << property.name();
            QString desc;
            addPropertyWrapper(proto, metaObject, property, desc);
            if (desc != "")
                propertyDesc << desc;
        }
        for (int i = firstMethod; i < metaObject->methodCount(); i++) {
            QMetaMethod method = metaObject->method(i);
            if (method.methodType() != QMetaMethod::Method && method.methodType() != QMetaMethod::Slot)
                continue;
            if (!(method.attributes() & QMetaMethod::Attributes::Scriptable))
                continue;
            //qDebug() << "Method" << method.name() << method.attributes();
            QString desc;
            addMethodWrapper(proto, metaObject, method, desc);
            if (desc != "")
                methodDesc << desc;
        }
    }

    QSet<QString> seenChildren;
    for (auto child : obj->findChildren<QDBusAbstractAdaptor*>(QString(), Qt::FindDirectChildrenOnly)) {
        const QMetaObject* adptrMetaObject = child->metaObject();
        QString objectChildId = adptrMetaObject->className();
        if (seenChildren.contains(objectChildId)) {
            qCritical() << "Warning: Got child" << objectChildId << "twice";
            continue;
        }
        seenChildren << objectChildId;
        for (int i = firstProperty; i < adptrMetaObject->propertyCount(); i++) {
            QMetaProperty property = adptrMetaObject->property(i);
            //qDebug() << "Adap Property" << property.name();
            QString desc;
            addPropertyWrapper(proto, adptrMetaObject, property, desc, objectChildId, metaObject);
            if (desc != "")
                propertyDesc << desc;
        }
        for (int i = firstMethod; i < adptrMetaObject->methodCount(); i++) {
            QMetaMethod method = adptrMetaObject->method(i);
            if (method.methodType() != QMetaMethod::Method && method.methodType() != QMetaMethod::Slot)
                continue;
            //qDebug() << "Adap Method" << method.name();
            QString desc;
            addMethodWrapper(proto, adptrMetaObject, method, desc, objectChildId, metaObject);
            if (desc != "")
                methodDesc << desc;
        }
	}
    //qDebug();

    qSort(propertyDesc);
    qSort(methodDesc);
    QString desc;
    desc += "  Properties:\n";
    for (const auto& d : propertyDesc)
        desc += "    " + d + "\n";
    desc += "  Methods:\n";
    for (const auto& d : methodDesc)
        desc += "    " + d + "\n";
    protoData.setProperty("description", desc);

    prototypes[metaObject] = proto;
    return proto;
}

QScriptValue ScriptWrapper::getWrapper(voxie::scripting::ScriptableObject* obj) {
    QMutexLocker locker(&objectMutex); // Keep locked until function returns
    if (objects.contains(obj))
        return objects[obj];

    QScriptValue proto = getWrapperPrototype(obj);

    QScriptValue data = engine->newObject();
    data.setProperty("dbus_path", obj->getPath().path());

    QScriptValue retobj = engine->newObject();
    retobj.setPrototype(proto);
    retobj.setData(data);

    QObject::connect(obj, &QObject::destroyed, this, [this, obj]() {
            QMutexLocker lock(&objectMutex);
            objects.remove(obj);
        });
    objects[obj] = retobj;
    return retobj;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
