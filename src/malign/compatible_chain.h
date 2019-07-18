#pragma once
#ifndef COMPATIBLE_CHAIN_H
#define COMPATIBLE_CHAIN_H

#include <assert.h>
#include <vector>
#include <limits>
#include <list>
#include "../Diamond/basic/shape_config.h"
#include "../malign/common_seed.h"
#include "../malign/semaphor.h"
using std::list;

struct Compatible_chain
{
	friend struct Subseq_align;
	
	struct Common_seed
	{
		uint32_t operator [](size_t i) {
			return pos[i];
		}

		uint64_t seed = 0;
		uint32_t length = 0;
		vector<uint32_t> pos;
	};

	typedef vector<Common_seed> Chain;

private:
	typedef Sorted_common_seed::Common_seed Common_seed_;
	struct Common_seed_proxy
	{
		double get_score(vector <Common_seed_> &data)
		{
			return 1;
		}

		void set(Common_seed_ *right)
		{
			common_seed_list.push_back(right);
		}

		size_t merge(Common_seed_ *right)
		{
			if (common_seed_list.size() == 0) return 0;

			Common_seed_ *left = common_seed_list.back();

			if (right->pos(Sorted_common_seed::pivot_seq) <= left->pos(Sorted_common_seed::pivot_seq)) return 0;
		
			size_t offset = right->pos(Sorted_common_seed::pivot_seq) - left->pos(Sorted_common_seed::pivot_seq);
			if (offset > ::shapes[Sorted_common_seed::sid].length_) return false;

			for (size_t i = 0; i < Sorted_common_seed::seqs_size; i++)
			{
				if (offset != right->pos(i) - left->pos(i)) return false;
			}
			common_seed_list.push_back(right);
			return offset;
		}

		uint32_t get_length(size_t offset)
		{
			const size_t size = common_seed_list.size();
			if (size == 0) return 0;
			if (size == 1) return ::shapes[Sorted_common_seed::sid].length_;

//			return common_seed_list.back()->pos(0) - common_seed_list[0]->pos(0) + ::shapes[Sorted_common_seed::sid].length_;
			return common_seed_list[offset]->pos(0) - common_seed_list[0]->pos(0) + ::shapes[Sorted_common_seed::sid].length_;
		}

		vector<Common_seed_ *>common_seed_list;
	};

	struct Chain_
	{
		Common_seed_proxy& operator[](size_t i)
		{
			assert(i < length());
			i = length() - i;
			Chain_ *chain = this;
			for (size_t k = 1; k < i; k++)
			{
				chain = chain->tail[0];
			}
			return chain->head;
		}

		size_t length()
		{
			size_t len = 1;
			Chain_ *chain = this;
			while (chain->tail.size() != 0)
			{
				len++;
				chain = chain->tail[0];
			}
			return len;
		}
/*
		size_t size()
		{
			size_t size = 0;
			for (size_t i = 0; i < left.size(); i++)
			{
				size += left[i]->size();
			}
			return size;
		}
*/
		vector <Chain_ *> tail;
		Common_seed_proxy head;
		double size = 1;
		float score = 1;
		uint16_t best_score_idx = 0;
		vector <size_t> offset;
	};

public:
	Compatible_chain(Sorted_common_seed &sorted_common_seed, size_t sid);
	~Compatible_chain()
	{
		if (_best_longest_chain != NULL) delete  _best_longest_chain;
		if (_best_chain != _best_longest_chain) delete _best_chain;
/*
		for (size_t k = 0; k < chain_.size(); k++)
		{
				delete [] chain_[k];
		}
*/
	}
	
	void Show();

	const Chain *best_chain() {
		return _best_chain;
	}

	const Chain *best_longest_chain() {
		return _best_longest_chain;
	}

/*
	void best_chain(vector <Common_seed_proxy> &chain)
	{
		extract_chain(best_chain_, chain);
	}
	
	void best_longest_chain(vector <Common_seed_proxy> &chain)
	{
		vector <Chain_ *> longest = matrix.back();
		Chain_ *ptr = longest[0];
		size_t size = longest.size();

		for (size_t i = 1; i < size; i++)
		{
			if (longest[i]->score < ptr->score) continue;
			if (longest[i]->score > ptr->score || longest[i]->size > ptr->size) ptr = longest[i];
		}

		extract_chain(ptr, chain);
	}
*/
//	static size_t sid;

private:
	Chain_ * best_longest_chain_()
	{
/*		list <Chain_ *> longest = matrix.back();
		Chain_ *best_longest = longest.front();
		size_t size = longest.size();

		for (size_t i = 1; i < size; i++)
		{
			if (longest[i]->score < best_longest->score) continue;
			if (longest[i]->score > best_longest->score || longest[i]->size > best_longest->size) best_longest = longest[i];
		}

		return best_longest;*/
		if (matrix.front().size() == 0) return &dummy_chain_;

		auto last_row = ++matrix.rbegin();
		Chain_ *best_longest = last_row->front();
		for (auto col = last_row->begin(); col != last_row->end(); col++) {
			if ((*col)->score < best_longest->score) continue;
			if ((*col)->score > best_longest->score || (*col)->size > best_longest->size) best_longest = *col;
		}
		return best_longest;
	}
/*
	void extract_chain(Chain_ *ptr, vector <Common_seed_proxy> &chain)
	{
		size_t size = ptr->length();
		chain.clear();
		chain.resize(size);

		for (size_t i = 1; i < size; i++)
		{
			chain[size - i] = ptr->head;
			ptr = ptr->tail[ptr->best_score_idx];
		}
		chain[0] = ptr->head;
	}
*/
	void _extract_chain(Chain_ *ptr, Chain &chain)
	{
		chain.clear();
		if (ptr == &dummy_chain_ || ptr == NULL) return;

		const size_t size = ptr->length();
		chain.resize(size);
		size_t offset = ptr->head.common_seed_list.size() - 1;
		for (size_t i = 1; i <= size; i++)
		{
			chain[size - i].seed = ptr->head.common_seed_list[0]->seed();
			chain[size - i].pos.resize(Sorted_common_seed::seqs_size);
			for (size_t k = 0; k < Sorted_common_seed::seqs_size; k++)
			{
				chain[size - i].pos[k] = ptr->head.common_seed_list[0]->pos(k);
			}
			chain[size - i].length = ptr->head.get_length(offset);
			if (i < size) {
				offset = ptr->offset[ptr->best_score_idx];
				ptr = ptr->tail[ptr->best_score_idx];
			}
		}
	}

	struct Chain_context
	{
		list <list <Chain_ *>> *matrix;
		vector <Chain_ *> *chain_;
		uint16_t *chain_idx;
		Chain_ **best_chain_;
		list <tthread::mutex *> *mxs;
		tthread::mutex mx;
		semaphore sem;
	};

	static void chain_worker(Chain_context *context, Common_seed_ *common_seed_ptr, volatile bool *done);

	static const size_t chunk_size = std::numeric_limits<uint16_t>::max() + 1;
	static const size_t block_size = 7;

	vector <Chain_ *> chain_;
	uint16_t chain_idx = 0;
	Chain_ dummy_chain_;
	Chain_ *best_chain_ = &dummy_chain_;
	list <list <Chain_ *>> matrix;
	Chain *_best_longest_chain = NULL;
	Chain *_best_chain = NULL;
	vector<double> chain_info;
};

#endif // COMPATIBLE_CHAIN_H
