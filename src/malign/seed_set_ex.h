#pragma once
#ifndef SEED_SET_EX_H_
#define SEED_SET_EX_H_

#include "../Diamond/data/sequence_set.h"
#include "../Diamond/util/hash_table.h"


struct Hashed_common_seed_set
{
	Hashed_common_seed_set(const Sequence_set &seqs);

	bool contains(uint64_t key, uint64_t shape) const
	{
		return data_[shape].contains(key);
	}

private:
	Ptr_vector<PHash_set<Modulo2, No_hash> > data_;
};


#endif