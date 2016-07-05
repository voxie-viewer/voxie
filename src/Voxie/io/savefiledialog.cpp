#include "savefiledialog.hpp"

#include <QtCore/QDebug>

using namespace voxie::io;

SaveFileDialog::SaveFileDialog(QWidget* parent, const QString& caption, const QString& directory) : QFileDialog(parent, caption, directory) {
    setAcceptMode(QFileDialog::AcceptSave);
    setFileMode(QFileDialog::AnyFile);
    setOption(QFileDialog::DontUseNativeDialog, true);
}
SaveFileDialog::~SaveFileDialog() {
}

const SaveFileDialog::Filter* SaveFileDialog::currentFilter() {
    QString str = selectedNameFilter();
    for (int i = 0; i < filters.size(); i++)
        if (filters[i].filterString == str)
            return &filters[i];
    
    qWarning() << "SaveFileDialog: Could not find filter:" << str;
    return nullptr;
}

void SaveFileDialog::addFilter(const QString& description, const QStringList& extensions, void* data) {
    Filter filter;
    filter.description = description;
    filter.extensions = extensions;
    filter.data = data;

    if (extensions.size() == 0) {
        qCritical() << "Got a filter without any extensions";
        return;
    }

    for (const auto& ext : filter.extensions)
        filter.lowerExtensions << ext.toLower();

    filter.filterString = filter.description + " (";
    for (int i = 0; i < extensions.size(); i++) {
        if (i != 0)
            filter.filterString += " ";
        filter.filterString += "*." + extensions[i];
    }
    filter.filterString += ")";

    filters.push_back(filter);
}

void SaveFileDialog::setup() {
    QStringList filterList;
    for (int i = 0; i < filters.size(); i++)
        filterList << filters[i].filterString;
    setNameFilters(filterList);
}

void SaveFileDialog::accept() {
    //qDebug() << "accept" << selectedFiles().size() << selectedFiles().first();
    QStringList files = selectedFiles();
    if (selectedFiles().size() == 0) {
        QFileDialog::accept();
        return;
    }
    QString file = selectedFiles()[0];

    if (QFileInfo(file).isDir()) {
        QFileDialog::accept();
        return;
    }

    QString fileLower = file.toLower();
    for (const auto& filter : filters) {
        for (const auto& extension : filter.extensions) {
            if (fileLower.endsWith("." + extension)) {
                selectNameFilter(filter.filterString);
                QFileDialog::accept();
                return;
            }
        }
    }

    const Filter* filter = currentFilter();
    if (filter && filter->extensions.size()) {
        file += "." + filter->extensions[0];
        this->setFocus(Qt::OtherFocusReason); // Make lineEdit loose focus, otherwise if the lineEdit is selected it will not be updated
        this->selectFile(file);
        //qDebug() << "selectFile" << file;
        //qDebug() << "selectedFiles" << selectedFiles().size() << selectedFiles()[0];
    }

    QFileDialog::accept();
}

void* SaveFileDialog::selectedFilterData() {
    const Filter* filter = currentFilter();
    if (!filter)
        return nullptr;
    return filter->data;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
