#include "toolselectionpolygon.hpp"

#include <PluginVisSlice/slicevisualizer.hpp>

#include <Voxie/filtermask/selection2dmask.hpp>

#include <QtWidgets/QGridLayout>

//ToolSelectionPolygon::ToolSelectionPolygon(QWidget *parent, SliceVisualizer *sv):
//    Visualizer2DTool(parent),
//    sv(sv)
//{
//    QGridLayout* layout = new QGridLayout(this);
//    selectionButton = new QPushButton(getIcon(), getName());
//    selectionButton->setCheckable(true);
//    connect(selectionButton, &QPushButton::clicked, [=]() {
//        this->sv->switchToolTo(this);
//    });
//    layout->addWidget(selectionButton,0,0);
//    selectionButton->show();
//    this->setLayout(layout);
//}

//void ToolSelectionPolygon::activateTool()
//{
//    selectionButton->setChecked(true);
//}

//void ToolSelectionPolygon::deactivateTool()
//{
//    selectionButton->setChecked(false);
//}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
