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

#include "HelpLinkHandler.hpp"

#include <Voxie/Component/HelpCommon.hpp>

#include <Main/Gui/HelpWindow.hpp>
#include <Main/Gui/SidePanel.hpp>

#include <Main/Root.hpp>

#include <VoxieBackend/Component/Extension.hpp>

#include <QDesktopServices>

using namespace vx::help;
using namespace vx;

HelpLinkHandler::HelpLinkHandler(gui::HelpWindow* window) : window(window) {}

void HelpLinkHandler::handleLink(QUrl url) {
  if (url.scheme() == Protocol) {
    QString suffix = "//" + url.authority() + url.path();
    if (suffix.startsWith(commands::Help)) {
      openHelp(url.toString());
    } else if (suffix.startsWith(commands::Create)) {
      createNodeByName(suffix.mid(commands::Create.length()));
    } else if (suffix.startsWith(commands::EditMarkdown)) {
      openMarkdown(suffix.mid(commands::EditMarkdown.length()));
    } else if (suffix.startsWith(commands::EditSource)) {
      openSourceCode(suffix.mid(commands::EditSource.length()));
    } else if (suffix.startsWith(commands::DBus)) {
      QString path = suffix.mid(commands::DBusNoSlash.length());

      // qDebug() << "Opening link:" << path;
      auto obj = ExportedObject::lookupWeakObject(QDBusObjectPath(path));
      // auto obj = ExportedObject::lookupObject(QDBusObjectPath(path));
      if (!obj) {
        qWarning() << "Attempting to open link pointing to non-existing object:"
                   << url;
        return;
      }

      // Support links for other object types?
      auto node = dynamic_cast<Node*>(obj);
      // auto node = dynamic_cast<Node*>(obj.data());
      if (!node) {
        qWarning() << "Attempting to open link pointing to object which is not "
                      "a Node:"
                   << url;
        return;
      }

      Root::instance()
          ->mainWindow()
          ->sidePanel->dataflowWidget->clearSelectedNodes();
      Root::instance()->mainWindow()->sidePanel->dataflowWidget->selectNode(
          node);

    } else {
      qWarning() << "Unhandled URL:" << url;
    }
  } else if (url.scheme() == "doi") {
    QDesktopServices::openUrl(
        "https://dx.doi.org/" +
        QString::fromUtf8(QUrl::toPercentEncoding(url.path())));
  } else {
    QDesktopServices::openUrl(url);
  }
}

void HelpLinkHandler::openMarkdown(QString prototypeName) {
  // TODO: This currently does not work if the help file is in the script folder

  QString filePath = Root::instance()->directoryManager()->docPrototypePath() +
                     "/" + prototypeName + ".md";

  // need to create the file first?
  if (!QFileInfo(filePath).exists()) {
    QFile file(filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.close();
  }

  QDesktopServices::openUrl(
      QUrl::fromLocalFile(QFileInfo(filePath).absoluteFilePath()));
}

void HelpLinkHandler::openSourceCode(QString prototypeName) {
  if (Root::instance()->prototypeMap().contains(prototypeName)) {
    auto extension = qSharedPointerDynamicCast<Extension>(
        Root::instance()->prototypeMap()[prototypeName]->container());
    if (extension) {
      auto scriptFilename = extension->scriptFilename();
      QDesktopServices::openUrl(QUrl::fromLocalFile(scriptFilename));
    } else {
      qWarning() << "Could not find source for prototype" << prototypeName;
    }
  } else {
    qWarning() << "Attempt open source code for invalid prototype"
               << prototypeName << "from URL";
  }
}

void HelpLinkHandler::createNodeByName(QString prototypeName) {
  if (Root::instance()->prototypeMap().contains(prototypeName)) {
    Root::instance()->prototypeMap()[prototypeName]->create({}, {}, {});
    Root::instance()->mainWindow()->activateWindow();
  } else {
    qWarning() << "Attempt create invalid node" << prototypeName << "from URL";
  }
}

void HelpLinkHandler::openHelp(QString uri) {
  // Enqueue this for later to avoid problems if this is called as a link
  // handler
  if (!this->window) {
    enqueueOnMainThread(
        [uri]() { Root::instance()->helpWindow()->openHelpForUri(uri); });
  } else {
    // Use the help window where the link was clicked (if there are multiple
    // help windows at some point)
    QPointer<vx::gui::HelpWindow> helpWin = window;
    enqueueOnMainThread([uri, helpWin]() {
      if (helpWin) helpWin->openHelpForUri(uri);
    });
  }
}
