#pragma once

#include <QtCore/QList>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QWidget>

namespace voxie
{
namespace gui
{
namespace preferences
{

class ScriptPreferences : public QWidget
{
	Q_OBJECT
private:
	struct SettingsEntry {
		QString extension;
		QString executable;
		QString arguments;
	};

	QListWidget *list;
	QLineEdit *nameEdit;
	QLineEdit *extensionEdit;
	QLineEdit *executableEdit;
	QLineEdit *argsEdit;

	QMap<QString, SettingsEntry> scriptingTools;
public:
	explicit ScriptPreferences(QWidget *parent = 0);
	~ScriptPreferences();

private:
	/**
	 * @brief Fills the list with the entries from the settings.
	 */
	void updateList();

	/**
	 * @brief Fills the data fields with the data from the selected entry.
	 */
	void updateData();

	/**
	 * @brief Stores the data from the data form into the selected entry.
	 */
	void storeData();

	/**
	 * @brief Adds a new entry.
	 */
	void addNew();

	/**
	 * @brief Removes the currently selected entry.
	 * @todo Implement message box.
	 */
	void removeCurrent();

	/**
	 * @brief Duplicates the current entry.
	 */
	void duplicateCurrent();

	/**
	 * @brief Sets the state of the edit fields.
	 * @param all Enable state of all fields except the name edit.
	 * @param enableName Enable state of the name edit.
	 */
	void setEnabled(bool all, bool enableName);

	/**
	 * @brief Saves the preferences.
	 */
	void save();

signals:

public slots:
};

}
}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
