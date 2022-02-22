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

#include <VoxieClient/DBusClient.hpp>
#include <VoxieClient/DBusProxies.hpp>
#include <VoxieClient/Exception.hpp>
#include <VoxieClient/Exceptions.hpp>
#include <VoxieClient/VoxieDBus.hpp>

#include <QSharedPointer>

namespace vx {
class VOXIECLIENT_EXPORT ClaimedOperationBase : public QObject {
  Q_OBJECT

  QSharedPointer<de::uni_stuttgart::Voxie::Client> client_;
  de::uni_stuttgart::Voxie::ExternalOperation exop_gen;
  QSharedPointer<RefCountHolder> holder;
  volatile bool isCancelled_;

 public:
  static const QCommandLineOption& voxieActionOption();
  static const QCommandLineOption& voxieOperationOption();
  static const QList<QCommandLineOption>& options();

  ClaimedOperationBase(DBusClient& client, const QDBusObjectPath& path);
  ~ClaimedOperationBase();

  const QSharedPointer<de::uni_stuttgart::Voxie::Client>& client() const {
    return client_;
  }
  de::uni_stuttgart::Voxie::ExternalOperation& opGen() { return exop_gen; }

  template <typename F>
  void forwardExc(const F& fun) {
    try {
      fun();
    } catch (vx::Exception& error) {
      HANDLEDBUSPENDINGREPLY(opGen().FinishError(error.name(), error.message(),
                                                 vx::emptyOptions()));
      throw;
    }
  }

  // May also be called from other threads
  bool isCancelled() const { return isCancelled_; }

  // May also be called from other threads
  void throwIfCancelled() const {
    if (isCancelled()) throw vx::OperationCancelledException();
  }

  static QDBusObjectPath getOperationPath(const QCommandLineParser& options,
                                          const QString& action);
};

// This class is fully defined in the .hpp file, is not marked with
// VOXIECLIENT_EXPORT
template <typename ExOpType>
class ClaimedOperation : public ClaimedOperationBase {
  ExOpType exop;

 public:
  ClaimedOperation(DBusClient& client, const QDBusObjectPath& path)
      : ClaimedOperationBase(client, path),
        exop(client.uniqueName(), path.path(), client.connection()) {
    if (!exop.isValid())
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Error while getting external operation object: " +
                              exop.lastError().name() + ": " +
                              exop.lastError().message());
  }
  ~ClaimedOperation() {}

  ExOpType& op() { return exop; }
};
}  // namespace vx
