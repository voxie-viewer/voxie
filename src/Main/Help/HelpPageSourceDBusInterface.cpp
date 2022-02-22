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

#include "HelpPageSourceDBusInterface.hpp"

#include <Main/Root.hpp>

#include <Main/Help/CMark.hpp>
#include <Main/Help/CMarkUtil.hpp>
#include <Main/Help/HelpPage.hpp>

#include <VoxieClient/QtUtil.hpp>

#include <QtCore/QDebug>
#include <QtCore/QXmlStreamReader>

using namespace vx::help;

// https://stackoverflow.com/questions/8215303/how-do-i-remove-trailing-whitespace-from-a-qstring/8216059#8216059
/*
static QString rstrip(const QString& str) {
  int n = str.size() - 1;
  for (; n >= 0; --n) {
    if (!str.at(n).isSpace()) {
      return str.left(n + 1);
    }
  }
  return "";
}
*/
static QString lstrip(const QString& str) {
  for (int n = 0; n < str.size(); n++) {
    if (!str.at(n).isSpace()) {
      return str.mid(n);
    }
  }
  return "";
}

namespace vx {
namespace help {
// DBus comment doc parser. See
// https://gitlab.gnome.org/GNOME/glib/-/blob/8055dd27022fdbef809f09fd5471d03857f3378e/gio/gdbus-2.0/codegen/parser.py
class DBusXmlDoc {
  QString symbol_;
  QString body_;
  QList<std::tuple<QString, QString>> param_;
  QMap<QString, QString> paramByName_;

 public:
  DBusXmlDoc(const QString& symbol, const QString& body,
             const QList<std::tuple<QString, QString>>& param)
      : symbol_(symbol), body_(body), param_(param) {
    for (const auto& entry : param)
      paramByName_[std::get<0>(entry)] = std::get<1>(entry);
  }

  const QString& symbol() const { return symbol_; }
  const QString& body() const { return body_; }
  const QList<std::tuple<QString, QString>>& param() const { return param_; }
  const QMap<QString, QString>& paramByName() const { return paramByName_; }
};

class DBusXmlParser {
  QXmlStreamReader* reader_;

  QString lastDocName;
  QString lastDocBody;
  QList<std::tuple<QString, QString>> lastDocParam;

 public:
  DBusXmlParser(QXmlStreamReader* reader) : reader_(reader) {}

  QSharedPointer<DBusXmlDoc> getDoc(const QString& symbol) {
    if (symbol != lastDocName) return QSharedPointer<DBusXmlDoc>();

    return createQSharedPointer<DBusXmlDoc>(lastDocName, lastDocBody,
                                            lastDocParam);
  }

  QXmlStreamReader* reader() { return reader_; }

  void skipCurrentElement() {
    // qDebug() << "Start skipCurrentElement";
    for (;;) {
      auto token = readNext();
      if (token == QXmlStreamReader::Invalid ||
          token == QXmlStreamReader::EndElement)
        break;
      if (token == QXmlStreamReader::StartElement) skipCurrentElement();
    }
    // qDebug() << "End skipCurrentElement";
  }

  bool goToStartElement() {
    for (;;) {
      auto token = readNext();
      // qDebug() << token;
      if (token == QXmlStreamReader::Invalid ||
          token == QXmlStreamReader::EndElement)
        return false;
      if (token == QXmlStreamReader::StartElement) return true;
    }
  }

