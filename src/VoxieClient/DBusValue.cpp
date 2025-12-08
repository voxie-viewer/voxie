/*
 * Copyright (c) 2014-2025 The Voxie Authors
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

#include "DBusValue.hpp"

#include <VoxieClient/DBusSignature.hpp>
#include <VoxieClient/DBusVariant.hpp>
#include <VoxieClient/QtUtil.hpp>

#include <QtCore/QDebug>
#include <QtCore/QMutex>

#include <QtDBus/QDBusMetaType>

class vx::DBusValue::InstanceBasic : public Instance {
 public:
  ~InstanceBasic() override {}

  DBusTypeKind kind() const override { return DBusTypeKind::Basic; }
  virtual char signatureChar() const = 0;
};

namespace {
template <typename T>
bool valuesAreEqual(const T& v1, const T& v2) {
  return v1 == v2;
}
template <>
bool valuesAreEqual(const QDBusUnixFileDescriptor& v1,
                    const QDBusUnixFileDescriptor& v2) {
  // TODO: What should this do?
  return v1.fileDescriptor() == v2.fileDescriptor();
}
}  // namespace

template <typename T>
class vx::DBusValue::InstanceBasicInst : public InstanceBasic {
 public:
  ~InstanceBasicInst() override {}

  T value;
  InstanceBasicInst(T value) : value(value) {}

  static QString getSignatureString() {
    const char s[]{BasicTypeTraits<T>::getSignatureChar(), 0};
    return QString(s);
  }

  char signatureChar() const override {
    return BasicTypeTraits<T>::getSignatureChar();
  }
  QDBusSignature signature() const override {
    return QDBusSignature(getSignatureString());
  };
  // TODO: Remove this and use value directly? (Same for other Instance types)
  const T& getValue() const { return value; }

  void marshalToQDBusArgument(QDBusArgument& arg) const override {
    arg << this->getValue();
  }

  bool equals(const Instance* other) const override {
    auto other2 = dynamic_cast<const InstanceBasicInst<T>*>(other);
    if (!other2) return false;
    return valuesAreEqual(this->value, other2->value);
  }
};

class vx::DBusValue::InstanceVariant : public Instance {
 public:
  DBusVariant variant;
  InstanceVariant(DBusVariant variant) : variant(variant) {}

  DBusTypeKind kind() const override { return DBusTypeKind::Variant; }
  QDBusSignature signature() const override { return QDBusSignature("v"); };
  const DBusVariant& getVariant() const { return variant; }

  void marshalToQDBusArgument(QDBusArgument& arg) const override {
    auto variant = getVariant().asQDBusVariant();
    arg << variant;
  }

  bool equals(const Instance* other) const override {
    auto other2 = dynamic_cast<const InstanceVariant*>(other);
    if (!other2) return false;
    return this->variant == other2->variant;
  }
};

namespace vx {
// Hack to get a metatype ID for a DBus signature
static int getMetaTypeForDBusSignature(const QDBusSignature& signature) {
  static QMutex mutex;
  static QMap<QString, int> cache;

  int id = QDBusMetaType::signatureToType(
      signature.signature().toUtf8().constData());
  if (id != QMetaType::UnknownType) return id;

  {
    QMutexLocker locker(&mutex);
    if (cache.contains(signature.signature()))
      return cache[signature.signature()];
  }

  QString typeName = "Voxie_DBusType_" + signature.signature();
  qDebug() << "Registering" << typeName;
  using MyType = int;
  id = QMetaType::registerType(
      typeName.toUtf8().constData(), [](void* ptr) { delete (MyType*)ptr; },
      [](const void* origPtr) {
        (void)origPtr;
        return (void*)new MyType;
      },
      QtMetaTypePrivate::QMetaTypeFunctionHelper<MyType>::Destruct,
      QtMetaTypePrivate::QMetaTypeFunctionHelper<MyType>::Construct,
      sizeof(MyType), QMetaType::TypeFlags(), nullptr);
  if (id == -1)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "getMetaTypeForDBusSignature: Error while trying to register type " +
            typeName);

  auto parsedSignature =
      vx::DBusSignatureBase::parseSingle(signature.signature());
  auto emptyValue = parsedSignature->createEmptyValue();

  // Note: This is a hack because TypedConstructor is only available with Qt5.12
  // (see Qt commit c2165f891c1a4380fdbf3bbf7af4696dc863cb99) and there is no
  // way to pass a value to the constructor or marshaller otherwise.
  vx::checkOnMainThread("getMetaTypeForDBusSignature");
  static std::shared_ptr<DBusValue> currentValue;
  struct Deleter {
    void* ptr;
    Deleter(void* ptr) : ptr(ptr) {}
    ~Deleter() {
      // Make sure this gets cleaned up even if there are exceptions
      if (currentValue.get() == ptr) currentValue.reset();
    }
  };
  auto newCurrentValue = std::make_shared<DBusValue>(emptyValue);
  Deleter deleter(newCurrentValue.get());
  if (currentValue)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "getMetaTypeForDBusSignature: Recursive invocation");
  currentValue = newCurrentValue;

  QDBusMetaType::registerMarshallOperators(
      id,
      [](QDBusArgument& arg, const void* ptr) {
        // Marshall function

        (void)ptr;

        auto currentValuePtr = currentValue;
        currentValue.reset();
        if (!currentValuePtr)
          // TODO: Should this throw here?
          throw vx::Exception(
              "de.uni_stuttgart.Voxie.InternalError",
              "getMetaTypeForDBusSignature: currentValue not set");
        auto emptyValueVal = *currentValuePtr;

        emptyValueVal.marshalToQDBusArgument(arg);
      },
      [](const QDBusArgument& arg, void* ptr) {
        // Demarshall function
        // Do nothing
        (void)arg;
        (void)ptr;
      });

  // This should trigger the marshalling
  auto resultSignature = QDBusMetaType::typeToSignature(id);
  if (resultSignature != signature.signature())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "getMetaTypeForDBusSignature: Got unexpected signature: Expected " +
            signature.signature() + ", got " + resultSignature);

  {
    QMutexLocker locker(&mutex);
    if (cache.contains(signature.signature())) {
      return cache[signature.signature()];
    } else {
      cache[signature.signature()] = id;
      return id;
    }
  }
}
}  // namespace vx

// TODO: Optimize cases like 'ay'?
class vx::DBusValue::InstanceArray : public Instance {
 public:
  ~InstanceArray() override {}

  QDBusSignature elementType;
  std::vector<DBusValue> values;
  InstanceArray(QDBusSignature elementType,
                const std::vector<DBusValue>& values)
      : elementType(elementType), values(values) {
    for (const auto& value : this->values)
      if (value.signature() != elementType)
        throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                            "Got invalid value in array: Expected DBus type " +
                                elementType.signature() + " but got " +
                                value.signature().signature());
  }

  DBusTypeKind kind() const override { return DBusTypeKind::Array; }
  QDBusSignature signature() const override {
    return QDBusSignature("a" + elementType.signature());
  };

  void marshalToQDBusArgument(QDBusArgument& arg) const override {
    arg.beginArray(getMetaTypeForDBusSignature(this->elementType));
    for (const auto& entry : this->values) entry.marshalToQDBusArgument(arg);
    arg.endArray();
  }

  bool equals(const Instance* other) const override {
    auto other2 = dynamic_cast<const InstanceArray*>(other);
    if (!other2) return false;

    if (this->elementType != other2->elementType) return false;
    if (this->values.size() != other2->values.size()) return false;
    for (size_t i = 0; i < this->values.size(); i++)
      if (this->values[i] != other2->values[i]) return false;
    return true;
  }
};

class vx::DBusValue::InstanceDictionary : public Instance {
 public:
  ~InstanceDictionary() override {}

  QDBusSignature keyType;
  QDBusSignature valueType;
  DictionaryType values;
  InstanceDictionary(QDBusSignature keyType, QDBusSignature valueType,
                     const DictionaryType& values)
      : keyType(keyType), valueType(valueType), values(values) {
    for (const auto& entry : this->values) {
      if (std::get<0>(entry).signature() != keyType)
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            "Got invalid value as dictionary key: Expected DBus type " +
                keyType.signature() + " but got " +
                std::get<0>(entry).signature().signature());
      if (std::get<1>(entry).signature() != valueType)
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.Error",
            "Got invalid value as dictionary value: Expected DBus type " +
                valueType.signature() + " but got " +
                std::get<1>(entry).signature().signature());
    }
  }

  DBusTypeKind kind() const override { return DBusTypeKind::Dictionary; }
  QDBusSignature signature() const override {
    return QDBusSignature("a{" + keyType.signature() + valueType.signature() +
                          "}");
  };

  void marshalToQDBusArgument(QDBusArgument& arg) const override {
    arg.beginMap(getMetaTypeForDBusSignature(this->keyType),
                 getMetaTypeForDBusSignature(this->valueType));
    for (const auto& entry : this->values) {
      arg.beginMapEntry();
      std::get<0>(entry).marshalToQDBusArgument(arg);
      std::get<1>(entry).marshalToQDBusArgument(arg);
      arg.endMapEntry();
    }
    arg.endMap();
  }

  bool equals(const Instance* other) const override {
    auto other2 = dynamic_cast<const InstanceDictionary*>(other);
    if (!other2) return false;

    if (this->values.size() != other2->values.size()) return false;
    for (size_t i = 0; i < this->values.size(); i++)
      if (this->values[i] != other2->values[i]) return false;
    return true;
  }
};

class vx::DBusValue::InstanceStruct : public Instance {
 public:
  ~InstanceStruct() override {}

  std::vector<DBusValue> values;
  InstanceStruct(const std::vector<DBusValue>& values) : values(values) {
    if (this->values.size() == 0)
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got empty structure");
  }

  DBusTypeKind kind() const override { return DBusTypeKind::Struct; }
  QDBusSignature signature() const override {
    QString result = "(";
    for (const auto& entry : this->values)
      result += entry.signature().signature();
    result += ")";
    return QDBusSignature(result);
  };

  void marshalToQDBusArgument(QDBusArgument& arg) const override {
    arg.beginStructure();
    for (const auto& entry : this->values) entry.marshalToQDBusArgument(arg);
    arg.endStructure();
  }

  bool equals(const Instance* other) const override {
    auto other2 = dynamic_cast<const InstanceStruct*>(other);
    if (!other2) return false;

    if (this->values.size() != other2->values.size())
      // TODO: This incorrectly returns false if the values are ordered
      // differently
      for (size_t i = 0; i < this->values.size(); i++)
        if (this->values[i] != other2->values[i]) return false;
    return true;
  }
};

vx::DBusValue::Instance::~Instance() {}

QDBusSignature vx::DBusValue::signature() const {
  return instance->signature();
}

template <typename T>
const T& vx::DBusValue::getBasicValueInst() const {
  // Should already be checked by getBasicValue()
  static_assert(BasicTypeTraits<T>::valid,
                "Internal: Invalid type for Basic type");

  // TODO: More checks?
  // - Check at runtime whether this is a basic value

  auto ptr = dynamic_cast<const InstanceBasicInst<T>*>(instance.get());
  if (!ptr)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Expected DBus type " +
                            InstanceBasicInst<T>::getSignatureString() +
                            " but got " + instance->signature().signature());
  return ptr->getValue();
}

char vx::DBusValue::getBasicSignatureChar() const {
  auto ptr = dynamic_cast<const InstanceBasic*>(instance.get());
  if (!ptr)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Expected DBus basic type but got " +
                            instance->signature().signature());
  return ptr->signatureChar();
}

vx::DBusValue vx::DBusValue::createVariant(const DBusVariant& variant) {
  return DBusValue(std::make_shared<InstanceVariant>(variant));
}

const vx::DBusVariant& vx::DBusValue::getVariant() const {
  auto ptr = dynamic_cast<const InstanceVariant*>(instance.get());
  if (!ptr)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Expected DBus type v but got " + instance->signature().signature());
  return ptr->getVariant();
}

vx::DBusValue vx::DBusValue::createArray(QDBusSignature elementType,
                                         const std::vector<DBusValue>& values) {
  return DBusValue(std::make_shared<InstanceArray>(elementType, values));
}

// TODO: Return values in a different form?
const std::vector<vx::DBusValue>& vx::DBusValue::getArray() const {
  auto ptr = dynamic_cast<const InstanceArray*>(instance.get());
  if (!ptr)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Expected DBus type a* but got " + instance->signature().signature());
  return ptr->values;
}
const QDBusSignature& vx::DBusValue::getArrayElementType() const {
  auto ptr = dynamic_cast<const InstanceArray*>(instance.get());
  if (!ptr)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Expected DBus type a* but got " + instance->signature().signature());
  return ptr->elementType;
}

vx::DBusValue vx::DBusValue::createDictionary(QDBusSignature keyType,
                                              QDBusSignature valueType,
                                              const DictionaryType& values) {
  return DBusValue(
      std::make_shared<InstanceDictionary>(keyType, valueType, values));
}

// TODO: Return values in a different form?
const vx::DBusValue::DictionaryType& vx::DBusValue::getDictionary() const {
  auto ptr = dynamic_cast<const InstanceDictionary*>(instance.get());
  if (!ptr)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Expected DBus type a{**} but got " +
                            instance->signature().signature());
  return ptr->values;
}
const QDBusSignature& vx::DBusValue::getDictionaryKeyType() const {
  auto ptr = dynamic_cast<const InstanceDictionary*>(instance.get());
  if (!ptr)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Expected DBus type a{**} but got " +
                            instance->signature().signature());
  return ptr->keyType;
}
const QDBusSignature& vx::DBusValue::getDictionaryValueType() const {
  auto ptr = dynamic_cast<const InstanceDictionary*>(instance.get());
  if (!ptr)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Expected DBus type a{**} but got " +
                            instance->signature().signature());
  return ptr->valueType;
}

vx::DBusValue vx::DBusValue::createStruct(
    const std::vector<DBusValue>& values) {
  return DBusValue(std::make_shared<InstanceStruct>(values));
}

// TODO: Return values in a different form?
const std::vector<vx::DBusValue>& vx::DBusValue::getStruct() const {
  auto ptr = dynamic_cast<const InstanceStruct*>(instance.get());
  if (!ptr)
    throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                        "Expected DBus type (...) but got " +
                            instance->signature().signature());
  return ptr->values;
}

vx::DBusValue vx::DBusValue::demarshalFromQDBusArgument(
    const std::shared_ptr<DBusSignatureBase>& signature,
    const QDBusArgument& arg) {
  if (auto sigBasic = dynamic_cast<DBusSignatureBasic*>(signature.get())) {
#define TYPE(sig, T)                                              \
  if (sigBasic->signatureCharacter() == sig) {                    \
    T value;                                                      \
    arg >> value;                                                 \
    /* TODO: Is there any way to check whether this succeeded? */ \
    return DBusValue::createBasic<T>(value);                      \
  }
    // TODO: Can this use the list from VX_DBUS_FOR_EACH_BASIC_TYPE? Can e.g.
    // uchar and quint8 be different?
    TYPE('y', uchar)
    TYPE('b', bool)
    TYPE('n', ushort)
    TYPE('q', short)
    TYPE('i', int)
    TYPE('u', uint)
    TYPE('x', qlonglong)
    TYPE('t', qulonglong)
    TYPE('d', double)
    TYPE('h', QDBusUnixFileDescriptor)

    TYPE('s', QString)
    TYPE('o', QDBusObjectPath)
    TYPE('g', QDBusSignature)
