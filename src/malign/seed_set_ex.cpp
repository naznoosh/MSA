#include "../malign/seed_set_ex.h"
#include "../malign/minimizer.h"

struct Hashed_common_seed_set_callback
{
	Hashed_common_seed_set_callback(Ptr_vector<PHash_set<Modulo2, No_hash> > &dst, Ptr_vector<PHash_set<Modulo2, No_hash> > *filter = NULL, Minimizer *minimizer = NULL) :
		dst(dst),
		minimizer (minimizer),
		filter (filter) {}

	bool operator()(uint64_t seed, uint64_t pos, uint64_t shape)
	{
		if(!minimizer || minimizer->minimize(seed, shape))
			if(!filter || (*filter)[shape].contains(seed))
				dst[shape].insert(seed);

		return true;
	}
	void finish()
	{}
	Ptr_vector<PHash_set<Modulo2, No_hash> > &dst;
	Minimizer *minimizer;
	Ptr_vector<PHash_set<Modulo2, No_hash> > *filter;
};


Hashed_common_seed_set::Hashed_common_seed_set(const Sequence_set &seqs)
{
	size_t hash_sz = 0;
	for (size_t i = 0; i < seqs.get_length(); ++i)
		if (seqs[i].length() > hash_sz)
			hash_sz = seqs[i].length();

	hash_sz = next_power_of_2(hash_sz * 1.25);

	for (size_t i = 0; i < shapes.count(); ++i)
		data_.push_back(new PHash_set<Modulo2, No_hash>(hash_sz));

	Minimizer m;
	Minimizer *minimizer = NULL;
	if (config.minimizer) minimizer = &m;

//	Ptr_vector<Hashed_common_seed_set_callback> v;
//	v.push_back(new Hashed_common_seed_set_callback(data_, NULL, minimizer));
//	seqs.enum_seeds(v, vector<size_t>{0, 1}, 0, shapes.count(), &no_filter);
	Hashed_common_seed_set_callback v(data_, NULL, minimizer);
	seqs.enum_seeds(&v, 0, 1, std::make_pair(0, shapes.count()), &no_filter);

	Ptr_vector<PHash_set<Modulo2, No_hash> > data_;
	for (unsigned i = 1; i < seqs.get_length(); ++i)
	{
		for (size_t k = 0; k < shapes.count(); ++k)
			data_.push_back(new PHash_set<Modulo2, No_hash>(hash_sz));
		if (minimizer) minimizer->reset();
//		Ptr_vector<Hashed_common_seed_set_callback> v;
//		v.push_back(new Hashed_common_seed_set_callback(data_, &this->data_, minimizer));
//		seqs.enum_seeds(v, vector<size_t>{i, i + 1}, 0, shapes.count(), &no_filter/*this*/);
		Hashed_common_seed_set_callback v(data_, &this->data_, minimizer);
		seqs.enum_seeds(&v, i, i + 1, std::make_pair(0, shapes.count()), &no_filter/*this*/);
		this->data_.clear();
		this->data_ = data_;
		data_.resize(0);
	}
}
