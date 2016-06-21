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

ScriptConsole::ScriptConsole(QWidget *parent) :
	QDialog(parent),
	snippetEdit(nullptr),
	scriptLog(nullptr)
{
	this->setWindowTitle("Voxie - Script Console");
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

	connect(Root::instance(), &Root::logEmitted, this, &ScriptConsole::appendLog);

    for (const auto& msg : Root::getBufferedMessages())
        this->scriptLog->append(msg);
}

void ScriptConsole::executeScript()
{
	QString snippet = this->snippetEdit->text();

    /*
	if(Root::instance()->exec(snippet)) {
		this->snippetEdit->setText("");
	}
    */

    Root::instance()->exec(snippet, snippet);
    this->snippetEdit->setText("");
}

void ScriptConsole::appendLog(const QString &log)
{
	this->scriptLog->append(log);
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
