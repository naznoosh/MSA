#include "minimizer.h"
#include "../Diamond/basic/shape_config.h"


Minimizer::Minimizer() :
	min_seed(shapes.count(), -1),
	idx(shapes.count())
{
//	min_seed = new uint64_t[shapes.count()]{ (uint64_t)-1 };
//	idx = new size_t[shapes.count()]{ 0 };
//	wnd = new vector <uint64_t>[shapes.count()];

	for (size_t i = 0; i < shapes.count(); ++i)
	{
		size_t wnd_size = shapes[i].length_;
		if (shapes[i].positions_[shapes[i].weight_ - 1] == shapes[i].weight_ - 1) wnd_size = shapes[i].weight_;
		wnd.push_back(vector <size_t>(wnd_size));
	}
}

bool Minimizer::minimize(size_t &seed, size_t shape)
{
	vector <size_t> &wnd = this->wnd[shape];
	size_t & idx = this->idx[shape];
	size_t &min_seed = this->min_seed[shape];


	if (wnd[idx] == min_seed)
	{
		wnd[idx] = seed;

		min_seed = seed;
		for (size_t k = 0; k < wnd.size(); k++)
		{
			if (wnd[k] < min_seed) min_seed = wnd[k];
		}

		if (++idx >= wnd.size()) idx = 0;
		seed = min_seed;
		return true;
	}

	wnd[idx] = seed;
	if (++idx >= wnd.size()) idx = 0;

	if (seed < min_seed)
	{
		min_seed = seed;
		return true;
	}

	return false;
}

void Minimizer::reset()
{
	min_seed.assign(min_seed.size(), -1);
	idx.assign(idx.size(), 0);

	for (size_t i = 0; i < shapes.count(); ++i)	wnd[i].assign(wnd[i].size(), 0);
}

/*
struct Minimizer_hashed_common_seed_set_callback
{
	Minimizer_hashed_common_seed_set_callback(Ptr_vector<PHash_set<Modulo2, No_hash> > &dst, size_t wnd_size) :
		dst(dst),
		wnd(wnd_size),
		idx(0),
		min_seed(-1) {}

	bool operator()(uint64_t seed, uint64_t pos, uint64_t shape)
	{
		if (is_minimize(seed))
		{
			dst[shape].insert(seed);
			return true;
		}
		return false;
	}

	void finish()
	{

	}

	bool is_minimize(uint64_t seed)
	{
		if (wnd[idx] == min_seed)
		{
			wnd[idx] = seed;

			min_seed = seed;
			for (size_t k = 0; k < wnd.size(); k++)
			{
				if (wnd[k] < min_seed) min_seed = wnd[k];
			}

			if (idx++ >= wnd.size()) idx = 0;
			return false;
		}

		wnd[idx] = seed;
		if (idx++ >= wnd.size()) idx = 0;

		if (seed < min_seed)
		{
			min_seed = seed;
			return true;
		}

		return false;
	}

	Ptr_vector<PHash_set<Modulo2, No_hash> > &dst;
	vector <uint64_t> wnd;
	size_t idx;
	uint64_t min_seed;
};
*/