  // Strip common whitespace prefix which occurs in every line
  QString stripCommonWhitespacePrefix(const QString& text) {
    // TODO: Handle \t differently?

    auto lines = text.split('\n');

    // Search for empty lines
    bool haveNonEmpty = false;
    int firstNonEmpty = -1;
    QList<bool> isEmpty;
    for (int i = 0; i < lines.size(); i++) {
      bool empty = lines[i].trimmed() == "";
      isEmpty << empty;
      if (!empty) {
        if (!haveNonEmpty) firstNonEmpty = i;
        haveNonEmpty = true;
      }
    }
    if (!haveNonEmpty) return text;  // No non-empty line

    // Get length of whitespace at start of first non-empty line
    QString firstLine = lines[firstNonEmpty];
    int len = firstLine.size();
    for (int j = 0; j < firstLine.size(); j++) {
      if (!firstLine.at(j).isSpace()) {
        len = j;
        break;
      }
    }
    QString commonWhitespacePrefix = firstLine.left(len);

    // qDebug() << "LEN1" << commonWhitespacePrefix.size();
    // Check for each non-empty line whether it start with
    // commonWhitespacePrefix and short the prefix if necessary
    for (int i = 0; i < lines.size(); i++) {
      if (isEmpty[i]) continue;

      QString prefix = lines[i].left(commonWhitespacePrefix.size());
      if (prefix.size() < commonWhitespacePrefix.size())
        commonWhitespacePrefix = commonWhitespacePrefix.left(prefix.size());

      if (prefix == commonWhitespacePrefix) continue;

      for (int j = 0; j < prefix.size(); j++) {
        if (prefix[j] != commonWhitespacePrefix[j]) {
          commonWhitespacePrefix = commonWhitespacePrefix.left(j);
          break;
        }
      }
    }

    // qDebug() << "LEN2" << commonWhitespacePrefix.size();
    // Shorten all lines by the prefix
    QString result;
    for (int i = 0; i < lines.size(); i++) {
      if (i != 0) result += "\n";
      result += lines[i].mid(commonWhitespacePrefix.size());
    }
    // qDebug() << "INPUT" << text;
    // qDebug() << "OUTPUT" << result;
    return result;
  }

  // https://gitlab.gnome.org/GNOME/glib/-/blob/8055dd27022fdbef809f09fd5471d03857f3378e/gio/gdbus-2.0/codegen/parser.py#L71
  QXmlStreamReader::TokenType readNext() {
    auto tok = reader()->readNext();
    while (tok == QXmlStreamReader::Comment) {
      // qDebug() << tok;
      auto lines =
          stripCommonWhitespacePrefix(reader()->text().toString()).split('\n');

      int pos = 0;

      // Get first non-empty line
      QString line;
      while (pos < lines.size() && line == "") {
        line = lstrip(lines[pos]);
        pos++;
      }

      int index1 = line.indexOf(": ");
      // qDebug() << index << line;
      QString symbol;
      QString body;
      if (index1 == -1) {
        if (line.endsWith(":")) {
          symbol = line.left(line.size() - 1);
        } else {  // non-documentation comment
          tok = reader()->readNext();
          continue;
        }
      } else {
        symbol = line.left(index1);
        QString remaining = line.mid(index1 + 2).trimmed();
        if (remaining != "") body += remaining + "\n\n";
      }

      // TODO: Skip if symbol is "TODO"? (not done by dbus-codegen)

      lastDocName = symbol;
      lastDocBody = "";
      lastDocParam.clear();

      // qDebug() << "SYMBOL" << symbol;
      while (pos < lines.size() &&
             (line = lstrip(lines[pos])).startsWith("@")) {
        // qDebug() << "param" << line;
        int index2 = line.indexOf(": ");
        if (index2 == -1) break;
        lastDocParam << std::make_tuple(line.mid(1, index2 - 1),
                                        line.mid(index2 + 2));

        pos++;
      }

      while (pos < lines.size()) {
        body += lines[pos] + "\n";
        pos++;
      }

      lastDocBody = body;

      /*
      qDebug() << "doc" << lastDocName << lastDocBody;
      for (const auto& entry : lastDocParam)
        qDebug() << "  param " << std::get<0>(entry) << std::get<1>(entry);
      */

      tok = reader()->readNext();
    }
    // qDebug() << "QQ" << tok;
    return tok;
  }
};

class DBusXmlNodeBase {
 public:
  QMap<QString, QList<QString>> annotations;

  DBusXmlNodeBase(DBusXmlParser* parser) { Q_UNUSED(parser); }

 protected:
  void addAnnotation(DBusXmlParser* parser) {
    if (parser->reader()->name().toString() != "annotation") {
      qWarning()
          << "DBusXmlNode::addAnnotation called for non-annotation element";
      parser->skipCurrentElement();
      return;
    }

    auto name = parser->reader()->attributes().value("name").toString();
    auto value = parser->reader()->attributes().value("value").toString();
    // qDebug() << name << value;
    annotations[name] << value;

    // TODO: Check for children?
    parser->skipCurrentElement();
  }
};

class DBusXmlNode : public DBusXmlNodeBase {
 public:
  QString name;
  QSharedPointer<DBusXmlDoc> doc;

