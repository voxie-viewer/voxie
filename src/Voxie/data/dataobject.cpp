#include "dataobject.hpp"

#include <QtGui/QIcon>

#include <QtWidgets/QWidget>

using namespace voxie::data;
using namespace voxie::data::internal;
using namespace voxie::scripting;

DataObject::DataObject(const QString& type, QObject *parent) : ScriptableObject(type, parent) {
    new DataObjectAdaptor(this);
}
DataObject::~DataObject() {
}

QIcon DataObject::icon() const {
    return QIcon();
}

void DataObject::addChildObject(DataObject* obj) {
    connect(obj, &QObject::destroyed, this, [this, obj] () {
            childObjects_.removeOne(obj);
        });
    childObjects_ << obj;
    connect(this, &QObject::destroyed, obj, [this, obj] () {
            obj->parentObjects_.removeOne(this);
        });
    obj->parentObjects_ << this;
}

void DataObject::addPropertySection(QWidget* section) {
    connect(section, &QObject::destroyed, this, [this, section] () {
            propertySections_.removeOne(section);
        });
    propertySections_ << section;
    connect(this, &QObject::destroyed, section, &QObject::deleteLater);
    emit propertySectionAdded(section);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
