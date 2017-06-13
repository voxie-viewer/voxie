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
    explicit ScriptConsole(QWidget *parent, const QString& title);

private slots:
    /**
     * @brief Executes the current script snippet.
     */
    void executeScript();

public slots:
    /**
     * @brief Appends a message to the log window.
     * @param log The text to be logged.
     */
    void append(const QString &log);

    void appendLine(const QString &log);

signals:
    void executeCode(const QString& code);
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
