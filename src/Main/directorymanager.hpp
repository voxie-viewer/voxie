#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

namespace voxie
{
/**
 * @brief A class for finding plugins and scripts.
 */
class DirectoryManager : public QObject {
    Q_OBJECT

    QString baseDir_;
    QList<QString> pluginPath_;
    QList<QString> scriptPath_;
    QString pythonLibDir_;

    static bool isRunningFromQtCreator();

public:
    DirectoryManager(QObject* parent = nullptr);
    virtual ~DirectoryManager();

    const QString& baseDir() const { return baseDir_; }
    const QList<QString>& pluginPath() const { return pluginPath_; }
    const QList<QString>& scriptPath() const { return scriptPath_; }
    const QString& pythonLibDir() const { return pythonLibDir_; }
};

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
