#include "rawimporter.hpp"

#include <Voxie/data/voxeldata.hpp>

#include <Voxie/scripting/scriptingexception.hpp>

#include <Voxie/io/operation.hpp>

#include <cmath>
#include <cstdlib>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

using namespace voxie::data;
using namespace voxie::io;
using namespace voxie::scripting;

RAWImporter::RAWImporter(QObject *parent) :
	Loader(Filter("RAW Files", {"*.raw"}), parent)
{

}

static inline size_t divrem(size_t value, size_t divisor, size_t &rL)
{
	std::lldiv_t result = std::lldiv(value, divisor);
	rL = result.rem;
	return result.quot;
}

static bool isPowerOfThree(size_t nL)
{
	// General algorithm only
	size_t rL, r;
	nL = divrem(nL, 3486784401, rL); if(nL!=0 && rL!=0) return false;
	nL = divrem(nL+rL,   59049, rL); if(nL!=0 && rL!=0) return false;
	size_t n = (size_t)nL + (size_t)rL;
	n = divrem(n,   243, r); if(n!=0 && r!=0) return false;
	n = divrem(n+r,  27, r); if(n!=0 && r!=0) return false;
	n += r;
	return n==1 || n==3 || n==9;
}

QSharedPointer<voxie::data::VoxelData> RAWImporter::load(const QSharedPointer<voxie::io::Operation>& op, const QString &fileName)
{
	QFile file(fileName);

	size_t fileSize = file.size();

	if(file.exists() == false)
	{
        throw ScriptingException("de.uni_stuttgart.Voxie.RAWImporter.FileNotFound", "File not found");
	}

	if(fileSize == 0)
	{
        throw ScriptingException("de.uni_stuttgart.Voxie.RAWImporter.Error", "File must not be empty");
	}

	if(isPowerOfThree(fileSize >> 2) == false)
	{
        throw ScriptingException("de.uni_stuttgart.Voxie.RAWImporter.Error", "File must be a cubic data set");
	}

	size_t size = std::pow(fileSize >> 2, 1.0f / 3.0f);
	if((size * size * size) != (fileSize >> 2))
	{
        throw ScriptingException("de.uni_stuttgart.Voxie.RAWImporter.Error", "Data set dimensions are too large for double");
	}

	auto data = VoxelData::create(size, size, size);
	if(file.open(QFile::ReadOnly) == false)
	{
        throw ScriptingException("de.uni_stuttgart.Voxie.RAWImporter.Error", "Failed to open file");
	}

    op->throwIfCancelled();
	if(static_cast<size_t>(file.read(reinterpret_cast<char*>(data->getData()), fileSize)) != fileSize)
	{
        throw ScriptingException("de.uni_stuttgart.Voxie.RAWImporter.Error", "Failed to read whole file");
	}
    op->updateProgress(1.0);

	file.close();

    return data;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
