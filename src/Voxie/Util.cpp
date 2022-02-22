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

#include "Util.hpp"

// https://developer.apple.com/library/archive/documentation/AppleScript/Conceptual/AppleScriptLangGuide/reference/ASLR_classes.html#//apple_ref/doc/uid/TP40000983-CH1g-BBCIAHJF
// http://web.archive.org/web/20200728075846/https://developer.apple.com/library/archive/documentation/AppleScript/Conceptual/AppleScriptLangGuide/reference/ASLR_classes.html#//apple_ref/doc/uid/TP40000983-CH1g-BBCIAHJF
QString vx::appleScriptEscape(const QString& arg) {
  QString res = arg;
  res.replace('\\', "\\\\");
  res.replace('\"', "\\\"");
  return "\"" + res + "\"";
}

QString vx::escapeArgument(const QString& arg) {
  QString res = arg;
  res.replace('\'', "'\\''");
  return "'" + res + "'";
}
QString vx::escapeArguments(const QList<QString>& args) {
  QString res;

  for (int i = 0; i < args.size(); i++) {
    if (i != 0) res += " ";
    // TODO: Escape differently for windows?
    QString arg = args[i];
    arg.replace('\'', "'\\''");
    res += "'";
    res += arg;
    res += "'";
  }

  return res;
}

// https://docs.python.org/3/reference/lexical_analysis.html#string-and-bytes-literals
QString vx::escapePythonString(const QString& s) {
  QString ret = "'";
  for (QChar c : s) {
    if (c < 32 || c >= 127 || c == '\\' || c == '\'') {
      if (c == '\\')
        ret += "\\\\";
      else if (c == '\'')
        ret += "\\\'";
      else if (c == '\a')
        ret += "\\a";
      else if (c == '\b')
        ret += "\\b";
      else if (c == '\f')
        ret += "\\f";
      else if (c == '\n')
        ret += "\\n";
      else if (c == '\r')
        ret += "\\r";
      else if (c == '\t')
        ret += "\\t";
      else if (c == '\v')
        ret += "\\v";
      else
        ret += QString("\\u%1").arg(c.unicode(), 4, 16, (QChar)'0');
    } else {
      ret += c;
    }
  }
  ret += "'";
  return ret;
}
QString vx::escapePythonStringArray(const QList<QString>& l) {
  QString ret = "[";
  for (int i = 0; i < l.size(); i++) {
    if (i != 0) ret += ", ";
    ret += escapePythonString(l[i]);
  }
  ret += "]";
  return ret;
}

QString vx::escapePython(const QMap<QString, QList<QString>>& m) {
  QString ret = "{";
  bool first = true;
  for (const auto& key : m.keys()) {
    if (!first) ret += ", ";
    ret += escapePythonString(key) + ": " + escapePythonStringArray(m[key]);
    first = false;
  }
  ret += "}";
  return ret;
}
