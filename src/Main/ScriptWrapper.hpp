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

#include <VoxieClient/ObjectExport/ExportedObject.hpp>

#include <QtCore/QMutex>

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

#include <functional>

class ScriptWrapper : public QObject {
  Q_OBJECT

  QScriptEngine* engine;

  QScriptValue toScriptValue(const QVariant& value);
  void addPropertyWrapper(const QScriptValue& obj, const QMetaObject* type,
                          const QMetaProperty& property, QString& propertyDesc,
                          const QString& objectChildId = "",
                          const QMetaObject* parentType = nullptr);
  void addMethodWrapper(const QScriptValue& obj, const QMetaObject* type,
                        const QMetaMethod& method, QString& methodDesc,
                        const QString& objectChildId = "",
                        const QMetaObject* parentType = nullptr);

  QMutex prototypeMutex;
  QMap<const QMetaObject*, QScriptValue> prototypes;
  QMutex objectMutex;
  QMap<const QObject*, QScriptValue> objects;

 public:
  ScriptWrapper(QScriptEngine* engine);
  virtual ~ScriptWrapper();

  QScriptValue getWrapperPrototype(vx::ExportedObject* obj);
  QScriptValue getWrapper(vx::ExportedObject* obj);

  static void addScriptFunction(
      QScriptValue obj, const QString& name,
      const QScriptValue::PropertyFlags& flags,
      const std::function<QScriptValue(QScriptContext*, QScriptEngine*)>& fun);
  static QVariant fromScriptValue(const QScriptValue& value);
};
