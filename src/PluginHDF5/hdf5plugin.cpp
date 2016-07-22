#include "hdf5plugin.hpp"

#include <PluginHDF5/hdfexporter.hpp>
#include <PluginHDF5/hdfloader.hpp>
#include <PluginHDF5/hdfsliceexporter.hpp>

using namespace voxie::io;
using namespace voxie::plugin;

HDF5Plugin::HDF5Plugin(QObject *parent) :
	QGenericPlugin(parent)
{
}

QObject* HDF5Plugin::create ( const QString & key, const QString & specification )
{
	(void)key;
	(void)specification;
	return nullptr;
}

QVector<Loader*> HDF5Plugin::loaders()
{
	QVector<voxie::io::Loader*> list;
	list.append(new HDFLoader());
	return list;
}

QVector<VoxelExporter*> HDF5Plugin::voxelExporters()
{
	QVector<voxie::io::VoxelExporter*> list;
	list.append(new HDFExporter());
	return list;
}

QVector<SliceExporter*> HDF5Plugin::sliceExporters()
{
	QVector<voxie::io::SliceExporter*> list;
	list.append(new HDFSliceExporter());
	return list;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