  DBusXmlNode(DBusXmlParser* parser) : DBusXmlNodeBase(parser) {
    if (!parser->reader()->attributes().hasAttribute("name")) {
      qWarning() << "Got node without 'name' attribute";
      name = "";
    } else {
      name = parser->reader()->attributes().value("name").toString();
    }

    doc = parser->getDoc(name);
  }
};

class DBusXmlMember : public DBusXmlNode {
 public:
  DBusXmlMember(DBusXmlParser* parser) : DBusXmlNode(parser) {}
};

// TODO: Should this have a 'doc' attribute?
class DBusXmlArgument : public DBusXmlNodeBase {
 public:
  QString name;
  QString type;
  QString direction;

  DBusXmlArgument(DBusXmlParser* parser, const QString& defaultDirection)
      : DBusXmlNodeBase(parser) {
    if (!parser->reader()->attributes().hasAttribute("name")) {
      // name is optional for arguments
      name = "";
    } else {
      name = parser->reader()->attributes().value("name").toString();
    }

    if (!parser->reader()->attributes().hasAttribute("direction")) {
      direction = defaultDirection;
    } else {
      direction = parser->reader()->attributes().value("direction").toString();
    }

    if (!parser->reader()->attributes().hasAttribute("type")) {
      qWarning() << "Got argument without 'type' attribute";
      type = "";
    } else {
      type = parser->reader()->attributes().value("type").toString();
    }

    while (parser->goToStartElement()) {
      QString childName = parser->reader()->name().toString();
      if (childName == "annotation")
        addAnnotation(parser);
      else {
        qWarning() << "Got unknown child in argument:" << childName;
        parser->skipCurrentElement();
      }
    }
  }
};

class DBusXmlMemberWithArguments : public DBusXmlMember {
 public:
  QList<QSharedPointer<DBusXmlArgument>> arguments;

  DBusXmlMemberWithArguments(DBusXmlParser* parser,
                             const QString& defaultArgDirection)
      : DBusXmlMember(parser) {
    while (parser->goToStartElement()) {
      QString childName = parser->reader()->name().toString();
      if (childName == "arg")
        arguments << createQSharedPointer<DBusXmlArgument>(parser,
                                                           defaultArgDirection);
      else if (childName == "annotation")
        addAnnotation(parser);
      else {
        qWarning() << "Got unknown child in member:" << childName;
        parser->skipCurrentElement();
      }
    }
  }
};

class DBusXmlProperty : public DBusXmlMember {
 public:
  QString type;
  QString access;

  DBusXmlProperty(DBusXmlParser* parser) : DBusXmlMember(parser) {
    if (!parser->reader()->attributes().hasAttribute("type")) {
      qWarning() << "Got property without 'type' attribute";
      type = "";
    } else {
      type = parser->reader()->attributes().value("type").toString();
    }

    if (!parser->reader()->attributes().hasAttribute("access")) {
      qWarning() << "Got property without 'access' attribute";
      access = "";
    } else {
      access = parser->reader()->attributes().value("access").toString();
    }

    while (parser->goToStartElement()) {
      QString childName = parser->reader()->name().toString();
      if (childName == "annotation")
        addAnnotation(parser);
      else {
        qWarning() << "Got unknown child in property:" << childName;
        parser->skipCurrentElement();
      }
    }
  }
};

class DBusXmlMethod : public DBusXmlMemberWithArguments {
 public:
  DBusXmlMethod(DBusXmlParser* parser)
      : DBusXmlMemberWithArguments(parser, "in") {}
};

class DBusXmlSignal : public DBusXmlMemberWithArguments {
 public:
  DBusXmlSignal(DBusXmlParser* parser)
      : DBusXmlMemberWithArguments(parser, "out") {}
};

class DBusXmlInterface : public DBusXmlNode {
 public:
  QList<QSharedPointer<DBusXmlMethod>> methods;
  QList<QSharedPointer<DBusXmlProperty>> properties;
  QList<QSharedPointer<DBusXmlSignal>> signals_;

