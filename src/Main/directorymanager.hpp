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

    QList<QString> pluginPath_;
    QList<QString> scriptPath_;

    static bool isRunningFromQtCreator();

public:
    DirectoryManager(QObject* parent = nullptr);
    virtual ~DirectoryManager();

    const QList<QString>& pluginPath() const { return pluginPath_; }
    const QList<QString>& scriptPath() const { return scriptPath_; }
};

}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
