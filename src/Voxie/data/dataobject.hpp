#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <Voxie/scripting/scriptingcontainer.hpp>

#include <QtDBus/QDBusAbstractAdaptor>

namespace voxie { namespace data {

class VOXIECORESHARED_EXPORT DataObject : public voxie::scripting::ScriptableObject {
    Q_OBJECT

    QList<DataObject*> parentObjects_;
    QList<DataObject*> childObjects_;

    QString displayName_;

    QList<QWidget*> propertySections_;

 public:
	explicit DataObject(const QString& type, QObject *parent = 0);
    ~DataObject() override;

    const QList<DataObject*>& parentObjects() const { return parentObjects_; }
    const QList<DataObject*>& childObjects() const { return childObjects_; }
    void addChildObject(DataObject* obj);

    Q_PROPERTY (QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    const QString& displayName() const { return displayName_; }
    void setDisplayName(const QString& name) { displayName_ = name; emit displayNameChanged(name); }

    virtual QIcon icon() const;

    QList<QWidget*> propertySections() const { return propertySections_; }
    void addPropertySection(QWidget* section);

 signals:
    void displayNameChanged(const QString& newName);
    void propertySectionAdded(QWidget* section);
};

namespace internal {
class DataObjectAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.DataObject")

    DataObject* object;

public:
    DataObjectAdaptor(DataObject* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~DataObjectAdaptor() {}

    Q_PROPERTY (QString DisplayName READ displayName)
    QString displayName() { return object->displayName(); }
};
}

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
