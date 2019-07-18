#include "common_noise.h"

const size_t too_frequent = 1024;
Default_noise_filter default_noise_filter;

Common_noise::Common_noise()
{}


Common_noise::~Common_noise()
{
	for (size_t i = 0; i < _noise_entry.size(); i++)
	{
		if(_noise_entry[i] != NULL) delete [] _noise_entry[i];
	}
}


bool Common_noise::contains(uint64_t seed, uint64_t shape) const
{
	int first = 0, last = (int)_limit[shape] - 1;
	noise *noise_entry_ = _noise_entry[shape];

	while (first <= last)
	{
		int mid = (first + last) / 2;
		if (noise_entry_[mid].seed == seed) return true;
		if (noise_entry_[mid].seed < seed) first = mid + 1;
		else                               last = mid - 1;
	}
	return false;
}

