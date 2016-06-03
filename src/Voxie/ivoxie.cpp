#include "ivoxie.hpp"

voxie::IVoxie *root = nullptr;

void voxie::setVoxieRoot(IVoxie *voxie)
{
	::root = voxie;
}

voxie::IVoxie &voxie::voxieRoot()
{
	return *::root;
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
