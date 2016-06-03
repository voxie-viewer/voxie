#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>

namespace voxie
{
namespace gui
{

/**
 * @brief A script console window.
 *
 * Allows the user to enter JavaScript snippets and displays the global script log.
 */
class ScriptConsole :
        public QDialog
{
    Q_OBJECT
private:
    QLineEdit *snippetEdit;
    QTextEdit *scriptLog;
public:
    explicit ScriptConsole(QWidget *parent = 0);

private slots:
    /**
     * @brief Executes the current script snippet.
     */
    void executeScript();

    /**
     * @brief Appends a message to the log window.
     * @param log The text to be logged.
     */
    void appendLog(const QString &log);

signals:

public slots:

};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
