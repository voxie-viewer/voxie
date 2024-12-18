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

#include "ExportedObject.hpp"

#include <VoxieClient/Format.hpp>
#include <VoxieClient/ObjectExport/BusManager.hpp>

#include <cstdint>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtCore/QRegExp>
#include <QtCore/QThread>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

#ifndef _MSC_VER
#include <cxxabi.h>
#endif

using namespace vx;

static QRegExp invalidChars("[^A-Za-z0-9_]+", Qt::CaseSensitive);

// TODO: move fields into a new class?
static QMutex mutex;
static QMap<QString, uint64_t> currentId;
static QMap<QDBusObjectPath, ExportedObject*> weakReferences;
static QMap<QDBusObjectPath, QWeakPointer<RefCountedObject>> references;

ExportedObject::ExportedObject(const QString& type, QObject* parent,
                               bool singleton)
    : QObject(parent) {
  auto coreApplicationInstance = QCoreApplication::instance();
  if (!coreApplicationInstance) {
    qCritical() << "Warning: Trying to create object" << this
                << "while no QCoreApplication exists";
  } else {
    if (QThread::currentThread() != coreApplicationInstance->thread())
      qCritical() << "Warning: Trying to create object" << this << "on thread "
                  << QThread::currentThread();
  }

  if (singleton) {
    path = QString("/de/uni_stuttgart/Voxie%1%2")
               .arg(type == "" ? "" : "/")
               .arg(type);
  } else {
    uint64_t id;
    {
      QMutexLocker lock(&mutex);
      id = ++currentId[type];
    }
    path = QString("/de/uni_stuttgart/Voxie/%1/%2").arg(type).arg(id);
  }
  getBusManager()->registerObject(this, singleton,
                                  QDBusConnection::ExportAdaptors);

  {
    QMutexLocker lock1(&mutex);
    // Note: The lambda must not capture this (because it will be executed after
    // the destructor for ExportedObject has been run), instead path has to be
    // copied and captured by value
    QObject::connect(this, &QObject::destroyed, [path = path]() {
      QMutexLocker lock2(&mutex);
      weakReferences.remove(QDBusObjectPath(path));
    });
    weakReferences.insert(QDBusObjectPath(path), this);
  }
}

ExportedObject::~ExportedObject() {
  auto coreApplicationInstance = QCoreApplication::instance();
  if (coreApplicationInstance &&
      QThread::currentThread() != coreApplicationInstance->thread())
    qCritical() << "Warning: Trying to destroy object on thread "
                << QThread::currentThread();
}

void ExportedObject::checkOptions(const QMap<QString, QDBusVariant>& options,
                                  const QSet<QString>& allowed) {
  QSet<QString> optional;
  if (options.contains("Optional")) {
    auto value = options["Optional"].variant();
    if (value.type() != QVariant::StringList)
      throw Exception("de.uni_stuttgart.Voxie.InvalidOptionValue",
                      "Invalid type for 'Optional' option");
    optional = toQSet(value.toStringList());
  }
  for (auto key : options.keys()) {
    if (!allowed.contains(key) && !optional.contains(key) && key != "Optional")
      throw Exception("de.uni_stuttgart.Voxie.InvalidOption",
                      "Unknown option '" + key + "'");
  }
}

Q_NORETURN void ExportedObject::throwMissingOption(const QString& name) {
  throw vx::Exception("de.uni_stuttgart.Voxie.MissingOptionValue",
                      "No value given for '" + name + "' option");
}
Q_NORETURN void ExportedObject::throwInvalidOption(const QString& name,
                                                   const QString& expected,
                                                   const QString& actual) {
  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidOptionValue",
                      "Invalid type for '" + name + "' option: got " + actual +
                          ", expected " + expected);
}

QDBusObjectPath ExportedObject::getPath() const {
  return QDBusObjectPath(path);
}

