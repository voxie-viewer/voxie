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

#include <Main/Root.hpp>

#include <Main/AllDebugOptions.hpp>

#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QFile>

#include <QtWidgets/QApplication>

static void setupDebugOptions(const QList<QString>& commandLineValues) {
  auto debugOptionList = vx::allDebugOptions();
  QMap<QString, vx::DebugOption*> debugOptions;
  for (const auto& option : *debugOptionList) {
    if (debugOptions.contains(option->name()))
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InternalError",
          QString() + "Got duplicate debug option name: " + option->name());
    debugOptions[option->name()] = option;
  }

  QList<QString> values;
  values << QString::fromUtf8(qgetenv("VOXIE_DEBUG_OPTIONS"));
  values << commandLineValues;

  for (const auto& str1 : values) {
    for (const auto& str2 : str1.split(":")) {
      if (str2 == "") continue;

      QString name, value;
      bool haveValue;
      int pos = str2.indexOf('=');
      if (pos == -1) {
        name = str2;
        haveValue = false;
      } else {
        name = str2.left(pos);
        value = str2.mid(pos + 1);
        haveValue = true;
      }

      if (!debugOptions.contains(name)) {
        qWarning() << "Unknown debug option" << name;
        continue;
      }
      vx::DebugOption* option = debugOptions[name];
      if (haveValue)
        option->setValueString(value);
      else
        option->setValueStringEmpty();
    }
  }
}

int main(int argc, char* argv[]) {
#ifdef QT_DEBUG
  qSetMessagePattern("%{message} (%{file}:%{line})");
#endif

  if (argc < 1) {
    qCritical("argc is smaller than 1");
    return 1;
  }

  QCommandLineParser parser;
  parser.setApplicationDescription("Test helper");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("files", "File to open.", "[files...]");

  QCommandLineOption dbusP2POption(
      "dbus-p2p",
      "Use DBus P2P connections instead of session bus (default on Windows and "
      "MacOS, currently implies --new-instance)");
  parser.addOption(dbusP2POption);
  QCommandLineOption dbusBusOption(
      "dbus-bus",
      "Use DBus session bus instead of P2P connections (default on Linux)");
  parser.addOption(dbusBusOption);

  QCommandLineOption noOpenGLOption("no-opengl", "Disable usage of OpenGL");
  parser.addOption(noOpenGLOption);

  QCommandLineOption noOpenCLOption(
      "no-opencl", "Disable usage of OpenCL (currently default)");
  parser.addOption(noOpenCLOption);

  QCommandLineOption openCLOption("opencl", "Enable usage of OpenCL");
  parser.addOption(openCLOption);

  QCommandLineOption newInstanceOption(
      "new-instance",
      "Create a new instance, even if voxie is already runnung");
  parser.addOption(newInstanceOption);

  QCommandLineOption qtOptionOption("qt-option", "Pass option to Qt", "option");
  parser.addOption(qtOptionOption);

  QCommandLineOption isoOption(
      "iso", "Create an isosurface view for every opened file");
  parser.addOption(isoOption);
  QCommandLineOption sliceOption("slice",
                                 "Create a slice view for every opened file");
  parser.addOption(sliceOption);
  QCommandLineOption rawOption("raw",
                               "Create a raw view for every opened file");
  parser.addOption(rawOption);
  QCommandLineOption importPropertyOption(
      "import-property", "Import property to use when opening files",
      "property=JSON");
  parser.addOption(importPropertyOption);

  QCommandLineOption openHelpPage("open-help-page",
                                  "Open a help page after startup", "URI");
  parser.addOption(openHelpPage);

  QCommandLineOption debugOutputPrototypes(
      "debug-output-prototypes", "Output JSON files for all object prototypes",
      "directory");
  parser.addOption(debugOutputPrototypes);

  QCommandLineOption outputHelpDirectory("output-help-directory",
                                         "Output HTML files continaing help",
                                         "directory");
  parser.addOption(outputHelpDirectory);

  QCommandLineOption batchOption(
      "batch",
      "Activate batch mode (quit after processing command line arguments, "
      "default to P2P DBus, hide main window)");
  parser.addOption(batchOption);

  QCommandLineOption mainWindowOption("main-window",
                                      "How to show the main window", "option");
  parser.addOption(mainWindowOption);

  QCommandLineOption debugOptionOption("debug-option", "Set a debug option",
                                       "option=value");
  parser.addOption(debugOptionOption);

  QStringList args;
  for (char** arg = argv; *arg; arg++) args.push_back(*arg);

  {
    int argc0 = 1;
    char* args0[2] = {argv[0], NULL};
    QCoreApplication app0(argc0, args0);
    parser.process(args);

    // Needed because if a 3d visualizer window is detached it is moved into a
    // new native window and reinitializing all OpenGL resources currently isn't
    // supported
    // Note: This causes problems when OpenGL is not supported
    if (!parser.isSet("no-opengl"))
      app0.setAttribute(Qt::AA_ShareOpenGLContexts, true);

    setupDebugOptions(parser.values(debugOptionOption));
  }

  QStringList qtOptionsList = parser.values(qtOptionOption);
  int qtOptionsArgc = qtOptionsList.size() + 1;
  std::vector<std::string> qtOptions;
  std::vector<char*> qtOptionsChar;
  qtOptionsChar.push_back(argv[0]);
  for (const QString& opt : qtOptionsList) {
    qtOptions.push_back(opt.toUtf8().data());
    // Cast away const-ness because QCoreApplication constructor wants a poitner
    // to non-const char (but hopefully should not modify the strings)
    qtOptionsChar.push_back(const_cast<char*>(qtOptions.back().c_str()));
  }
  qtOptionsChar.push_back(NULL);

  // TODO: Support proper headless operation, enable headless when --batch is
  // set
  bool headless = parser.isSet("output-help-directory");

  if (parser.isSet("main-window") && parser.value("main-window") == "headless")
    headless = true;

  QScopedPointer<QCoreApplication> app(
      headless ? new QCoreApplication(qtOptionsArgc, qtOptionsChar.data())
               : new QApplication(qtOptionsArgc, qtOptionsChar.data()));
  if (qtOptionsArgc != 1) {
    qCritical("Got invalid Qt options:");
    for (char** arg = qtOptionsChar.data(); *arg; arg++)
      if (arg != qtOptionsChar.data()) qCritical("  '%s'", *arg);
    return 1;
  }

  return vx::Root::startVoxie(*app, parser, headless);
}
