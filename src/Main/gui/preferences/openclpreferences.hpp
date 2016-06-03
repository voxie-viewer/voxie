#pragma once

#include <Voxie/lib/CL/cl.hpp>

#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

namespace voxie
{
namespace gui
{
namespace preferences
{

class OpenclPreferences : public QWidget
{
    Q_OBJECT
public:
    explicit OpenclPreferences(QWidget *parent = 0);
    ~OpenclPreferences();

    static const QString defaultPlatformSettingsKey;
    static const QString defaultDevicesSettingsKey;

private:
    QGridLayout *setupLayout();
    void setupElements(QGridLayout* layout);
    void updateDeviceToUseList(cl::Platform platform);

    // elements
    QLabel* lbl_currentPlatform;
    QComboBox* cmb_platformToUse;
    QListWidget* list_currentDevices;
    QListWidget* list_devicesToUse;
    QLabel* lbl_currentDeviceInfo;
    QLabel* lbl_deviceToUseInfo;
    QPushButton* btn_saveConfig;
    QLabel* lbl_needToRestart;

signals:

private slots:
    void currentDeviceSelected(QListWidgetItem* item);
    void deviceToUseSelected(QListWidgetItem* item);
    void platformToUseSelected(int index);
    void saveConfig();
};


}
}
}

Q_DECLARE_METATYPE(cl::Device)
Q_DECLARE_METATYPE(cl::Platform)

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
