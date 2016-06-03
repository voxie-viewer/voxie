#include "colorizer.hpp"

#include <functional>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QXmlStreamWriter>

using namespace voxie::data;

int numColorizers = 0;

Colorizer::Colorizer(QObject *parent) :
	QObject(parent), nanColor(qRgba(0,0,0,0))
{
    this->setObjectName("Colorizer" + QString::number(numColorizers++));
}


void
Colorizer::putMapping(float in, QRgb out)
{
	bool isNew = !this->hasMapping(in);
	QRgb old = this->getMapping(in);

	colorMap.insert(in, out);

	if(isNew){
		emit this->mappingAdded(QPair<float,QRgb>(in,out));
	} else {
		emit this->mappingChanged(in, old, out);
	}
}

void
Colorizer::changeMapping(float in, QRgb newOut)
{
	if(this->colorMap.contains(in)){
		QRgb old = this->getMapping(in);
		this->colorMap.insert(in, newOut);
		emit this->mappingChanged(in, old, newOut);
	}
}

bool
Colorizer::removeMapping(float in)
{
	if(this->colorMap.contains(in)){
		QRgb old = getMapping(in);
		this->colorMap.remove(in);
		emit this->mappingRemoved(QPair<float,QRgb>(in,old));
		return true;
	}
	return false;
}

QRgb
Colorizer::getMapping(float in) const
{
	return this->colorMap.value(in, qRgba(0,0,0,0));
}


QRgb
Colorizer::getColor(float in) const
{
	if(std::isnan(in)){
		return nanColor;
	}

	if(this->colorMap.size() == 0){
		in = (in < 0 ? 0: in > 1 ? 1:in);
		int val = (int) (in*255);
		return qRgba(val,val,val,255);
	}

	auto inputs = colorMap.keys();
	auto length = inputs.length();

	int i = -1;
	while(i+1 < length && inputs[i+1] < in){
		i++;
	}

	if(i < 0){
		return this->colorMap.value(inputs[0]);
	} else if(i+1 == inputs.length()){
		return this->colorMap.value(inputs[i]);
	} else {
		// interpolate
		float in1 = inputs[i];
		float in2 = inputs[i+1];
		float rel = (in - in1)/(in2 - in1);

		QRgb c1 = this->colorMap.value(in1);
		QRgb c2 = this->colorMap.value(in2);

		int a = (int)(qAlpha(c1) + rel*(qAlpha(c2)-qAlpha(c1)));
		int r = (int)(qRed(c1) + rel*(qRed(c2)-qRed(c1)));
		int g = (int)(qGreen(c1) + rel*(qGreen(c2)-qGreen(c1)));
		int b = (int)(qBlue(c1) + rel*(qBlue(c2)-qBlue(c1)));
		return qRgba(r,g,b,a);
	}
}


QVector< QPair<float,QRgb> >
Colorizer::getMappings() const
{
	QVector< QPair<float,QRgb> > mappings;
	for (QMap<float,QRgb>::const_iterator i = colorMap.begin(); i != colorMap.end(); ++i) {
		mappings.append(QPair<float,QRgb>(i.key(), i.value()));
	}
	return mappings;
}


Colorizer*
Colorizer::clone() const
{
	Colorizer* colorizerClone = new Colorizer();
	colorizerClone->setNanColor(this->getNanColor());
	auto mappings = this->getMappings();
	for(const QPair<float,QRgb>& mapping: mappings ){
		colorizerClone->putMapping(mapping);
	}
	return colorizerClone;
}


