#pragma once

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMainWindow>

namespace voxie {
class Root;
namespace data {
class DataObject;
}
namespace io {
class Operation;
}
namespace gui {
class ObjectTree;

class SidePanel : public QWidget {
    Q_OBJECT

    QVBoxLayout* sections;

    QLayout* bottomLayout;

    ObjectTree* objectTree;
    QList<QPointer<QWidget>> visibleSections;

    void addDataObject(voxie::data::DataObject* obj);

public:
    explicit SidePanel(Root* root, QMainWindow* mainWindow, QWidget *parent = 0);
    ~SidePanel();

    /**
     * @brief Adds a section to the window.
     * @param widget
     * @param closeable If true, the user can close the section.
     */
    QWidget* addSection(QWidget *widget, bool closeable = false, voxie::data::DataObject* obj = nullptr);

    void addProgressBar(voxie::io::Operation* operation);
};

} }

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
