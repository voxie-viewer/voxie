#pragma once

#include <Voxie/voxiecore_global.hpp>

#include <QtWidgets/QFileDialog>

namespace voxie {
namespace io {

// A dialog for selecting the filename which will automatically append the
// correct extension (or will select the correct filetype if an extension is
// provided by the user)
class VOXIECORESHARED_EXPORT SaveFileDialog : public QFileDialog {
    Q_OBJECT

    struct Filter {
        QString description;
        QStringList extensions;
        void* data;
        QStringList lowerExtensions;
        QString filterString;
    };

    QList<Filter> filters;

    const Filter* currentFilter();

public:
    SaveFileDialog(QWidget* parent, const QString& caption, const QString& directory);
    ~SaveFileDialog();

    void addFilter(const QString& description, const QStringList& extensions, void* data);

    void setup();

    void* selectedFilterData();
    
protected:
    void accept() override;
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