  DBusXmlInterface(DBusXmlParser* parser) : DBusXmlNode(parser) {
    while (parser->goToStartElement()) {
      QString childName = parser->reader()->name().toString();
      if (childName == "method")
        methods << createQSharedPointer<DBusXmlMethod>(parser);
      else if (childName == "property")
        properties << createQSharedPointer<DBusXmlProperty>(parser);
      else if (childName == "signal")
        signals_ << createQSharedPointer<DBusXmlSignal>(parser);
      else if (childName == "annotation")
        addAnnotation(parser);
      else {
        qWarning() << "Got unknown child in interface:" << childName;
        parser->skipCurrentElement();
      }
    }
  }
};

class DBusXmlInfo {
 public:
  QList<QSharedPointer<DBusXmlInterface>> interfaces;
  QMap<QString, QSharedPointer<DBusXmlInterface>> interfacesByName;

  DBusXmlInfo() {
    QDirIterator it(":/dbusxml");
    while (it.hasNext()) {
      QString filename = it.next();
      // qDebug() << filename;
      if (!filename.endsWith(".xml")) continue;

      QFile file(filename);
      if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open " << filename;
        return;
      }
      QXmlStreamReader streamReader(&file);
      DBusXmlParser parser(&streamReader);

      // Wait for start of document
      while (!parser.reader()->atEnd() &&
             parser.readNext() != QXmlStreamReader::StartElement)
        ;

      while (!parser.reader()->atEnd()) {
        QXmlStreamReader::TokenType token = parser.readNext();

        if (token != QXmlStreamReader::StartElement) continue;

        if (parser.reader()->name().toString() != "interface") {
          parser.skipCurrentElement();
          continue;
        }

        auto interf = createQSharedPointer<DBusXmlInterface>(&parser);
        interfaces << interf;
        interfacesByName[interf->name] = interf;
      }
    }
  }
};

class HelpPageSourceDBusInterface : public HelpPageSource {
 public:
  HelpPageSourceDBusInterface() : HelpPageSource("dbus-interface") {}
  ~HelpPageSourceDBusInterface() override {}

  const QSharedPointer<DBusXmlInfo>& info() {
    static QSharedPointer<DBusXmlInfo> info =
        createQSharedPointer<DBusXmlInfo>();
    return info;
  }

  QList<QSharedPointer<HelpPageInfo>> listPages() override {
    QList<QSharedPointer<HelpPageInfo>> list{
        createQSharedPointer<HelpPageInfo>(urlPrefix(), "DBus interfaces", ""),
    };

    QList<QSharedPointer<DBusXmlInterface>> interfaces = info()->interfaces;
    std::sort(interfaces.begin(), interfaces.end(),
              [](auto a, auto b) { return a->name < b->name; });
    for (const auto& interf : interfaces) {
      list << createQSharedPointer<HelpPageInfo>(
          urlPrefix() + "/" + interf->name, interf->name,
          interf->doc &&
                  interf->doc->paramByName().contains("short_description")
              ? interf->doc->paramByName()["short_description"].trimmed()
              : "");
    }

    return list;
  }

  void addDocShort(const QSharedPointer<vx::cmark::Node>& doc,
                   const QSharedPointer<DBusXmlNode>& node) {
    if (node->doc && node->doc->paramByName().contains("short_description"))
      addTextParagraph(doc,
                       node->doc->paramByName()["short_description"].trimmed());
  }

  void addDoc(const QSharedPointer<vx::cmark::Node>& doc,
              const QSharedPointer<DBusXmlNode>& node) {
    if (node->doc) {
      // qDebug() << "Got doc" << node->doc->body();
      auto rootNode = vx::cmark::Node::parseDocument(node->doc->body());

      // Move all nodes from rootNode to doc
      doc->appendAllChildrenOf(rootNode);
    }
  }

  void addAnnotations(const QSharedPointer<vx::cmark::Node>& doc,
                      const QSharedPointer<DBusXmlNodeBase>& node) {
    QList<QString> annotationKeys = node->annotations.keys();
    std::sort(annotationKeys.begin(), annotationKeys.end());
    for (const auto& annotation : annotationKeys) {
      QString str = "Annotation '" + annotation + "':";
      for (const auto& entry : node->annotations[annotation])
        str += " '" + entry + "'";
      addTextParagraph(doc, str);
    }
  }

