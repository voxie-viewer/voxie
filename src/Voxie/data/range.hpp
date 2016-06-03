#pragma once

#include <stdlib.h>

#ifdef __unix__
#undef min
#undef max
#endif
namespace voxie
{
namespace data
{

class Range
{
public:
	explicit Range() :
		min(1),
		max(1)
	{
	}

	explicit Range(size_t exactValue) : min(exactValue), max(exactValue) { }

	explicit Range(size_t min, size_t max) : min(min), max(max) { }

	inline bool isValid() const
	{
		return this->max >= this->min;
	}

	inline bool isSingularity() const
	{
		return this->isValid() && (this->max == this->min);
	}

	inline bool isNull() const
	{
		return (this->max == 0) && (this->min == 0);
	}

	inline bool contains(size_t value) const
	{
		return this->isValid() && (value >= this->min) && (value <= this->max);
	}

	size_t min;
	size_t max;
};

}
}

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
