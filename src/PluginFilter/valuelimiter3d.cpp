#include "valuelimiter3d.hpp"

#include <QtGui/QValidator>

#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

ValueLimiter3D::ValueLimiter3D(QObject *parent) :
    Filter3D(parent),
    lowerLimit(0),
    upperLimit(100),
    settingsdialog(nullptr)
{
}

ValueLimiter3D::~ValueLimiter3D()
{
    if(this->settingsdialog){
        this->settingsdialog->deleteLater();
    }
}

voxie::data::VoxelData*
ValueLimiter3D::getSourceVolume(voxie::data::VoxelData *input)
{
    // filter works inplace
    return input;
}

void
ValueLimiter3D::applyTo(voxie::data::VoxelData* input, voxie::data::VoxelData* output)
{
    Q_UNUSED(input);
    for(size_t z = 0; z < output->getDimensions().z; z++){
    for(size_t y = 0; y < output->getDimensions().y; y++){
    for(size_t x = 0; x < output->getDimensions().x; x++){
        if(this->getMask()->contains(x,y,z, output->getSpacing(), output->getOrigin())){
            if(output->getVoxel(x,y,z) > this->upperLimit){
                output->setVoxel(x,y,z, this->upperLimit);
            } else if(output->getVoxel(x,y,z) < this->lowerLimit){
                output->setVoxel(x,y,z, this->lowerLimit);
            }
        }
    }
    }
    }
}

QXmlStreamAttributes
ValueLimiter3D::exportFilterSettingsXML()
{
    QXmlStreamAttributes attributes;
    attributes.append("lowerLimit", QString::number(this->lowerLimit));
    attributes.append("upperLimit", QString::number(this->upperLimit));
    return attributes;
}


void
ValueLimiter3D::importFilterSettingsXML(QXmlStreamAttributes attributes)
{
    bool valid;
    float lower = attributes.value("lowerLimit").toFloat(&valid);
    if(!valid)
        lower = this->lowerLimit;
    float upper = attributes.value("upperLimit").toFloat(&valid);
    if(!valid)
        upper = this->upperLimit;

    this->setLimits(upper,lower);
}

void
ValueLimiter3D::setLowerLimit(float value)
{
    if(value != this->lowerLimit){
        this->lowerLimit = value;
        triggerFilterChanged();
    }
}

void
ValueLimiter3D::setUpperLimit(float value)
{
    if(value != this->upperLimit){
        this->upperLimit = value;
        triggerFilterChanged();
    }
}

void
ValueLimiter3D::setLimits(float upper, float lower)
{
    bool changed = false;
    if(lower != this->lowerLimit){
        this->lowerLimit = lower;
        changed = true;
    }
    if(upper != this->upperLimit){
        this->upperLimit = upper;
        changed = true;
    }
    if(changed)
        triggerFilterChanged();
}


// ---

voxie::filter::Filter3D* MetaValueLimiter3D::createFilter() const
{
    ValueLimiter3D* filter = new ValueLimiter3D();
    //filter->setName("Value Limiter");
    filter->setMetaName(this->objectName());
    return filter;
}

// ---

class NumberValidator : public QValidator
{
public:
    NumberValidator(QObject * parent = 0) : QValidator(parent) {}
    QValidator::State validate(QString &s, int &i) const
    {
        Q_UNUSED(i);
        if (s.isEmpty() || s == "-") {
            return QValidator::Intermediate;
        }
        bool success = false;
        s.toDouble(&success);
        return success ? QValidator::Acceptable : QValidator::Invalid;
    }
};

QDialog*
ValueLimiter3D::getSettingsDialog()
{
    if(this->settingsdialog == nullptr){
        this->settingsdialog = new QDialog();
        //this->settingsdialog->setWindowTitle(this->getName());
        QPushButton* btnOk = new QPushButton("ok");
        QLineEdit* editUpper = new QLineEdit(QString::number(this->upperLimit));
        QLineEdit* editLower = new QLineEdit(QString::number(this->lowerLimit));
        editUpper->setValidator(new NumberValidator());
        editUpper->setValidator(new NumberValidator());

        {// connections
            connect(this, &Filter3D::filterChanged, this->settingsdialog, [=]()
            {
                editLower->setText(QString::number(this->getLowerLimit()));
                editUpper->setText(QString::number(this->getUpperLimit()));
            });
            connect(btnOk, &QPushButton::clicked, this, [=]()
            {
                bool valid;
                float lower = editLower->text().toFloat(&valid);
                if(!valid)
                    lower = this->getLowerLimit();

                float upper = editUpper->text().toFloat(&valid);
                if(!valid)
                    upper = this->getUpperLimit();

                this->setLimits(upper, lower);
            });

        }// end connections

        {// layout
            QHBoxLayout* upperRow = new QHBoxLayout();
            QHBoxLayout* lowerRow = new QHBoxLayout();
            upperRow->addWidget(new QLabel("upper limit:"));
            lowerRow->addWidget(new QLabel("lower limit:"));
            upperRow->addWidget(editUpper);
            lowerRow->addWidget(editLower);
            QVBoxLayout* layout = new QVBoxLayout();
            layout->addLayout(upperRow);
            layout->addLayout(lowerRow);
            layout->addWidget(btnOk);
            this->settingsdialog->setLayout(layout);
        }// end layout
    }
    return this->settingsdialog;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
