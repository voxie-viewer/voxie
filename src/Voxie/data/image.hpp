#pragma once

#include <Voxie/data/floatimage.hpp>

#include <Voxie/scripting/scriptingcontainer.hpp>
#include <Voxie/scripting/scriptingexception.hpp>

#include <QtCore/QObject>

#include <QtDBus/QDBusContext>

namespace voxie { namespace data {

class VOXIECORESHARED_EXPORT Image : public voxie::scripting::ScriptableObject, public QDBusContext {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Image")

    FloatImage image_;

public:
    // throws ScriptingException
    Image(quint64 width, quint64 height);
    virtual ~Image();

    FloatImage& image() { return image_; }
    const FloatImage& image() const { return image_; }

    Q_PROPERTY (quint64 Width READ width);
    quint64 width () const { return image().getWidth (); }

    Q_PROPERTY (quint64 Height READ height);
    quint64 height () const { return image().getHeight (); }

    Q_SCRIPTABLE double GetPixel (quint64 x, quint64 y) {
        try {
            if (x >= width () || y >= height ())
                throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IndexOutOfRange", "Index is out of range");
            return image().getPixel (x, y);
        } catch (voxie::scripting::ScriptingException& e) {
            e.handle(this);
            return 0;
        }
    }
    Q_SCRIPTABLE void SetPixel (quint64 x, quint64 y, double val) {
        try {
            if (x >= width () || y >= height ())
                throw voxie::scripting::ScriptingException("de.uni_stuttgart.Voxie.IndexOutOfRange", "Index is out of range");
            image().setPixel (x, y, val);
        } catch (voxie::scripting::ScriptingException& e) {
            e.handle(this);
        }
    }

    Q_SCRIPTABLE void UpdateBuffer (const QMap<QString, QVariant>& options) {
        try {
            checkOptions(options);
            image().switchMode (FloatImage::STDMEMORY_MODE);
        } catch (voxie::scripting::ScriptingException& e) {
            e.handle(this);
        }
    }

    Q_SCRIPTABLE voxie::scripting::Array2Info GetDataReadonly ();
    //Q_SCRIPTABLE voxie::scripting::Array2Info GetDataWritable ();
};

}}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
