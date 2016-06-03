#include "shape3d.hpp"

#include <Voxie/visualization/qveclineedit.hpp>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

using namespace voxie::filter;
using namespace voxie::visualization;

static QVector<Shape3D*> initShapeInstances();

// static init
static QObject shapeContainer;
QVector<Shape3D*> Shape3D::shapeInstances = initShapeInstances();

static QVector<Shape3D*> initShapeInstances() {
    QVector<Shape3D*> shapes;
    shapes.append(new Cuboid());
    shapes.append(new Tetrahedron());
    shapes.append(new Sphere());

    for (const auto& shape : shapes)
        shape->setParent(&shapeContainer);

    return shapes;
}

QWidget*
Cuboid::editorWidget()
{
    QWidget* widget = new QWidget();
    QVecLineEdit* origEdit = new QVecLineEdit(); origEdit->setVector(this->origin);
    QVecLineEdit* sizeEdit = new QVecLineEdit(); sizeEdit->setVector(this->dimension);
    QHBoxLayout* origEditLayout = new QHBoxLayout();
    QHBoxLayout* sizeEditLayout = new QHBoxLayout();
    origEditLayout->addWidget(new QLabel("Point"));
    origEditLayout->addWidget(origEdit);
    sizeEditLayout->addWidget(new QLabel("Size"));
    sizeEditLayout->addWidget(sizeEdit);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(origEditLayout);
    layout->addLayout(sizeEditLayout);
    widget->setLayout(layout);
    // --
    connect(origEdit, &QVecLineEdit::editingFinished, [=](){
        bool valid = false;
        QVector3D vec = origEdit->getVector(&valid);
        if(valid){
            this->setOrigin(vec);
        } else {
            origEdit->setVector(this->origin);
        }
    });
    connect(sizeEdit, &QVecLineEdit::editingFinished, [=](){
        bool valid = false;
        QVector3D vec = sizeEdit->getVector(&valid);
        if(valid){
            this->setDimension(vec);
        } else {
            sizeEdit->setVector(this->origin);
        }
    });
    // --
    return widget;
}

QString
Cuboid::toString()
{
    QString toReturn = name() + ": ";
    toReturn += "(" + QVecLineEdit::toString(this->origin)+ ")" + "(" + QVecLineEdit::toString(this->dimension) + ")";
    return toReturn;
}

QWidget*
Tetrahedron::editorWidget()
{
    QWidget* widget = new QWidget();
    QVecLineEdit* edit0 = new QVecLineEdit(); edit0->setVector(this->p0);
    QVecLineEdit* edit1 = new QVecLineEdit(); edit1->setVector(this->p1);
    QVecLineEdit* edit2 = new QVecLineEdit(); edit2->setVector(this->p2);
    QVecLineEdit* edit3 = new QVecLineEdit(); edit3->setVector(this->p3);
    QHBoxLayout* edit0Layout = new QHBoxLayout();
    QHBoxLayout* edit1Layout = new QHBoxLayout();
    QHBoxLayout* edit2Layout = new QHBoxLayout();
    QHBoxLayout* edit3Layout = new QHBoxLayout();
    edit0Layout->addWidget(new QLabel("Vertex 1"));
    edit1Layout->addWidget(new QLabel("Vertex 2"));
    edit2Layout->addWidget(new QLabel("Vertex 3"));
    edit3Layout->addWidget(new QLabel("Vertex 4"));
    edit0Layout->addWidget(edit0);
    edit1Layout->addWidget(edit1);
    edit2Layout->addWidget(edit2);
    edit3Layout->addWidget(edit3);
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(edit0Layout);
    layout->addLayout(edit1Layout);
    layout->addLayout(edit2Layout);
    layout->addLayout(edit3Layout);
    widget->setLayout(layout);
    //--
    connect(edit0, &QVecLineEdit::editingFinished, [=](){
        bool valid = false;
        QVector3D vec = edit0->getVector(&valid);
        if(valid){
            this->setVertex(0,vec);
        } else {
            edit0->setVector(this->p0);
        }
    });
    connect(edit1, &QVecLineEdit::editingFinished, [=](){
        bool valid = false;
        QVector3D vec = edit1->getVector(&valid);
        if(valid){
            this->setVertex(1,vec);
        } else {
            edit1->setVector(this->p1);
        }
    });
    connect(edit2, &QVecLineEdit::editingFinished, [=](){
        bool valid = false;
        QVector3D vec = edit2->getVector(&valid);
        if(valid){
            this->setVertex(2,vec);
        } else {
            edit2->setVector(this->p2);
        }
    });
    connect(edit3, &QVecLineEdit::editingFinished, [=](){
        bool valid = false;
        QVector3D vec = edit3->getVector(&valid);
        if(valid){
            this->setVertex(3,vec);
        } else {
            edit3->setVector(this->p3);
        }
    });
    //--
    return widget;
}

QString
Tetrahedron::toString()
{
    QString toReturn = name() + ": ";
    for(QVector3D v: this->getVertices())
        toReturn += "(" + QVecLineEdit::toString(v)+ ")";
    return toReturn;
}


QWidget*
Sphere::editorWidget()
{
    QWidget* widget = new QWidget();
    QVecLineEdit* origEdit = new QVecLineEdit(); origEdit->setVector(this->origin);
    QLineEdit* radiusEdit = new QLineEdit(); radiusEdit->setText(QString::number(this->radius));
    QHBoxLayout* origEditLayout = new QHBoxLayout();
    QHBoxLayout* radiusEditLayout = new QHBoxLayout();
    origEditLayout->addWidget(new QLabel("Point"));
    origEditLayout->addWidget(origEdit);
    radiusEditLayout->addWidget(new QLabel("Radius"));
    radiusEditLayout->addWidget(radiusEdit);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(origEditLayout);
    layout->addLayout(radiusEditLayout);
    widget->setLayout(layout);
    // --
    connect(origEdit, &QVecLineEdit::editingFinished, [=](){
        bool valid = false;
        QVector3D vec = origEdit->getVector(&valid);
        if(valid){
            this->setOrigin(vec);
        } else {
            origEdit->setVector(this->origin);
        }
    });
    connect(radiusEdit, &QLineEdit::editingFinished, [=](){
        bool valid = false;
        qreal rad = radiusEdit->text().toDouble(&valid);
        if(valid){
            this->setRadius(rad);
        } else {
            radiusEdit->setText(QString::number(this->radius));
        }
    });
    // --
    return widget;
}


QString
Sphere::toString()
{
    QString toReturn = name() + ": ";
    toReturn += "(" + QVecLineEdit::toString(this->origin)+ ")" + "(r=" + QString::number(this->radius) + ")";
    return toReturn;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
