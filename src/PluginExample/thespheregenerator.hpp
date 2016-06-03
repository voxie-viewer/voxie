#pragma once

#include <Voxie/io/importer.hpp>

class TheSphereGenerator :
		public voxie::io::Importer
{
	Q_OBJECT
public:
	explicit TheSphereGenerator(QObject *parent = 0);
	~TheSphereGenerator();

    Q_INVOKABLE voxie::data::DataSet* genSphere(int size);
    voxie::data::VoxelData* genSphereImpl(int size);

	virtual voxie::data::VoxelData* importImpl() override;
};

namespace internal {
class TheSphereGeneratorAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "de.uni_stuttgart.Voxie.Plugins.ExamplePlugin.TheSphereGenerator")

    TheSphereGenerator* object;

public:
    TheSphereGeneratorAdaptor (TheSphereGenerator* object) : QDBusAbstractAdaptor (object), object (object) {}
    virtual ~TheSphereGeneratorAdaptor () {}

public slots:
    QDBusObjectPath GenerateSphere(int size, const QMap<QString, QVariant>& options);
};
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
