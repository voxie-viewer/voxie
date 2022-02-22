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

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QThread>

#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusObjectPath>

#include <VoxieClient/DBusUtil.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/QtUtil.hpp>

namespace vx {

/**
 * @brief Base class for all classes which export functionality over DBus.
 */
class VOXIECLIENT_EXPORT ExportedObject : public QObject, public QDBusContext {
  Q_OBJECT

 private:
  QString path;

 public:
  explicit ExportedObject(const QString& type, QObject* parent = 0,
                          bool singleton = false);
  virtual ~ExportedObject();

  QDBusObjectPath getPath() const;

  static QDBusObjectPath getPath(ExportedObject* obj) {
    if (!obj) return QDBusObjectPath("/");
    return obj->getPath();
  }
  static QDBusObjectPath getPath(const QSharedPointer<ExportedObject>& obj) {
    return getPath(obj.data());
  }

  static ExportedObject* lookupWeakObject(const QDBusObjectPath& path);

 private:
  static void addToQSet(QSet<QString>& set) { Q_UNUSED(set); }
  template <typename T, typename... U>
  static void addToQSet(QSet<QString>& set, T head, U... tail) {
    set.insert(head);
    addToQSet(set, tail...);
  }

  static Q_NORETURN void throwMissingOption(const QString& name);
  static Q_NORETURN void throwInvalidOption(const QString& name,
                                            const QString& expected,
                                            const QString& actual);

 public:
  static void checkOptions(const QMap<QString, QDBusVariant>& options,
                           const QSet<QString>& allowed);
  template <typename... T>
  static void checkOptions(const QMap<QString, QDBusVariant>& options,
                           T... allowed) {
    QSet<QString> set;
    addToQSet(set, allowed...);
    checkOptions(options, set);
  }

  static bool hasOption(const QMap<QString, QDBusVariant>& options,
                        const QString& name) {
    return options.find(name) != options.end();
  }
  template <typename T>
  static T getOptionValue(const QMap<QString, QDBusVariant>& options,
                          const QString& name) {
    auto value = options.find(name);
    if (value == options.end()) throwMissingOption(name);

    auto actualSig = dbusGetVariantSignature(*value);
    auto expectedSig = dbusGetSignature<T>();
    if (actualSig != expectedSig)
      throwInvalidOption(name, expectedSig.signature(), actualSig.signature());

    return dbusGetVariantValue<T>(*value);
  }

  template <typename T>
  static T getOptionValueOrDefault(const QMap<QString, QDBusVariant>& options,
                                   const QString& name, const T& def) {
    if (hasOption(options, name))
      return getOptionValue<T>(options, name);
    else
      return def;
  }
};

/**
 * @brief Base class for reference counted ExportedObjects.
 */
class VOXIECLIENT_EXPORT RefCountedObject : public ExportedObject {
  Q_OBJECT

  static void registerObject(const QSharedPointer<RefCountedObject>& obj);

  QWeakPointer<RefCountedObject> thisPointerWeak;

 protected:
  /**
   * @brief This function will be called after the object has been fully
   * constructed and thisSharedBase()/thisShared() is available.
   *
   * It will be called on the objects thread before that thread's main loop is
   * reentered.
   */
  virtual void initialize();

  /**
   * @brief This function will be called when the reference count drop to zero.
   * The last QSharedPointer destructor will not return before the function
   * returns.
   *
   * It will be called on whatever threads drop the last reference.
   */
  virtual void earlyTeardown();

  // TODO: add a teardown() method which will be called on object destruction?
  // Problem is that at that point the QSharedPointer will already be gone, the
  // only advantage of teardown() over the dtor would be that all virtual
  // functions are still available.
  // virtual void teardown();

  // Does not throw
  QWeakPointer<RefCountedObject> thisSharedWeakBase();

  // throws Exception if registerObject() has not been called
  QSharedPointer<RefCountedObject> thisSharedBase();

  // throws Exception if registerObject() has not been called
  template <typename T>
  QSharedPointer<T> thisSharedCasted() {
    auto ptr = qSharedPointerDynamicCast<T>(thisSharedBase());
    if (!ptr)
      throw Exception("de.uni_stuttgart.Voxie.Error",
                      "Invalid cast in thisSharedCasted()");
    return ptr;
  }

 public:
  explicit RefCountedObject(const QString& type);

  static QSharedPointer<RefCountedObject> tryLookupObject(
      const QDBusObjectPath& path);

  template <typename T, typename... Args>
  static QSharedPointer<T> createBase(Args&&... args);
};

template <typename T, typename... Args>
QSharedPointer<T> RefCountedObject::createBase(Args&&... args) {
  return executeOnMainThread([&args...] {
    auto data = QSharedPointer<T>(new T(std::forward<Args>(args)...),
                                  [](RefCountedObject* obj) {
                                    // qDebug() << "Object is being destroyed"
                                    // << obj->getPath().path();
                                    obj->earlyTeardown();
                                    obj->deleteLater();
                                  });
    QSharedPointer<vx::RefCountedObject> dataRCO = data;
    dataRCO->thisPointerWeak = dataRCO;
    dataRCO->initialize();
    registerObject(dataRCO);
    return data;
  });
}

#define REFCOUNTEDOBJ_DECL(CLSNAME)                                            \
 public:                                                                       \
  QSharedPointer<CLSNAME> thisShared() {                                       \
    return this->thisSharedCasted<CLSNAME>();                                  \
  }                                                                            \
                                                                               \
 public:                                                                       \
  template <typename... Args>                                                  \
  static QSharedPointer<CLSNAME> create(Args&&... args) {                      \
    return vx::RefCountedObject::createBase<CLSNAME, Args...>(                 \
        std::forward<Args>(args)...);                                          \
  }                                                                            \
                                                                               \
  static QSharedPointer<CLSNAME> lookup(const QDBusObjectPath& path) {         \
    auto obj = vx::RefCountedObject::tryLookupObject(path);                    \
    if (!obj)                                                                  \
      throw vx::Exception("de.uni_stuttgart.Voxie.ObjectNotFound",             \
                          "Object " + path.path() + " not found");             \
    auto objCst = qSharedPointerDynamicCast<CLSNAME>(obj);                     \
    if (!objCst)                                                               \
      throw vx::Exception("de.uni_stuttgart.Voxie.InvalidObjectType",          \
                          "Object " + path.path() + " is not a " #CLSNAME);    \
    return objCst;                                                             \
  }                                                                            \
  static QSharedPointer<CLSNAME> lookupOptional(const QDBusObjectPath& path) { \
    if (path.path() == "/") return QSharedPointer<CLSNAME>();                  \
    return lookup(path);                                                       \
  }                                                                            \
                                                                               \
 private:

}  // namespace vx
