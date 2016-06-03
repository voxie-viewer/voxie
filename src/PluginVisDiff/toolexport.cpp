#include "toolexport.hpp"

#include <QtCore/QDir>

#include <QtGui/QPainter>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>

ToolExport::ToolExport(QWidget *parent, DiffVisualizer* sv) :
    Visualizer2DTool(parent),
    sv(sv)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(2);
    saveButton = new QPushButton(getIcon(), getName());
    connect(saveButton, &QPushButton::clicked, [=]() {
        saveDialog();
    });
    layout->addWidget(saveButton,0,0);
    saveButton->show();
    this->setLayout(layout);
}

QString ToolExport::getName() {
    return "&Export..";
}

void ToolExport::activateTool() {
    saveDialog();
}

void ToolExport::saveDialog()
{
    QString fileName;


    QString selectedFilter;

    fileName = QFileDialog::getSaveFileName(this,tr("Save File"),QDir::homePath(),tr("Image as JPEG (*.jpg *.jpeg);;Image as PNG (*.png);;Colorized image as JPEG (*.jpg *.jpeg);;Colorized image as PNG (*.png);;Canvas as JPEG (*.jpg *.jpeg);;Canvas as PNG (*.png)"),&selectedFilter);
    if( !fileName.isNull() ) {
        //qDebug() << selectedFilter;
        QImage im;
        if(selectedFilter.startsWith("Canvas")) {
            im = QImage(sv->canvasWidth(), sv->canvasHeight(), QImage::Format_ARGB32_Premultiplied);
            QPainter painter(&im);
            QMap<int, QImage> map = sv->drawStack();
            int i;
            QImage nullImage;
            for (i = -1; i < sv->tools().size(); ++i) {
                QImage im = map.value(i, nullImage);

                if(im.width() > 0) {
                    // qDebug() << "drawing stack at " << i << " of range -1 to " << sv->tools.size()-1;
                    painter.drawImage(0, 0, im);
                }/* else {
                    qDebug() << "stack at " << i << " is null";
                }*/
            }
        } else if(selectedFilter.startsWith("Colorized")) {
            if(sv->drawStack().contains(-1)) {
                im = sv->drawStack().value(-1);
            }
        } else {
            if(sv->filteredSliceImage().getWidth() > 0) {
                im = sv->filteredSliceImage().toQImage();
            }
        }
        //  if( !fileName.isEmpty() ){
        //QString fileName = d.selectedFiles()[0];
        //QFile fil(fileName + ??????????????? ;
        //qDebug() << "filename: " << fileName;
        //qDebug() << "selectedFilter: " << d.selectedNameFilter();
        //qDebug() << fileName+d.selectedNameFilter();
        im.save(fileName);
    }
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
