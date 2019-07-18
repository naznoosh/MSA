#pragma once
#ifndef MINIMIZER_H_
#define MINIMIZER_H_

//#include "../Diamond/src/util/hash_table.h"
//#include "../Diamond/src/util/ptr_vector.h"
#include <vector>

using std::vector;
using std::size_t;

struct Minimizer
{
	Minimizer();

	bool minimize(size_t &seed, size_t shape);

	void reset();

private:
	vector <vector <size_t>> wnd;
	vector <size_t> idx;
	vector <size_t> min_seed;

};

/*
struct Hashed_minimizer
{
	Hashed_minimizer(Ptr_vector<PHash_set<Modulo2, No_hash> > &hash, Minimizer &minimizer) :
		hash(hash),
		minimizer(minimizer) {}

	bool contains(uint64_t key, uint64_t shape) const
	{
		return hash[shape].contains(key) && minimizer.contains(key, shape);
	}
private:
	Ptr_vector<PHash_set<Modulo2, No_hash> > &hash;
	Minimizer &minimizer;
};
*/
#endif