void
Colorizer::toXML(QIODevice* ioDevice) const
{
	if(!ioDevice->isOpen()){
		if(!ioDevice->open(QIODevice::WriteOnly)){
			qDebug() << "cannot open ioDevice for xml writing";
			return;
		}
	} else {
		if(ioDevice->openMode() == QIODevice::ReadOnly){
			qDebug() << "cannot write to ioDevice for xml writing, is opened read only";
			return;
		} else {
			qDebug() << "ioDevice already open, writing xml may have undesired effect";
		}
	}

	QXmlStreamWriter xml(ioDevice);

	std::function<void(float, QRgb)> writeMapping = [&xml](float key, QRgb value)
		{   xml.writeStartElement("key");
			xml.writeCharacters(QString::number(key,'f',8));
			xml.writeEndElement();
			xml.writeStartElement("value");
			xml.writeCharacters(QString::number(value, 16));
			xml.writeEndElement();
		};

	xml.writeStartDocument();
	xml.writeDTD("<!DOCTYPE colorizer>");

	xml.writeStartElement("colorizermap");

	xml.writeStartElement("nanmapping");
	xml.writeStartElement("value");
	xml.writeCharacters(QString::number(this->getNanColor()));
	xml.writeEndElement();
	xml.writeEndElement();

	for(QPair<float, QRgb> mapping : this->getMappings()){
		xml.writeStartElement("mapping");
		writeMapping(mapping.first, mapping.second);
		xml.writeEndElement();
	}

	xml.writeEndElement();
	xml.writeEndDocument();
	ioDevice->close();
}

void
Colorizer::toXML(QString filename) const
{
    QFile file(filename);
    if(file.exists()){
        toXML(&file);
    }
}

void
Colorizer::loadFromXML(QIODevice *ioDevice)
{
    Colorizer* loaded = fromXML(ioDevice);
    if(loaded != nullptr){
        this->clearMappings();
        for(auto mapping : loaded->getMappings()){
            this->putMapping(mapping);
        }
        this->setNanColor(loaded->getNanColor());
        loaded->deleteLater();
    }
}

void Colorizer::loadFromXML(QString filename)
{
    QFile file(filename);
    if(file.exists()){
        loadFromXML(&file);
    }
}

Colorizer*
Colorizer::redPeakColorizer()
{
	Colorizer* colorizer = new Colorizer();
	colorizer->putMapping(0, qRgba(0,0,0,255));
	colorizer->putMapping(0.95f, qRgba(242,242,242,255));
	colorizer->putMapping(0.99f, qRgba(255,0,0,255));
	return colorizer;
}


Colorizer*
Colorizer::increasingTransparencyColorizer()
{
	Colorizer* colorizer = new Colorizer();
	colorizer->putMapping(0, qRgba(255,255,255,0));
	colorizer->putMapping(1, qRgba(255,255,255,255));
	return colorizer;
}


Colorizer*
Colorizer::fromXML(QIODevice *ioDevice)
{
	if(!ioDevice->isOpen()){
		if(!ioDevice->open(QIODevice::ReadOnly)){
			qDebug() << "cannot open ioDevice for xml reading";
			return nullptr;
		}
	} else {
		if(ioDevice->openMode() == QIODevice::WriteOnly){
			qDebug() << "cannot write to ioDevice for xml reading, is opened write only";
			return nullptr;
		}
	}

	Colorizer* colorizer = new Colorizer();
	QXmlStreamReader xml(ioDevice);


    float key = 0; QRgb value = 0; bool nanMapping = false; bool readKey=false; bool readValue=false;
	while (!xml.atEnd()) {
		QXmlStreamReader::TokenType token = xml.readNext();
		if(token == QXmlStreamReader::StartElement){
			QString name = xml.name().toString();
            //qDebug() << name;
			if(name == "nanmapping"){
				nanMapping = true;
			} else if(name == "mapping"){
				nanMapping = false;
			} else if(name == "key"){
				key = xml.readElementText().toFloat();
				readKey = true;
			} else if(name == "value"){
				value = xml.readElementText().toUInt(nullptr, 16);
				readValue = true;
			}
		}
		// --
		if(nanMapping && readValue){
			colorizer->setNanColor(value);
			readKey = readValue = false;
		} else if(readValue && readKey){
			colorizer->putMapping(key, value);
			readKey = readValue = false;
		}
	}

	ioDevice->close();
	return colorizer;
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
