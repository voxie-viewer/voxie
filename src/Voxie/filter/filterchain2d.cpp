#include "filterchain2d.hpp"

#include <Voxie/ivoxie.hpp>

#include <Voxie/plugin/metafilter2d.hpp>
#include <Voxie/plugin/voxieplugin.hpp>

#include <QtCore/QThreadPool>
#include <QtCore/QtMath>

#include <QtWidgets/QMessageBox>

using namespace voxie::filter;
using namespace voxie::data;
using namespace voxie::plugin;

FilterChain2D::~FilterChain2D() {
}

void FilterChain2D::applyTo(FloatImage slice)
{
    Filter2D* activeFilter;

    for(int x=0; x < this->filters.length(); x++)
    {
        activeFilter = this->filters.at(x);
        if(activeFilter->isEnabled())
        {
            slice = activeFilter->applyTo(slice);
        }
    }
    this->outputFloatImage = slice;
    emit this->allFiltersApplied(); // signals that chain has finished work so that application can get output
}

void FilterChain2D::applyTo(SliceImage slice)
{
    Filter2D* activeFilter;

    for(int x=0; x < this->filters.length(); x++)
    {

        activeFilter = this->filters.at(x);
        if(activeFilter->isEnabled())
        {
            slice = activeFilter->applyTo(slice);
        }
    }
    this->outputSliceImage = slice;
    emit this->allFiltersApplied(); // signals that chain has finished work so that application can get output
}

void FilterChain2D::addFilter(Filter2D* filter)
{
    this->filters.append(filter);
    connect(filter, &Filter2D::filterChanged, this, &FilterChain2D::filterChanged);
    emit this->filterListChanged();
}

void FilterChain2D::removeFilter(Filter2D* filter)
{
    //get filter position
    int filterPos = this->filters.indexOf(filter,0);

    //check if filter found
    if(filterPos == -1)
    {
        //no matching
    }
    disconnect(filter, &Filter2D::filterChanged, this, &FilterChain2D::filterChanged);
    this->filters.remove(filterPos);
    emit this->filterListChanged();
}

void FilterChain2D::changePosition(Filter2D* filter, int pos)
{
    //get filter position for deleting
    int filterPos = this->filters.indexOf(filter,0);
    if(filterPos == -1) //check if filter found
    {
        //no matching
    }else
    {
        this->filters.remove(filterPos);
        this->filters.insert(pos, filter);
        emit this->filterListChanged();
    }

}

QVector<Filter2D*> FilterChain2D::getFilters()
{
    return this->filters;
}

Filter2D* FilterChain2D::getFilter(int pos)
{
    return this->filters.at(pos);
}

FloatImage &FilterChain2D::getOutputImage()
{
    return this->outputFloatImage;
}

SliceImage& FilterChain2D::getOutputSlice()
{
    return this->outputSliceImage;
}

void FilterChain2D::onPlaneChanged(const voxie::data::Plane &oldPlane, const voxie::data::Plane &newPlane, bool equivalent)
{
    //qDebug() << (oldPlane.normal() - newPlane.normal()).lengthSquared();
    //qDebug() << "adjust masks!" << equivalent;
    if(equivalent){
        if(oldPlane.origin != newPlane.origin){
            // translation
            QPointF difference = newPlane.get2DPlanePoint(oldPlane.origin);
            //qDebug() << difference;
            for(Filter2D* filter: this->filters){
                filter->getMask()->translateOrigin(difference.x(), difference.y());
            }
        }
        //qDebug() << oldPlane.tangent() << newPlane.tangent() << (oldPlane.tangent() != newPlane.tangent());
        if((oldPlane.normal() - newPlane.normal()).lengthSquared() < 1e-5 && oldPlane.tangent() != newPlane.tangent()){
            // rotation
            qreal cos = QVector3D::dotProduct(oldPlane.tangent(), newPlane.tangent());
            qreal angle = qRadiansToDegrees(qAcos(cos));
            if(std::isnan(angle)){
                return;
            }
            QPointF oldXAxis = newPlane.get2DPlanePoint(newPlane.origin + oldPlane.tangent());
            angle = angle * (oldXAxis.y() > 0 ? 1:-1); // clockwise or counterclockwise
            //qDebug() << "rotation adjust" << cos << angle << oldXAxis;
            for(Filter2D* filter: this->filters){
                filter->getMask()->rotate(angle);
            }
        }
    } else {
        // lets not clear, this would be just a pain in the ass
//        for(Filter2D* filter: this->filters){
//            filter->getMask()->clearMask();
//        }
    }

}

void FilterChain2D::toXML(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    QXmlStreamWriter xml;
    xml.setDevice(&file);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE filterchain>");
    xml.writeStartElement("filterchain2d");
    xml.writeAttribute("version", "1.0");

    for(int i=0; i<this->getFilters().size(); i++) {
        xml.writeStartElement(this->getFilter(i)->getMetaName());
        xml.writeAttribute("type", "filter2d");
        xml.writeAttributes(this->getFilter(i)->exportFilterSettingsXML());
        xml.writeEndElement();
    }

    xml.writeEndDocument();
    file.flush();
    file.close();
}

void FilterChain2D::fromXML(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QXmlStreamReader xml;
    xml.setDevice(&file);

    if (xml.readNextStartElement()) {
        if (!(xml.name() == "filterchain2d" && xml.attributes().value("version") == "1.0")) {
            xml.raiseError(QObject::tr("The file is no valid filterchain."));
            QMessageBox messageBox;
            messageBox.critical(0,"Error","Invalid filterchain file.");
            messageBox.setFixedSize(500,200);
            return;
        }
    }

    this->filters.clear();
    while (xml.readNextStartElement()) {
        if (xml.attributes().value("type").compare(QString("filter2d"))==0) {
            for(VoxiePlugin* plugin : ::voxie::voxieRoot().plugins()) {
                for(MetaFilter2D *metaFilter : plugin->filters2D()) {
                    if (metaFilter->objectName().compare(xml.name())==0) {
                        Filter2D* filter = metaFilter->createFilter();
                        this->addFilter(filter);
                        filter->importFilterSettingsXML(xml.attributes());
                    }
                }
            }
        }
        xml.skipCurrentElement();
    }
    emit filterListChanged();
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
