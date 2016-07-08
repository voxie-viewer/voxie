#include "loader.hpp"

#include <Voxie/data/dataset.hpp>
#include <Voxie/data/voxeldata.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

using namespace voxie::io;
using namespace voxie::io::internal;

Loader::Filter::Filter (const QString& description, const QStringList& patterns) : description_ (description), patterns_ (patterns) {
    filterString_ = this->description() + " (" + this->patterns().join(" ") + ")";
#if !defined(Q_OS_WIN)
    Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
    Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif
    for (const QString& pattern : patterns_)
        compiledPatterns_ << QRegExp(pattern, caseSensitivity, QRegExp::Wildcard);
}

Loader::Filter::Filter (const QVariantMap& filter) : Filter (filter["Description"].toString(), filter["Patterns"].toStringList()) {
}

Loader::Filter::operator QVariantMap () const {
    QVariantMap filter;
    filter["Description"] = description();
    filter["Patterns"] = patterns();
    return filter;
}

bool Loader::Filter::matches (const QString& filename) const {
    for (const QRegExp& regexp : compiledPatterns())
        if (regexp.exactMatch(filename))
            return true;
    return false;
}


Loader::Loader(Filter filter, QObject *parent) :
    voxie::plugin::PluginMember("Loader", parent), filter_ (filter)
{
    new LoaderAdaptor (this);
}

Loader::~Loader()
{

}

voxie::data::DataSet* Loader::load(const QString &fileName) {
    return registerVoxelData(loadImpl(fileName), fileName);
}

voxie::data::DataSet* Loader::registerVoxelData(const QSharedPointer<voxie::data::VoxelData>& data, const QString &fileName) {
    voxie::data::DataSet* dataSet = new voxie::data::DataSet(data);
    dataSet->setFileInfo(QFileInfo(fileName));
    dataSet->setObjectName(data->objectName());
    emit dataLoaded(dataSet);
    return dataSet;
}

QDBusObjectPath LoaderAdaptor::Load(const QString &fileName, const QMap<QString, QVariant>& options) {
    try {
        voxie::scripting::ScriptingContainerBase::checkOptions(options);
        return voxie::scripting::ScriptingContainerBase::getPath(object->load(fileName));
    } catch (voxie::scripting::ScriptingException& e) {
        e.handle(object);
        return voxie::scripting::ScriptingContainerBase::getPath(nullptr);
    }
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
