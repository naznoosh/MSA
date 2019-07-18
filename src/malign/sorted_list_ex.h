#ifndef SORTED_LIST_EX_H_
#define SORTED_LIST_EX_H_
/*
#include "../util/util.h"
#include "seed_histogram.h"
#include "../basic/packed_loc.h"
#include "../util/system.h"

#pragma pack(1)
*/
#include "../Diamond/data/sorted_list.h"
#include "../Diamond/util/ptr_vector.h"
#include "../malign/seed_histogram_ex.h"
#include "../malign/common_noise.h"

inline Packed_seed rebuild_seed(unsigned seedp, unsigned seedo)
{
	return ((Packed_seed)seedo << Const::seedp_bits) | seedp;
}


typedef sorted_list::entry entry;

struct Sorted_seqs_seed
{
	friend struct Sorted_common_seed;

	typedef vector<Array<size_t, Const::seedp + 1>> Seqs_limits;
/*
	typedef Packed_loc _pos;

	struct entry
	{
		entry():
			key (),
			value ()
		{ }
		entry(unsigned key, _pos value):
			key (key),
			value (value)
		{ }
		bool operator<(const entry &rhs) const
		{ return key < rhs.key; }
		unsigned	key;
		_pos		value;
	} PACKED_ATTRIBUTE ;
*/
//	static char* alloc_buffer(const Seqs_histogram &hst);
	Sorted_seqs_seed();
	~Sorted_seqs_seed();

//	template<typename _filter>
	Sorted_seqs_seed(const Sequence_set *seqs, size_t sid, const Seqs_histogram &hst, Common_noise *filter);//const _filter *filter) :

	void Show();
private:
	template<typename _filter>
	static void seqs_worker(const Sequence_set *seqs, unsigned begin, unsigned end, vector<entry *> *seqs_entry, Seqs_limits *seqs_limits, const _filter *filter)
	{
		for (unsigned i = begin; i < end; ++i)
		{
			Build_callback cb((*seqs_entry)[i], (*seqs_limits)[i], seqs->position(i, 0));
			seqs->enum_seeds(&cb, i, i + 1, std::make_pair(0, shapes.count()), filter);
		}
/*
		Ptr_vector<Build_callback> cb;
		for (size_t i = 0; i < seqs.get_length(); ++i)
			cb.push_back(new Build_callback(seqs_entry[i], seqs_limits[i], seqs.position(i, 0)));
		seqs.enum_seeds(cb, hst.get_partition(), sid, sid + 1, filter);
*/
	}


	friend struct Build_callback;

	typedef vector<Array<entry*, Const::seedp> > Ptr_set;

	struct Buffered_iterator
	{
		static const unsigned BUFFER_SIZE = 16;
		Buffered_iterator(entry* entry_begin, Array<size_t, Const::seedp + 1>& seq_limits)
		{
			memset(n, 0, sizeof(n));
			for (size_t i = 0; i < Const::seedp; i++)
			{
				ptr[i] = entry_begin + seq_limits[i];
			}
		}
		void push(uint64_t key, Loc value)
		{
			const unsigned p(seed_partition(key));
			assert(n[p] < BUFFER_SIZE);
			buf[p][n[p]++] = entry (seed_partition_offset(key), value);
			if(n[p] == BUFFER_SIZE)
				flush(p);
		}
		void flush(unsigned p)
		{
			memcpy(ptr[p], buf[p], n[p] * sizeof(entry));
			ptr[p] += n[p];
			n[p] = 0;
		}
		void flush()
		{
			for(unsigned p=0;p<Const::seedp;++p)
				if(n[p] > 0)
					flush(p);
		}
		entry*   ptr[Const::seedp];
		entry    buf[Const::seedp][BUFFER_SIZE];
		uint8_t  n[Const::seedp];
	};

	struct Sort_context
	{
		Sort_context(entry* entry_begin, Array<size_t, Const::seedp + 1>& seq_limits):
			entry_begin (entry_begin),
			seq_limits (seq_limits)
		{ }
		void operator()(unsigned thread_id ,unsigned seedp) const
		{ 
//			std::sort(entry_begin + seq_limits[seedp], entry_begin + seq_limits[seedp + 1]);
			std::stable_sort(entry_begin + seq_limits[seedp], entry_begin + seq_limits[seedp+1]);
		}
		
		entry* entry_begin;
		Array<size_t, Const::seedp + 1>& seq_limits;
	};

	struct Build_callback
	{
		Build_callback(entry* entry_begin, Array<size_t, Const::seedp + 1>& seq_limits, size_t pos_offset) :
			it(new Sorted_seqs_seed::Buffered_iterator(entry_begin, seq_limits)),
			offset (pos_offset)
		{ }
		bool operator()(uint64_t seed, uint64_t pos, size_t shape)
		{
			it->push(seed, pos-offset);
			return true;
		}
		void finish()
		{
			it->flush();
		}
		auto_ptr<Sorted_seqs_seed::Buffered_iterator> it;
		size_t offset;
	};

	Seqs_limits seqs_limits;
	vector<entry *> seqs_entry;
	const Sequence_set *seqs;
	Common_noise *noise;
	size_t sid;
};

#endif /* SORTED_LIST_EX_H_ */
