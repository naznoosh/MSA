#ifndef SEED_HISTOGRAM_EX_H_
#define SEED_HISTOGRAM_EX_H_
/*
#include <limits>
#include "sequence_set.h"
#include "../util/thread.h"
#include "../basic/seed_iterator.h"
#include "seed_set.h"
*/
//#include <vector>
#include "../Diamond/basic/seed.h"
#include "../Diamond/basic/shape_config.h"
#include "../Diamond/data/seed_histogram.h"

//using std::vector;

//typedef vector<Array<unsigned,Const::seedp> > shape_histogram;
/*
struct seedp_range
{
	seedp_range():
		begin_ (0),
		end_ (0)
	{ }
	seedp_range(unsigned begin, unsigned end):
		begin_ (begin),
		end_ (end)
	{ }
	bool contains(unsigned i) const
	{ return i >= begin_ && i < end_; }
	unsigned begin() const
	{ return begin_; }
	unsigned end() const
	{ return end_; }
	bool lower(unsigned i) const
	{ return i < begin_; }
	bool lower_or_equal(unsigned i) const
	{ return i < end_; }
	unsigned size() const
	{
		return end_ - begin_;
	}
	static seedp_range all()
	{
		return seedp_range(0, Const::seedp);
	}
private:
	unsigned begin_, end_;
};

extern seedp_range current_range;

inline size_t partition_size(const shape_histogram &hst, unsigned p)
{
	size_t s = 0;
	for(unsigned i=0;i<hst.size();++i)
		s += hst[i][p];
	return s;
}

inline size_t hst_size(const shape_histogram &hst, const seedp_range &range)
{
	size_t s = 0;
	for(unsigned i=range.begin();i<range.end();++i)
		s += partition_size(hst, i);
	return s;
}
*/
struct Seqs_histogram
{
	Seqs_histogram();
	
	template<typename _filter>
	Seqs_histogram(const Sequence_set &seqs, const _filter *filter) :
		data_(shapes.count()),
		p_(seqs.partition(config.threads_))
	{
/*		vector<size_t> part = seqs.partition(config.threads_);
		for (size_t i = 0; i < p_.size(); ++i)
			p_[i] = i;
*/
		size_t seqs_size = seqs.get_length();
		for (unsigned s = 0; s < shapes.count(); ++s) {
			data_[s].resize(seqs_size);
			memset(data_[s].data(), 0, seqs_size *sizeof(unsigned)*Const::seedp);
		}

		Thread_pool threads;
		for (size_t i = 0; i < p_.size() - 1; ++i)
		{
			threads.push_back(launch_thread(seqs_worker<_filter>, &seqs, (unsigned)p_[i], (unsigned)p_[i + 1], &data_, filter));
		}
		threads.join_all();
	}

	const shape_histogram& get(unsigned sid) const
	{ return data_[sid]; }

	const vector<size_t>& get_partition() const
	{
		return p_;
	}

//	size_t max_chunk_size() const;


private:
	template<typename _filter>
	static void seqs_worker(const Sequence_set *seqs, unsigned begin, unsigned end, vector<shape_histogram> *data_, const _filter *filter)
	{
		for (unsigned i = begin; i < end; ++i)
		{
			Callback cb(i, *data_);
			seqs->enum_seeds(&cb, i, i+1, std::make_pair(0, shapes.count()), filter);
		}
	}

	struct Callback
	{
		Callback(size_t seqi, vector<shape_histogram> &data)
		{
			for (unsigned s = 0; s < shapes.count(); ++s)
				ptr.push_back(data[s][seqi].begin());
		}
		bool operator()(uint64_t seed, uint64_t pos, size_t shape)
		{
			++ptr[shape][seed_partition(seed)];
			return true;
		}
		void finish() const
		{
		}
		vector<unsigned*> ptr;
	};

	vector<shape_histogram> data_;
	vector<size_t> p_;
};

#endif /* SEED_HISTOGRAM_EX_H_ */