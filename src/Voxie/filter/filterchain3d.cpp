#include "filterchain3d.hpp"

#include <Voxie/ivoxie.hpp>

#include <Voxie/plugin/metafilter3d.hpp>
#include <Voxie/plugin/voxieplugin.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <QtWidgets/QMessageBox>

using namespace voxie::data;
using namespace voxie::filter;
using namespace voxie::plugin;


FilterChain3D::FilterChain3D(QObject *parent):
	QObject(parent)
{
    this->filters = QVector<Filter3D*>();
}

FilterChain3D::~FilterChain3D()
{
  this->filters.clear();
}


void FilterChain3D::applyTo(voxie::data::DataSet* dataSet)
{
	Filter3D* activeFilter;
    try {
        dataSet->resetData();
    } catch (voxie::scripting::ScriptingException& e) {
        qCritical() << "Error while applying filters:" << e.message();
        QMessageBox(QMessageBox::Critical, "Voxie: Error while applying filters", QString("Error while applying filters: %1").arg(e.message()), QMessageBox::Ok, voxieRoot().mainWindow()).exec();
        return;
    }
    QSharedPointer<voxie::data::VoxelData> volume = dataSet->filteredData();
    for(int x=0; x < this->filters.length(); x++)
	{
        activeFilter = this->filters.at(x);
		if(activeFilter->isEnabled())
		{
			volume = activeFilter->applyTo(volume);
		}
	}
	this->outputVolume = volume;
    emit volume->changed();
	emit this->allFiltersApplied(); // signals that chain has finished work so that application can get output
}

void FilterChain3D::addFilter(Filter3D* filter)
{
    if(filter != nullptr){
        this->filters.append(filter);
        connect(filter, &Filter3D::filterChanged, this, & FilterChain3D::onFilterChanged);
        if(this->signalOnChangeEnabled())
            emit this->filterListChanged();
    }
}

void FilterChain3D::removeFilter(Filter3D* filter)
{
	//get filter position
    int filterPos = this->filters.indexOf(filter,0);

	//check if filter found
	if(filterPos == -1)
	{
		//no matching
	}

    this->filters.remove(filterPos);
    disconnect(filter, &Filter3D::filterChanged, this, & FilterChain3D::onFilterChanged);
    if(this->signalOnChangeEnabled())
        emit this->filterListChanged();
}

void FilterChain3D::changePosition(Filter3D* filter, int pos)
{
	//get filter position for deleting
    int filterPos = this->filters.indexOf(filter,0);
	if(filterPos == -1) //check if filter found
	{
		//no matching
	}
    this->filters.remove(filterPos);

    this->filters.insert(pos, filter);
    if(this->signalOnChangeEnabled())
        emit this->filterListChanged();
}

QVector<Filter3D*> FilterChain3D::getFilters()
{
	return this->filters;
}

Filter3D* FilterChain3D::getFilter(int pos)
{
    return this->filters.at(pos);
}

QSharedPointer<VoxelData> FilterChain3D::getOutputVolume()
{
	return this->outputVolume;
}

void FilterChain3D::toXML(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::WriteOnly);
    QXmlStreamWriter xml;
    xml.setDevice(&file);
    xml.setAutoFormatting(true);

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE filterchain>");
    xml.writeStartElement("filterchain3d");
    xml.writeAttribute("version", "1.0");

    for(int i=0; i<this->getFilters().size(); i++) {
        xml.writeStartElement(this->getFilter(i)->getMetaName());
        xml.writeAttribute("type", "filter3d");
        xml.writeAttributes(this->getFilter(i)->exportFilterSettingsXML());
        xml.writeEndElement();
    }

    xml.writeEndDocument();
    file.flush();
    file.close();
}

void FilterChain3D::fromXML(QString fileName)
{
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QXmlStreamReader xml;
    xml.setDevice(&file);

    if (xml.readNextStartElement()) {
        if (!(xml.name() == "filterchain3d" && xml.attributes().value("version") == "1.0")) {
            xml.raiseError(QObject::tr("The file is no valid filterchain."));
            QMessageBox messageBox;
            messageBox.critical(0,"Error","Invalid filterchain file.");
            messageBox.setFixedSize(500,200);
            return;
        }
    }

    this->filters.clear();
    while (xml.readNextStartElement()) {
        if (xml.attributes().value("type").compare("filter3d")==0) {
            for(VoxiePlugin* plugin : ::voxie::voxieRoot().plugins()) {
                for(MetaFilter3D *metaFilter : plugin->filters3D()) {
                    if (metaFilter->objectName().compare(xml.name())==0) {
                        Filter3D* filter = metaFilter->createFilter();
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
