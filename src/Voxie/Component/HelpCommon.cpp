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

#include "HelpCommon.hpp"

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <Voxie/Node/NodePrototype.hpp>

// TODO: When the prototype name is ambiguous there should be some identifier
// clarifying which one is meant

QString vx::help::uriForPrototype(const QString& name) {
  return Protocol + ":" + commands::HelpPrototype + name;
}
QString vx::help::uriForPrototype(
    const QSharedPointer<vx::NodePrototype>& prototype) {
  return uriForPrototype(prototype->name());
}

QString vx::help::uriForDBusObject(const QString& dbusPath) {
  return Protocol + ":" + commands::DBusNoSlash + dbusPath;
}
QString vx::help::uriForDBusObject(ExportedObject* obj) {
  return uriForDBusObject(obj->getPath().path());
}

QString vx::help::uriForEditSource(const QString& prototypeName) {
  return Protocol + ":" + commands::EditSource + prototypeName;
}
QString vx::help::uriForEditSource(
    const QSharedPointer<vx::NodePrototype>& prototype) {
  return uriForEditSource(prototype->name());
}

QString vx::help::uriForEditMarkdown(const QString& name) {
  return Protocol + ":" + commands::EditMarkdown + name;
}
QString vx::help::uriForEditMarkdown(
    const QSharedPointer<vx::NodePrototype>& prototype) {
  return uriForEditMarkdown(prototype->name());
}

QString vx::help::uriForCreateNode(const QString& name) {
  return Protocol + ":" + commands::Create + name;
}
QString vx::help::uriForCreateNode(
    const QSharedPointer<vx::NodePrototype>& prototype) {
  return uriForCreateNode(prototype->name());
}

QString vx::help::uriForHelp(const QString& name) {
  return Protocol + ":" + commands::Help + name;
}
QString vx::help::uriForHelpTopic(const QString& name) {
  return Protocol + ":" + commands::HelpTopic + name;
}