#undef TYPE

    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "demarshalFromQDBusArgument: Got invalid basic value");
  }

  if (arg.currentType() == QDBusArgument::BasicType) {
    // Got a "basic" type from QDBus event though it is not considered basic by
    // the DBus spec

    auto signatureString = signature->asString();

    // This is also considered 'basic' by QtDBus
    if (signatureString == "ay") {
      QByteArray value;
      arg >> value;
      return DBusValue::fromQByteArray(value);
    }

    // This is also considered 'basic' by QtDBus
    if (signatureString == "as") {
      QStringList value;
      arg >> value;
      return DBusValue::fromQStringList(value);
    }

    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "demarshalFromQDBusArgument: Got unknown basic value with signature " +
            signatureString);
  }

  if (auto sigVariant = dynamic_cast<DBusSignatureVariant*>(signature.get())) {
    (void)sigVariant;

    QDBusVariant value;
    // TODO: Is there any way to check whether this succeeded?
    arg >> value;
    return DBusValue::createVariant(DBusVariant::fromQDBusVariant(value));
  }

  if (auto sigArray = dynamic_cast<DBusSignatureArray*>(signature.get())) {
    // qDebug() << "A" << arg.currentSignature();
    arg.beginArray();
    // qDebug() << "B" << arg.currentSignature() << arg.atEnd() <<
    // arg.currentType();

    std::vector<DBusValue> values;
    while (!arg.atEnd()) {
      values.push_back(
          demarshalFromQDBusArgument(sigArray->elementType(), arg));
    }
    arg.endArray();
    return DBusValue::createArray(sigArray->elementType()->asQDBusSignature(),
                                  values);
  }

  if (auto sigDictionary =
          dynamic_cast<DBusSignatureDictionary*>(signature.get())) {
    // qDebug() << "A" << arg.currentSignature();
    arg.beginMap();
    // qDebug() << "B" << arg.currentSignature() << arg.atEnd() <<
    // arg.currentType();

    // std::map<DBusValue, DBusValue> values;
    std::vector<std::tuple<DBusValue, DBusValue>> values;
    while (!arg.atEnd()) {
      arg.beginMapEntry();
      auto key = demarshalFromQDBusArgument(sigDictionary->keyType(), arg);
      auto value = demarshalFromQDBusArgument(sigDictionary->valueType(), arg);
      // values[key] = value;
      values.push_back(std::make_tuple(key, value));
      arg.endMapEntry();
    }
    arg.endMap();
    return DBusValue::createDictionary(
        sigDictionary->keyType()->asQDBusSignature(),
        sigDictionary->valueType()->asQDBusSignature(), values);
  }

  throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                      "demarshalFromQDBusArgument: Got unknown value");
}

