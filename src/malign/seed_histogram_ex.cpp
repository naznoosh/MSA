#include "seed_histogram_ex.h"

//seedp_range current_range;

Seqs_histogram::Seqs_histogram()
{ }
/*
size_t Partitioned_histogram::max_chunk_size() const
{
	size_t max = 0;
	::partition<unsigned> p(Const::seedp, config.lowmem);
	for (unsigned shape = 0; shape < shapes.count(); ++shape)
		for (unsigned chunk = 0; chunk < p.parts; ++chunk)
			max = std::max(max, hst_size(data_[shape], seedp_range(p.getMin(chunk), p.getMax(chunk))));
	return max;
}
*/