#include "loader.hpp"

#include <Voxie/ivoxie.hpp>

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

using namespace voxie::io;

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
    voxieRoot().createLoaderAdaptor(this);
}

Loader::~Loader()
{

}

void Loader::setSelf(const QSharedPointer<Loader>& ptr) {
    if (ptr.data() != this) {
        qCritical() << "Loader::setSelf called with incorrect object";
        return;
    }
    if (self) {
        qCritical() << "Loader::setSelf called with multiple times";
        return;
    }
    self = ptr;
}


// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
