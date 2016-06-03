#include "directorymanager.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QStandardPaths>

using namespace voxie;

bool DirectoryManager::isRunningFromQtCreator() {
    // Hack to find out whether the program has been launched from QtCreator (in this case the directories for plugins and scripts are different)
    return QProcessEnvironment::systemEnvironment().contains("QTDIR");
}

DirectoryManager::DirectoryManager(QObject* parent) : QObject(parent) {
#if !defined(Q_OS_WIN)
    QString split = ":";
#else
    QString split = ";";
#endif

    QString pluginPathStr = QProcessEnvironment::systemEnvironment().value("VOXIE_PLUGIN_PATH", "");
    QString scriptPathStr = QProcessEnvironment::systemEnvironment().value("VOXIE_SCRIPT_PATH", "");

#if !defined(Q_OS_WIN)
    for (QString path : QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation)) {
        QString configDir = path + "/voxie";
        pluginPath_.push_back(configDir + "/plugins");
        scriptPath_.push_back(configDir + "/scripts");
    }
#else
    QString configDir = QDir::homePath() + "/AppData/Roaming/voxie";
    pluginPath_.push_back(configDir + "/plugins");
    scriptPath_.push_back(configDir + "/scripts");
#endif

    for (QString path : pluginPathStr.split(split)) {
        if (path == "")
            continue;
        pluginPath_.push_back(path);
    }
    for (QString path : scriptPathStr.split(split)) {
        if (path == "")
            continue;
        scriptPath_.push_back(path);
    }

    if (isRunningFromQtCreator()) {
        QDir voxieDir(QCoreApplication::applicationDirPath());
        QString suffix = "";
#if defined(Q_OS_WIN)
        suffix = "/" + voxieDir.dirName();
        voxieDir.cdUp();
#endif
        voxieDir.cdUp();

        for (QString dir : voxieDir.entryList(QStringList("*"), QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable)) {
            if((dir == "Voxie") || (dir == "VoxieCore"))
                continue;
            pluginPath_.push_back(voxieDir.absoluteFilePath(dir + suffix));
        }

        for (QString dir : voxieDir.entryList(QStringList("Script*"), QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable)) {
            scriptPath_.push_back(voxieDir.absoluteFilePath(dir + suffix));
        }

        voxieDir.cdUp();
        scriptPath_.push_back(voxieDir.absoluteFilePath("scripts"));
    } else {
        // On Linux the wrapper script is expected to set VOXIE_PLUGIN_PATH and VOXIE_SCRIPT_PATH
#if defined(Q_OS_WIN)
        QDir appDir(QCoreApplication::applicationDirPath());
        pluginPath_.push_back(appDir.absoluteFilePath("plugins"));
        scriptPath_.push_back(appDir.absoluteFilePath("scripts"));
#endif
    }
}

DirectoryManager::~DirectoryManager() {
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