  QSharedPointer<HelpPage> render(const QString& suffix) override {
    auto parts = suffix.split('/');

    if (parts.size() == 1 && parts[0] == "") {
      // Generate markdown document
      auto doc = vx::cmark::Node::newNode(CMARK_NODE_DOCUMENT);

      addHeading(doc, 1, "DBus interfaces");

      QList<QSharedPointer<DBusXmlInterface>> interfaces = info()->interfaces;
      std::sort(interfaces.begin(), interfaces.end(),
                [](auto a, auto b) { return a->name < b->name; });
      for (const auto& interf : interfaces) {
        addLink(doc, urlPrefix() + "/" + interf->name, interf->name);
        if (interf->doc &&
            interf->doc->paramByName().contains("short_description"))
          addTextParagraph(
              doc, interf->doc->paramByName()["short_description"].trimmed());
      }

      return createQSharedPointer<HelpPage>(doc, "", "DBus interfaces");
    } else if (parts.size() == 1) {
      // qDebug() << suffix;
      if (!info()->interfacesByName.contains(suffix)) {
        qWarning() << "Could not find DBus interface" << suffix;
        return QSharedPointer<HelpPage>();
      }
      auto interf = info()->interfacesByName[suffix];

      // Generate markdown document
      auto doc = vx::cmark::Node::newNode(CMARK_NODE_DOCUMENT);

      QString title = "DBus interface " + interf->name;
      addHeading(doc, 1, title);

      addDocShort(doc, interf);

      addDoc(doc, interf);

      addAnnotations(doc, interf);

      if (interf->methods.size() != 0) {
        addHeading(doc, 1, "Methods");

        for (const auto& method : interf->methods) {
          addHeading(doc, 2, method->name);

          addDocShort(doc, method);

          addDoc(doc, method);

          addAnnotations(doc, method);

          addHeading(doc, 3, "Arguments");
          int i = 0;
          for (const auto& par : method->arguments) {
            i++;
            addHeading(
                doc, 4,
                par->name != "" ? par->name : "Argument " + QString::number(i));
            addTextParagraph(doc, "Direction: " + par->direction);
            addTextParagraph(doc, "Signature: " + par->type);
            if (par->name != "" && method->doc &&
                method->doc->paramByName().contains(par->name))
              addTextParagraph(doc, "Documentation: " +
                                        method->doc->paramByName()[par->name]);
            addAnnotations(doc, par);
          }
        }
      }

      if (interf->properties.size() != 0) {
        addHeading(doc, 1, "Properties");

        for (const auto& property : interf->properties) {
          addHeading(doc, 2, property->name);

          addDocShort(doc, property);

          addDoc(doc, property);

          addAnnotations(doc, property);

          addTextParagraph(doc, "Signature: " + property->type);

          addTextParagraph(doc, "Access: " + property->access);
        }
      }

      if (interf->signals_.size() != 0) {
        addHeading(doc, 1, "Signals");

        for (const auto& signal : interf->signals_) {
          addHeading(doc, 2, signal->name);

          addDocShort(doc, signal);

          addDoc(doc, signal);

          addAnnotations(doc, signal);

          addHeading(doc, 3, "Arguments");
          int i = 0;
          for (const auto& par : signal->arguments) {
            i++;
            addHeading(
                doc, 4,
                par->name != "" ? par->name : "Argument " + QString::number(i));
            addTextParagraph(doc, "Direction: " + par->direction);
            addTextParagraph(doc, "Signature: " + par->type);
            if (par->name != "" && signal->doc &&
                signal->doc->paramByName().contains(par->name))
              addTextParagraph(doc, "Documentation: " +
                                        signal->doc->paramByName()[par->name]);
            addAnnotations(doc, par);
          }
        }
      }

      return createQSharedPointer<HelpPage>(doc, "", title);
    } else {
      qWarning() << "Unable to handle dbus-interface URL" << suffix;
      return QSharedPointer<HelpPage>();
    }
  }
};
}  // namespace help
}  // namespace vx

QSharedPointer<HelpPageSource> vx::help::getHelpPageSourceDBusInterface() {
  static QSharedPointer<HelpPageSource> source =
      createQSharedPointer<HelpPageSourceDBusInterface>();
  return source;
}
