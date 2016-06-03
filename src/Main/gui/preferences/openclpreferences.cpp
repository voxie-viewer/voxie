#include "openclpreferences.hpp"

#include <Main/root.hpp>

#include <Voxie/opencl/clinstance.hpp>

#include <functional>

#include <QtCore/QSettings>
#include <QtCore/QStringBuilder>

using namespace voxie::gui::preferences;
using namespace voxie;

int openclRole = 42;
const QString OpenclPreferences::defaultPlatformSettingsKey("default_platform");
const QString OpenclPreferences::defaultDevicesSettingsKey("default_devices");

OpenclPreferences::OpenclPreferences(QWidget *parent) :
	QWidget(parent)
{
    qRegisterMetaType<cl::Device>();
    qRegisterMetaType<cl::Platform>();

	setupElements(setupLayout());
}

QGridLayout*
OpenclPreferences::setupLayout()
{
	QGridLayout* layout = new QGridLayout(this);
	// column spacing
	layout->setColumnMinimumWidth(1, 10);
    layout->setColumnMinimumWidth(2, 10);
    layout->setColumnMinimumWidth(3, 10);

	// row spacing
	layout->setRowMinimumHeight(1, 10);
	layout->setRowMinimumHeight(3, 10);
	layout->setRowMinimumHeight(5, 20);

	return layout;
}

QListWidgetItem*
makeDeviceItem(QListWidget* list, cl::Device& device){
    try{
        using namespace voxie::opencl;
        QListWidgetItem* item = new QListWidgetItem(getName(device), list);
        item->setData(openclRole, QVariant::fromValue(device)); // save device id
        if(getType(device) == CL_DEVICE_TYPE_CPU){
            item->setIcon(QIcon(":/icons/processor.png"));
        } else if(getType(device) == CL_DEVICE_TYPE_GPU){
            item->setIcon(QIcon(":/icons/graphic-card.png"));
        }
        return item;
    } catch(voxie::opencl::CLException& ex){
        qWarning() << ex;
    }
    return nullptr;
}

void
OpenclPreferences::setupElements(QGridLayout* layout)
{
    // init elements
    this->lbl_currentPlatform = new QLabel("current Platform");
    this->cmb_platformToUse = new QComboBox();
    this->list_currentDevices = new QListWidget();
    this->list_devicesToUse = new QListWidget();
    this->lbl_currentDeviceInfo = new QLabel("current device info");
    this->lbl_deviceToUseInfo = new QLabel("device to use info");
    this->btn_saveConfig = new QPushButton("save configuration");
    this->lbl_needToRestart = new QLabel("");

    // place elements
    layout->addWidget(this->lbl_currentPlatform, 0, 0);
    layout->addWidget(this->cmb_platformToUse, 0, 4);

    layout->addWidget(this->list_currentDevices, 2, 0);
    layout->addWidget(this->list_devicesToUse, 2, 4);

    layout->addWidget(this->lbl_currentDeviceInfo, 4, 0);
    layout->addWidget(this->lbl_deviceToUseInfo, 4, 4);

    layout->addWidget(this->btn_saveConfig, 6, 4);
    layout->addWidget(this->lbl_needToRestart, 7, 4);

    // setup content
    connect(this->list_currentDevices, &QListWidget::itemClicked, this, &OpenclPreferences::currentDeviceSelected);
    connect(this->list_devicesToUse, &QListWidget::itemClicked, this, &OpenclPreferences::deviceToUseSelected);
    connect(this->cmb_platformToUse, SIGNAL(activated(int)), this, SLOT(platformToUseSelected(int)) );
    connect(this->btn_saveConfig, &QPushButton::pressed, this, &OpenclPreferences::saveConfig);

    try{
        using namespace voxie::opencl;
        CLInstance* instance = CLInstance::getDefaultInstance();
        this->lbl_currentPlatform->setText(getName(instance->getPlatform()));

        for(cl::Device device : instance->getDevices()){
            makeDeviceItem(list_currentDevices, device);
        }
    } catch(voxie::opencl::CLException& ex){
        qWarning() << ex;
    }

    try{
        using namespace voxie::opencl;
        bool deviceListInit = false;
        for(cl::Platform platform : getPlatforms()){
            this->cmb_platformToUse->addItem(getName(platform), QVariant::fromValue(platform));
            if(!deviceListInit){
                this->updateDeviceToUseList(platform);
                deviceListInit = true;
            }
        }
    } catch(voxie::opencl::CLException& ex){
        qWarning() << ex;
    }

}

void
updateDeviceInfoLabel(QLabel* label, const cl::Device& device)
{
    using namespace voxie::opencl;
    try{
        QString numComputeUnits = QString::number(getNumComputeUnits(device));
        QString memorySize = QString::number(getGlobalMemorySize(device)/(1024*1024));
        QString clockFrequency = QString::number(getClockFrequency(device));
        label->setWordWrap(true);
        label->setText("Compute Units: " % numComputeUnits % "\nClock Frequency: " % clockFrequency % " mHz\nMemory Size: " % memorySize % " mb");
    } catch(CLException& ex) {
        qWarning() << ex;
    }
}

void
OpenclPreferences::currentDeviceSelected(QListWidgetItem *item)
{
    cl::Device device = item->data(openclRole).value<cl::Device>();
    updateDeviceInfoLabel(this->lbl_currentDeviceInfo, device);
}

void
OpenclPreferences::deviceToUseSelected(QListWidgetItem *item)
{
    cl::Device device = item->data(openclRole).value<cl::Device>();
    updateDeviceInfoLabel(this->lbl_deviceToUseInfo, device);
}

void
OpenclPreferences::platformToUseSelected(int index)
{
    cl::Platform platform = this->cmb_platformToUse->itemData(index).value<cl::Platform>();
    this->updateDeviceToUseList(platform);
}


void
OpenclPreferences::updateDeviceToUseList(cl::Platform platform)
{
    using namespace voxie::opencl;
    try{
        QVector<cl::Device> devices = getDevices(platform);
        this->list_devicesToUse->clear();
        for(cl::Device device : devices){
            QListWidgetItem* item = makeDeviceItem(this->list_devicesToUse, device);
            if(item != nullptr){
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Unchecked);
            }
        }
    } catch(CLException& ex) {
        qWarning() << ex;
    }
}

void
OpenclPreferences::saveConfig()
{
    QString platformName = this->cmb_platformToUse->currentText();
    QStringList selectedDevices;
    for(int i = 0; i < this->list_devicesToUse->count(); i++){
        QListWidgetItem* item = this->list_devicesToUse->item(i);
        if(item->checkState() == Qt::Checked){
            selectedDevices.append(item->text());
        }
    }

    if(selectedDevices.isEmpty() || platformName.isEmpty()){
        return;
    }

    Root::instance()->settings()->setValue(OpenclPreferences::defaultPlatformSettingsKey, platformName);
    Root::instance()->settings()->setValue(OpenclPreferences::defaultDevicesSettingsKey, selectedDevices);

    this->lbl_needToRestart->setWordWrap(true);
    this->lbl_needToRestart->setText("<font color='red'>Saved - You need to restart the application for the changes to take effect</font>");
}


OpenclPreferences::~OpenclPreferences()
{

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