ExportedObject* ExportedObject::lookupWeakObject(const QDBusObjectPath& path) {
  QMutexLocker lock(&mutex);
  auto it = weakReferences.find(path);
  if (it == weakReferences.end()) return nullptr;
  return *it;
}

RefCountedObject::RefCountedObject(const QString& type)
    : ExportedObject(type) {}

QWeakPointer<RefCountedObject> RefCountedObject::thisSharedWeakBase() {
  return thisPointerWeak;
}

QSharedPointer<RefCountedObject> RefCountedObject::thisSharedBase() {
  auto ptr = thisPointerWeak.lock();
  if (!ptr)
    throw Exception(
        "de.uni_stuttgart.Voxie.Error",
        "RefCountedObject::thisSharedBase() called before constructor has "
        "finished or during destruction");
  return ptr;
}

void RefCountedObject::initialize() {
  // Default implementation does nothing
}

void RefCountedObject::earlyTeardown() {
  // Default implementation does nothing
}

void RefCountedObject::registerObject(
    const QSharedPointer<RefCountedObject>& obj) {
  QMutexLocker lock1(&mutex);
  QDBusObjectPath path = obj->getPath();
  connect(obj.data(), &QObject::destroyed, [path]() {
    QMutexLocker lock2(&mutex);
    references.remove(path);
  });
  references.insert(path, obj.toWeakRef());
}
QSharedPointer<RefCountedObject> RefCountedObject::tryLookupObject(
    const QDBusObjectPath& path) {
  QMutexLocker lock(&mutex);
  auto it = references.find(path);
  if (it == references.end()) return QSharedPointer<RefCountedObject>();
  return it->toStrongRef();
}

// TODO: Move somewhere else?
namespace {
struct free_deleter {
  void operator()(void* p) const { std::free(p); }
};

QString demangleName(const char* mangled_name) {
#ifndef _MSC_VER
  size_t len;
  int status;
  std::unique_ptr<char, free_deleter> demangled(
      abi::__cxa_demangle(mangled_name, NULL, &len, &status));
  if (status == 0)
    return demangled.get();
  else if (status == -2)
    return vx::format("<unable to demangle: {}>", mangled_name);
  else if (status == -1)
    return vx::format("<unable to demangle (memory allocation failed): {}>",
                      mangled_name);
  else if (status == -3)
    return vx::format("<unable to demangle (invalid argument): {}>",
                      mangled_name);
  else
    return vx::format("<unable to demangle (status {}): {}>", status,
                      mangled_name);
#else
  return mangled_name;
#endif
}
}  // namespace

// TODO: Move somewhere else?
static QString getClassName(const std::type_info& typeinfo,
                            const QMetaObject* metaObject) {
  // Note: metaObject can be incorrect if the type does not have a Q_OBJECT
  // declaration
  // return vx::format("{} / {}", demangleName(typeinfo.name()),
  //                   metaObject ? metaObject->className() : "<nullptr>");
  Q_UNUSED(metaObject);
  return demangleName(typeinfo.name());
}
static QString getClassName(const std::type_info& typeinfo,
                            int pointerMetaType) {
  const QMetaObject* metaObject = nullptr;
  if (QMetaType::typeFlags(pointerMetaType))
    metaObject = QMetaType::metaObjectForType(pointerMetaType);
  return getClassName(typeinfo, metaObject);
}
static QString getClassNameForObj(QObject* obj) {
  if (!obj) return "nullptr";
  return getClassName(typeid(*obj), obj->metaObject());
}

void RefCountedObject::lookupFailedIncorrectTypeImpl(
    const std::type_info& expected, int expectedPointerMetaTypeId,
    const char* parameterName) {
  QString message =
      vx::format("Got unexpected argument type{}{}: Expected {}, got {}",
                 parameterName ? " for parameter " : "",
                 parameterName ? parameterName : "",
                 getClassName(expected, expectedPointerMetaTypeId),
                 getClassNameForObj(this));

  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidObjectType", message);
}
