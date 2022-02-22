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

#include <QtCore/QSharedPointer>
#include <QtCore/QString>

#include <Voxie/Voxie.hpp>

namespace vx {
class ExportedObject;
class NodePrototype;

namespace help {

static const QString Protocol = "voxie";

namespace commands {

// + prototype name
static const QString EditMarkdown = "///action/edit-markdown/";
// + prototype name
static const QString EditSource = "///action/edit-source/";
// + prototype name
static const QString Create = "///action/create-node/";

static const QString Help = "///help/";
// TODO: Change to "///help/component/de.uni_stuttgart.Voxie.NodePrototype/"?
// + prototype name
static const QString HelpPrototype = "///help/prototype/";
// + page name without '.md'
static const QString HelpTopic = "///help/topic/";
static const QString HelpStatic = "///help/static/";
static const QString HelpDBusInterface = "///help/dbus-interface/";

// + DBus path (without leading '/')
static const QString DBus = "///dbus/";
// + DBus path
static const QString DBusNoSlash = "///dbus";

}  // namespace commands

QString VOXIECORESHARED_EXPORT uriForPrototype(const QString& name);
QString VOXIECORESHARED_EXPORT
uriForPrototype(const QSharedPointer<vx::NodePrototype>& prototype);

QString VOXIECORESHARED_EXPORT uriForDBusObject(const QString& dbusPath);
QString VOXIECORESHARED_EXPORT uriForDBusObject(ExportedObject* obj);

QString VOXIECORESHARED_EXPORT uriForEditSource(const QString& filename);
QString VOXIECORESHARED_EXPORT
uriForEditSource(const QSharedPointer<vx::NodePrototype>& prototype);

QString VOXIECORESHARED_EXPORT uriForEditMarkdown(const QString& name);
QString VOXIECORESHARED_EXPORT
uriForEditMarkdown(const QSharedPointer<vx::NodePrototype>& prototype);

QString VOXIECORESHARED_EXPORT uriForCreateNode(const QString& name);
QString VOXIECORESHARED_EXPORT
uriForCreateNode(const QSharedPointer<vx::NodePrototype>& prototype);

QString VOXIECORESHARED_EXPORT uriForHelp(const QString& name);

QString VOXIECORESHARED_EXPORT uriForHelpTopic(const QString& name);

}  // namespace help
}  // namespace vx