vx::DBusValue vx::DBusValue::fromQByteArray(const QByteArray& value) {
  // TODO: Make this faster?
  std::vector<DBusValue> values;
  for (const auto& entry : value)
    values.push_back(DBusValue::createBasic<quint8>(entry));
  return DBusValue::createArray(QDBusSignature("y"), values);
}

vx::DBusValue vx::DBusValue::fromQStringList(const QStringList& value) {
  std::vector<DBusValue> values;
  for (const auto& entry : value)
    values.push_back(DBusValue::createBasic<QString>(entry));
  return DBusValue::createArray(QDBusSignature("s"), values);
}

void vx::DBusValue::marshalToQDBusArgument(QDBusArgument& arg) const {
  instance->marshalToQDBusArgument(arg);
}

// Note: This assumes that the signature is known from the context
const QString vx::ToStringImpl<vx::DBusValue>::toString(
    const DBusValue& value) {
  auto kind = value.kind();
  switch (kind) {
    case DBusTypeKind::Basic: {
      char signatureChar = value.getBasicSignatureChar();
#define TYPE(sig, T) \
  if (signatureChar == sig) return vx::toString(value.getBasicValue<T>());
      VX_DBUS_FOR_EACH_BASIC_TYPE(TYPE)
#undef TYPE
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Unexpected Basic value");
    }

    case DBusTypeKind::Variant: {
      return vx::toString(value.getVariant());
    }

    case DBusTypeKind::Array: {
      const auto& values = value.getArray();
      QString res = "[";
      bool first = true;
      for (const auto& entry : values) {
        if (!first) res += ", ";
        res += vx::toString(entry);
        first = false;
      }
      res += "]";
      return res;
    }

    case DBusTypeKind::Dictionary: {
      const auto& values = value.getDictionary();
      QString res = "{";
      bool first = true;
      for (const auto& entry : values) {
        if (!first) res += ", ";
        res += vx::toString(std::get<0>(entry));
        res += ": ";
        res += vx::toString(std::get<1>(entry));
        first = false;
      }
      res += "}";
      return res;
    }

    case DBusTypeKind::Struct: {
      const auto& values = value.getStruct();
      QString res = "(";
      bool first = true;
      for (const auto& entry : values) {
        if (!first) res += ", ";
        res += vx::toString(entry);
        first = false;
      }
      res += ")";
      return res;
    }

    default:
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "Unexpected value for DBusTypeKind");
  }
}

#define TYPE(sig, T)                                                         \
  vx::DBusValue vx::DBusValue::BasicTypeTraits<T>::create(T value) {         \
    return DBusValue(std::make_shared<InstanceBasicInst<T>>(value));         \
  }                                                                          \
                                                                             \
  /* Explicit template instanciation */                                      \
  template VOXIECLIENT_EXPORT const T& vx::DBusValue::getBasicValueInst<T>() \
      const;

VX_DBUS_FOR_EACH_BASIC_TYPE(TYPE)
#undef TYPE

const QString vx::ToStringImpl<QDBusObjectPath>::toString(
    const QDBusObjectPath& value) {
  return value.path();
}
const QString vx::ToStringImpl<QDBusSignature>::toString(
    const QDBusSignature& value) {
  return value.signature();
}
const QString vx::ToStringImpl<QDBusUnixFileDescriptor>::toString(
    const QDBusUnixFileDescriptor& value) {
  return "fd=" + QString::number(value.fileDescriptor());
}

bool vx::operator==(const vx::DBusValue& v1, const vx::DBusValue& v2) {
  return v1.instance->equals(v2.instance.get());
}
bool vx::operator!=(const vx::DBusValue& v1, const vx::DBusValue& v2) {
  return !(v1 == v2);
}
