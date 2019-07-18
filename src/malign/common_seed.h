#pragma once
#ifndef COMMON_SEED_H
#define COMMON_SEED_H

#include <vector>
#include "../malign/sorted_list_ex.h"
#include "../malign/common_noise.h"

using std::vector;


//typedef vector<uint64_t> Position_list;
//typedef uint8_t *Common_seed_position_index;
/*
struct Position_list_proxy
{
	Position_list_proxy() {}
	Position_list_proxy(entry * ptr, size_t size) : ptr_(ptr), size_(size) {}
	uint64_t operator [](size_t i) const
	{
		assert(i >= 0 && i < size_);
		return ptr_[i].value;
	}

	size_t size() const { return size_; }

private:
	size_t size_;
	entry * ptr_;
};

struct Common_seed_info
{
	Common_seed_info() {}
	Common_seed_info(uint64_t seed, Position_list_proxy *pos) :
		seed(seed), pos(pos)
	{}

	uint64_t seed;
	Position_list_proxy * pos;
};
*/
struct Sorted_common_seed
{
	struct Communal_seed_proxy
	{
		friend struct Iterator;

		Communal_seed_proxy(Sorted_seqs_seed *seqs, uint32_t *data) : seqs(seqs), data_(data) {}
		entry& operator [](size_t i) const
		{
			assert(i >= 0 && i < seqs->seqs->get_length());
			return seqs->seqs_entry[i][data_[i]];
		}
		uint32_t * data() { return data_; }
//	private:
		uint32_t * data_;
		Sorted_seqs_seed *seqs;
	};

	struct Communal_list_proxy
	{
//		Communal_list_proxy() {}
		Communal_list_proxy(Sorted_seqs_seed *seqs) : cols_(seqs->seqs->get_length()), seqs(seqs) {}
		~Communal_list_proxy()
		{
			clear();
		}
		Communal_seed_proxy operator [](size_t i) const
		{
			assert(i >= 0 && i < size_);
			return Communal_seed_proxy(seqs, &data_[i*cols_]);
		}

		std::pair<size_t, size_t> size() const { return std::make_pair(size_, cols_); }
		void resize(size_t size)
		{
			if (size < size_) return;
			if (data_) delete[] data_;
			data_ = new uint32_t[cols_*size];
			size_ = size;
		}
		void clear()
		{
			if (data_) delete[] data_;
			data_ = NULL;
		}
		uint32_t * data() { return data_; }

	private:
		size_t size_ = 0;
		uint32_t * data_ = NULL;
		const size_t cols_ = 0;
		Sorted_seqs_seed *seqs;
	};

	struct Communal_seed_index
	{
		Communal_seed_index() {}
		Communal_seed_index(uint32_t pos, uint16_t freq, uint16_t seedp, size_t index) :
			pos(pos), freq(freq), seedp(seedp), index(index) {}

		bool operator < (const Communal_seed_index &right)
		{
			return pos < right.pos;
		}

		uint32_t pos;
		uint16_t freq;
		uint16_t seedp;
		size_t index;
	};

	struct Common_seed
	{
		Common_seed() {}
		Common_seed(uint64_t seed, uint32_t *pos_list) :
			seed_(seed),
			pos_(pos_list)
		{}

		uint32_t pos(size_t i) const
		{
			assert(i >= 0 && i < seqs_size);
			return pos_[i];
		}

		uint64_t seed() { return seed_; }

		bool operator >= (Common_seed const& right) const
		{
			//		if (pos.size() != commonSeed.pos.size()) return false;
			for (size_t k = 0; k < Sorted_common_seed::seqs_size; k++)
			{
				if (pos(k) < right.pos(k) + ::shapes[sid].length_) return false;
			}
			return true;
		}

		bool operator < (Common_seed const& right) const
		{
			return !(*this >= right);
		}

		uint64_t seed_ = 0;
		uint32_t *pos_ = NULL;
	};

	~Sorted_common_seed()
	{
		if (pos_list != NULL) delete[] pos_list;
	}
	Sorted_common_seed(Sorted_seqs_seed &seqs, size_t sid, Seqs_histogram &hst, Common_noise &noise); //const Noise_filter *filter);

//	void Show();

	static void seed_worker(const Sorted_seqs_seed *seqs, size_t begin, size_t end, uint32_t *communal_idx, size_t list_idx, Communal_seed_index *index_ptr);
	static void common_seed_worker(Communal_list_proxy *communal_list, Common_seed *common_seed, Communal_seed_index *index_, uint32_t *pos_list, size_t begin, size_t end);

//	vector <Common_seed_info> data_;
	Communal_list_proxy communal_list;
	vector <Communal_seed_index> index_;
//	size_t min_pos_idx = 0;
	size_t total_common_seeds = 0;
	static size_t seqs_size;
	static size_t pivot_seq;
	static size_t sid;
	vector <Common_seed> common_seed;
	uint32_t *pos_list = NULL;

	struct Iterator
	{
		Iterator(Communal_seed_proxy communal_seed, uint32_t pivot_pos, uint16_t seedp, uint32_t *pos_list) :
			communal_seed(communal_seed),
			pos_list(pos_list),
			pivot_pos(pivot_pos),
			pos_index(seqs_size),
			size(seqs_size)
		{
			unsigned key = communal_seed[0].key;
			for (size_t s = 0; s < seqs_size; s++)
			{
				size_t idx_cur = communal_seed.data()[s];
				size_t idx_end = communal_seed.seqs->seqs_limits[s][seedp + 1];
				size_t idx = idx_cur;
				entry *entry_cur = &communal_seed[s]; //&communal_seed.seqs->seqs_entry[s][idx_cur];
				while (idx < idx_end && entry_cur->key == key)
				{
					idx++;
					entry_cur++;
				}
				size[s] = idx - idx_cur;
			}
		}

		bool good()
		{
			return !done;
		}

		void next()
		{
			for (size_t k = 0; k < seqs_size; k++)
			{
				if (k == pivot_seq)	continue;
				
				if (pos_index[k] >= size[k]-1)
				{
					pos_index[k] = 0;
					continue;
				}
				pos_index[k]++;
				return;
			}
			done = true;
		}

		uint32_t *get()
		{
			uint32_t *ret = pos_list;
			uint32_t *idx_cur = communal_seed.data();

			for (size_t s = 0; s < seqs_size; s++, pos_list++)
			{
				*pos_list = communal_seed.seqs->seqs_entry[s][pos_index[s]+ idx_cur[s]].value.low;
			}
			ret[pivot_seq] = pivot_pos;
			return ret;
		}

		Communal_seed_proxy communal_seed;
		uint32_t *pos_list;
		uint32_t pivot_pos;
		vector<size_t> pos_index;
		vector<size_t> size;
		bool done = false;
	};
};

#endif // !COMMON_SEED_H

