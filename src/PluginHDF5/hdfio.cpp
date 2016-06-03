#include "hdfio.hpp"

#include <PluginHDF5/hdfexporter.hpp>
#include <PluginHDF5/hdfloader.hpp>
#include <PluginHDF5/hdfsliceexporter.hpp>

using namespace voxie::io;
using namespace voxie::plugin;

HDFIO::HDFIO(QObject *parent) :
	QGenericPlugin(parent)
{
}

QObject* HDFIO::create ( const QString & key, const QString & specification )
{
	(void)key;
	(void)specification;
	return nullptr;
}

QVector<Loader*> HDFIO::loaders()
{
	QVector<voxie::io::Loader*> list;
	list.append(new HDFLoader());
	return list;
}

QVector<VoxelExporter*> HDFIO::voxelExporters()
{
	QVector<voxie::io::VoxelExporter*> list;
	list.append(new HDFExporter());
	return list;
}

QVector<SliceExporter*> HDFIO::sliceExporters()
{
	QVector<voxie::io::SliceExporter*> list;
	list.append(new HDFSliceExporter());
	return list;
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(HDF-IO, HDFIO)
#endif // QT_VERSION < 0x050000

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
