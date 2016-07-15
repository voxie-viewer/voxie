#include "objecttree.hpp"

#include <Main/root.hpp>

#include <QtCore/QDebug>

using namespace voxie::gui;

ObjectTree::ObjectTree(voxie::Root* root, QWidget* parent) : QTreeWidget(parent) {
    setMinimumHeight(300);
    setSelectionMode(QAbstractItemView::SingleSelection);
    //setSelectionMode(QAbstractItemView::MultiSelection);
    setHeaderLabel("Data Objects");

    /*
    // Wait until Root::instance() exists
    {
        QObject temp;
        connect(&temp, &QObject::destroyed, this, [this] {
    */
    //qDebug() << "init";
    connect(root, &Root::dataObjectAdded, this, &ObjectTree::addObject);
    for (auto obj : root->dataObjects())
        addObject(obj);
    /*
            }, Qt::QueuedConnection);
    }
    */

    // Highlight all items representing the same object
	connect(this, &QTreeWidget::itemSelectionChanged, this, [this] () {
            if (suppressSelectionChanged) {
                //qDebug() << "suppressSelectionChanged";
                return;
            }
            auto citem = currentItem();
            auto obj = selectedObject();
            if (!obj)
                return;
            //qDebug() << "Select" << obj;
            suppressSelectionChanged = true;
            for (auto item : map[obj])
                setCurrentItem(item, 0, QItemSelectionModel::Select);
            if (citem)
                setCurrentItem(citem, 0, QItemSelectionModel::Current);
            else if (map[obj].size() > 0)
                setCurrentItem(map[obj][0], 0, QItemSelectionModel::Current);
            suppressSelectionChanged = false;
        });

	connect(this, &QTreeWidget::itemSelectionChanged, this, [this] () {
            if (suppressSelectionChanged) {
                //qDebug() << "suppressSelectionChanged";
                return;
            }
            auto obj = selectedObject();
            emitObjectSelected(obj);
        });
}
ObjectTree::~ObjectTree() {
}

void ObjectTree::emitObjectSelected(voxie::data::DataObject* obj) {
    if (!obj && !haveOldSelectedObject)
        return;
    if (obj && haveOldSelectedObject && obj == oldSelectedObject)
        return;

    oldSelectedObject = obj;
    haveOldSelectedObject = obj != nullptr;
    emit objectSelected(obj);
}

void ObjectTree::addObject(voxie::data::DataObject* obj) {
    //qDebug() << "addObject" << obj;

    if (map.contains(obj) && map[obj].size() != 0) {
        //qCritical() << "Object" << obj << "already in ObjectTree map";
        return;
    }

    connect(obj, &QObject::destroyed, this, [this, obj] {
            QList<QTreeWidgetItem*> items = map[obj];
            for (auto item : items)
                cleanupItem(item);
            if (map[obj].size() != 0)
                qCritical() << "cleanupItems(items) failed for" << obj;
            map.remove(obj);
        });

    const auto& parents = obj->parentObjects();
    if (parents.size() == 0) {
        auto item = new QTreeWidgetItem(this);
        setupItem(obj, item);
        map[obj] << item;
    } else {
        for (auto parent : parents) {
            if (!map.contains(parent))
                addObject(parent);
            if (!map.contains(parent)) {
                qCritical() << "Could not get items for parent" << parent;
                continue;
            }
            const auto& parentItems = map[parent];

            for (auto parentItem : parentItems) {
                auto item = new QTreeWidgetItem(parentItem);
                setupItem(obj, item);
                map[obj] << item;

                // expand all parents up to root
                auto toExpand = parentItem;
                while (toExpand) {
                    expandItem(toExpand);
                    toExpand = toExpand->parent();
                }
            }
        }
    }

    select(obj);
}

void ObjectTree::setupItem(voxie::data::DataObject* obj, QTreeWidgetItem* item) {
    item->setText(0, obj->displayName());
    item->setIcon(0, obj->icon());
    item->setData(0, Qt::UserRole, QVariant::fromValue(obj));
}

void ObjectTree::cleanupItem(QTreeWidgetItem* item) {
    // Because this will be executed from obj's QObject::~Object() a dynamic
    // cast to voxie::data::DataObject* will fail.
    //auto obj = item->data(0, Qt::UserRole).value<voxie::data::DataObject*>();
    auto obj = (voxie::data::DataObject*) item->data(0, Qt::UserRole).value<QObject*>();
    if (!obj) {
        qCritical() << "Could not find object for ObjectTree item";
        return;
    }
    map[obj].removeOne(item);

    QList<QTreeWidgetItem*> children;
    for (int i = 0; i < item->childCount(); i++)
        children << item->child(i);
    for (auto child : children)
        cleanupItem(child);

    delete item;
}

voxie::data::DataObject* ObjectTree::selectedObject() {
    auto citem = currentItem();
    if (!citem)
        return nullptr;
    auto obj = citem->data(0, Qt::UserRole).value<voxie::data::DataObject*>();
    if (!obj)
        return nullptr;
    return obj;
}

void ObjectTree::select(voxie::data::DataObject* obj) {
    if (selectedObject() == obj)
        return;

    const auto& items = map[obj];
    if (items.size() == 0) {
        qWarning() << "Could not find object" << obj;
        return;
    }

    suppressSelectionChanged = true;
    auto currentSelection = selectedItems();
    for (auto item : selectedItems())
        setCurrentItem(item, 0, QItemSelectionModel::Deselect);
    for (auto item : items)
        setCurrentItem(item, 0, QItemSelectionModel::Select);
    setCurrentItem(items[0], 0, QItemSelectionModel::Current);
    suppressSelectionChanged = false;

    emitObjectSelected(obj);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
