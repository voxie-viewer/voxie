#include "scriptconsole.hpp"

#include <Main/root.hpp>

#include <Main/gui/scriptlineedit.hpp>

#include <QtGui/QFontDatabase>

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

using namespace voxie;
using namespace voxie::gui;

ScriptConsole::ScriptConsole(QWidget *parent, const QString& title) :
	QDialog(parent),
	snippetEdit(nullptr),
	scriptLog(nullptr)
{
	this->setWindowTitle(title);
	this->resize(800, 500);

	this->scriptLog = new QTextEdit(this);
	this->scriptLog->setReadOnly(true);
	this->scriptLog->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	this->snippetEdit = new ScriptLineEdit(this);
	this->snippetEdit->setFocus();
	this->snippetEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	QPushButton *execButton = new QPushButton(this);
	execButton->setText("Run!");
	connect(execButton, &QPushButton::clicked, this, &ScriptConsole::executeScript);

	QHBoxLayout *hlayout = new QHBoxLayout();
	hlayout->addWidget(this->snippetEdit);
	hlayout->addWidget(execButton);

	QVBoxLayout *vlayout  = new QVBoxLayout();
	vlayout->addWidget(this->scriptLog);
	vlayout->addLayout(hlayout);
	this->setLayout(vlayout);

	connect(Root::instance(), &Root::logEmitted, this, &ScriptConsole::appendLine);

    for (const auto& msg : Root::getBufferedMessages())
        this->appendLine(msg);
}

void ScriptConsole::executeScript()
{
	QString snippet = this->snippetEdit->text();

    /*
	if(Root::instance()->exec(snippet)) {
		this->snippetEdit->setText("");
	}
    */

    executeCode(snippet);
    this->snippetEdit->setText("");
}

#include <iostream>

void ScriptConsole::append(const QString &log)
{
    this->scriptLog->moveCursor(QTextCursor::End);
    this->scriptLog->insertPlainText(log);
    this->scriptLog->ensureCursorVisible();
}

void ScriptConsole::appendLine(const QString &log)
{
    this->append(log + "\n");
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
