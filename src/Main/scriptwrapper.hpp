#pragma once

#include <Voxie/scripting/scriptingcontainer.hpp>

#include <QtCore/QMutex>

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

#include <functional>

class ScriptWrapper : public QObject {
    Q_OBJECT

    QScriptEngine* engine;

    QScriptValue toScriptValue(const QVariant& value);
    void addPropertyWrapper(const QScriptValue& obj, const QMetaObject* type, const QMetaProperty& property, QString& propertyDesc, const QString& objectChildId = "", const QMetaObject* parentType = nullptr);
    void addMethodWrapper(const QScriptValue& obj, const QMetaObject* type, const QMetaMethod& method, QString& methodDesc, const QString& objectChildId = "", const QMetaObject* parentType = nullptr);

    QMutex prototypeMutex;
    QMap<const QMetaObject*, QScriptValue> prototypes;
    QMutex objectMutex;
    QMap<const QObject*, QScriptValue> objects;

public:
    ScriptWrapper(QScriptEngine* engine);
    virtual ~ScriptWrapper();

    QScriptValue getWrapperPrototype(voxie::scripting::ScriptingContainerBase* obj);
    QScriptValue getWrapper(voxie::scripting::ScriptingContainerBase* obj);

    static void addScriptFunction (QScriptValue obj, const QString& name, const QScriptValue::PropertyFlags& flags, const std::function<QScriptValue(QScriptContext*, QScriptEngine*)>& fun);
    static QVariant fromScriptValue(const QScriptValue& value);
};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
