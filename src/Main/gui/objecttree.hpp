#pragma once

#include <QtCore/QPointer>

#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeWidget>

namespace voxie {
class Root;
namespace data {
class DataObject;
}
}

namespace voxie { namespace gui {

class ObjectTree : public QTreeWidget {
    Q_OBJECT

    QMap<voxie::data::DataObject*, QList<QTreeWidgetItem*>> map;

    bool suppressSelectionChanged;
    QPointer<voxie::data::DataObject> oldSelectedObject;
    bool haveOldSelectedObject = false;

    void addObject(voxie::data::DataObject* obj);
    void setupItem(voxie::data::DataObject* obj, QTreeWidgetItem* item);
    void cleanupItem(QTreeWidgetItem* item);
    void emitObjectSelected(voxie::data::DataObject* obj);

public:
    explicit ObjectTree(Root* root, QWidget* parent = nullptr);
    ~ObjectTree() override;

    voxie::data::DataObject* selectedObject();

    void select(voxie::data::DataObject* obj);

signals:
    void objectSelected(voxie::data::DataObject* obj);
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
