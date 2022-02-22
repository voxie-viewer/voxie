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

#include "ClaimedOperation.hpp"

using namespace vx;

namespace vx {
namespace {
QCommandLineOption voxieActionOptionValue("voxie-action", "Action to execute.",
                                          "action");
QCommandLineOption voxieOperationOptionValue("voxie-operation",
                                             "Operation object to use.",
                                             "operation");
QList<QCommandLineOption> optionsValue{voxieActionOptionValue,
                                       voxieOperationOptionValue};
}  // namespace
}  // namespace vx

const QCommandLineOption& ClaimedOperationBase::voxieActionOption() {
  return voxieActionOptionValue;
}
const QCommandLineOption& ClaimedOperationBase::voxieOperationOption() {
  return voxieOperationOptionValue;
}
const QList<QCommandLineOption>& ClaimedOperationBase::options() {
  return optionsValue;
}

ClaimedOperationBase::ClaimedOperationBase(DBusClient& client,
                                           const QDBusObjectPath& path)
    : client_(client.client()),
      exop_gen(client.uniqueName(), path.path(), client.connection()) {
  if (!exop_gen.isValid())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Error while getting generic external operation object: " +
            exop_gen.lastError().name() + ": " +
            exop_gen.lastError().message());

  HANDLEDBUSPENDINGREPLY(opGen().ClaimOperation(
      QDBusObjectPath(client.clientPath()), vx::emptyOptions()));
  holder.reset(new RefCountHolder(client.client(), path));

  isCancelled_ = false;
  QObject::connect(&opGen(),
                   &de::uni_stuttgart::Voxie::ExternalOperation::Cancelled,
                   this, [this] {
                     // qDebug() << "CANCELLED";
                     isCancelled_ = true;
                   });
  if (opGen().isCancelled())
    isCancelled_ = true;  // This is necessary to avoid a race condition when
                          // the operation is cancelled before the handler is
                          // registered
}
ClaimedOperationBase::~ClaimedOperationBase() {}

QDBusObjectPath ClaimedOperationBase::getOperationPath(
    const QCommandLineParser& options, const QString& action) {
  if (!options.isSet(voxieActionOption()) ||
      options.value(voxieActionOption()) != action)
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "--voxie-action is not '" + action + "'");

  if (!options.isSet(voxieOperationOption()))
    throw Exception("de.uni_stuttgart.Voxie.Error",
                    "--voxie-operation is not set");
  QString operationPath = options.value(voxieOperationOption());
  return QDBusObjectPath(operationPath);
}
