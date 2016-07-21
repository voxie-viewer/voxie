#pragma once

#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

namespace voxie {
namespace io {
class Loader;

class LoaderAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Loader")

    Loader* object;

public:
    LoaderAdaptor (Loader* object);
    ~LoaderAdaptor () override;

    Q_PROPERTY (QVariantMap Filter READ filter)
    QVariantMap filter();

public slots:
    QDBusObjectPath Load(const QString &fileName, const QMap<QString, QVariant>& options);
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
