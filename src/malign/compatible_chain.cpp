#include <condition_variable>
#include "compatible_chain.h"
#include "progress.h"

//size_t Compatible_chain::sid;

Compatible_chain::Compatible_chain(Sorted_common_seed &sorted_common_seed, size_t sid)
	: matrix(2)
{
//	Compatible_chain::sid = sid;
//	vector <vector <CompatibleChain>> compatibleChains(1);
/*
	size_t data_size = 0;
	for (size_t k = 0; k < sorted_common_seed.index_.size(); k++)
	{
		Sorted_common_seed::Iterator it(sorted_common_seed.data_[sorted_common_seed.index_[k].index], sorted_common_seed.index_[k].pos, sorted_common_seed.min_pos_idx);
		for (; it.good(); it.next())
		{
			data_size++;
		}
	}
*/

	Progress prog(sorted_common_seed.total_common_seeds);

	list <tthread::mutex *> mxs;
	mxs.push_back(new tthread::mutex());
	mxs.push_back(new tthread::mutex());
	Chain_context context = {
		&matrix,
		&chain_,
		&chain_idx,
		&best_chain_,
		&mxs
	};

	struct Chain_thread_info  
	{
		volatile bool done = true;
		thread* thread_;
	};

	vector <Chain_thread_info> chain_threads_info(config.threads_);

	for (size_t k = 0; k < sorted_common_seed.total_common_seeds; k++)
	{
		prog.progress(k);

		bool done = false;
		auto mx = ++mxs.rbegin();
		while (!done) {
			size_t i = 0;
			for (; i < config.threads_; i++) {
				if (chain_threads_info[i].done) break;
			}
			if (i < config.threads_) {
				if (chain_threads_info[i].thread_ != NULL) {
					chain_threads_info[i].thread_->join();
					delete chain_threads_info[i].thread_;
				}
				chain_threads_info[i].done = false;
				done = true;
				mx = ++mxs.rbegin();
				(*mx)->lock();
				(*mx)->unlock();

				if ((matrix.size() - 1) / block_size >= mxs.size() - 1)	mxs.push_back(new tthread::mutex());

				mx = ++mxs.rbegin();

				if (config.threads_ > 1) {
					chain_threads_info[i].thread_ = launch_thread(chain_worker, &context, &sorted_common_seed.common_seed[k], &chain_threads_info[i].done);
					context.sem.wait();
				}
				else chain_worker(&context, &sorted_common_seed.common_seed[k], &chain_threads_info[i].done);
			}
			else {
				(*mx)->lock();
				(*mx)->unlock();
				mx++;
			}
		}
	}

	for (size_t i = 0; i < config.threads_; i++) {
		if (chain_threads_info[i].thread_ != NULL) {
			chain_threads_info[i].thread_->join();
			delete chain_threads_info[i].thread_;
		}
	}

	for (auto mx = mxs.begin(); mx != mxs.end(); mx++) {
		delete *mx;
	}
	mxs.clear();

/*
	for (uint32_t k = 0; k < sorted_common_seed.total_common_seeds; k++)
	{
		prog.progress(k);

		bool chained = false;

		Common_seed_ *common_seed_ptr = &sorted_common_seed.common_seed[k];
		for (auto row = ++matrix.rbegin(), prev_row = row; row != matrix.rend(); prev_row = row++)
		{
			for (auto col = row->begin(); col != row->end(); col++)
			{
				size_t offset;
				if (offset = (*col)->head.merge(common_seed_ptr))
				{
					(*col)->score += (1.0f + offset) / ::shapes[Sorted_common_seed::sid].length_;
					if ((*col)->score > best_chain_->score) best_chain_ = *col;
					chained = true;
					break;
				}
			}
			if (chained) break;

			vector <Chain_ *> children(row->size());
			size_t children_size = 0;
			for (auto col = row->begin(); col != row->end(); col++)
			{
				if (*common_seed_ptr >= *(*col)->head.common_seed_list.back())
				{
					children[children_size++] = *col;
				}
			}
			if (children_size > 0)
			{
				if (chain_idx == 0) chain_.push_back(new Chain_[chunk_size]);

				Chain_ &chain = chain_.back()[chain_idx];
				chain.head.set(common_seed_ptr);
				chain.size = 0;
				chain.tail.resize(children_size);
				for (uint16_t n = 0; n < children_size; n++)
				{
					Chain_ *child = children[n];
					chain.tail[n] = child;
					chain.size += child->size;
					if (child->score > chain.score)
					{
						chain.score = child->score;
						chain.best_score_idx = n;
					}
				}
				chain.score += 1;
				if (chain.score > best_chain_->score) best_chain_ = &chain;

				if (row == ++matrix.rbegin())
				{
					matrix.back().push_back(&chain);
					matrix.push_back(list <Chain_ *>());
				}
				else prev_row->push_back(&chain);

				chain_idx++;
				chained = true;
				break;
			}
		}

		if (!chained)
		{
			if (chain_idx == 0) chain_.push_back(new Chain_[chunk_size]);
			Chain_ &chain = chain_.back()[chain_idx];
			chain.head.set(common_seed_ptr);
			matrix.front().push_back(&chain);
			chain_idx++;
		}
	}
*/
	_best_longest_chain = new Chain;
	_extract_chain(best_longest_chain_(), *_best_longest_chain);
	if (best_chain_ != best_longest_chain_()) {
		_best_chain = new Chain;
		_extract_chain(best_chain_, *_best_chain);
	}
	else _best_chain = _best_longest_chain;

	prog.finish();

	chain_info.resize(matrix.size()-1);
/*	for (size_t k = 0; k < matrix.size(); k++) {
		uint64_t size = 0;
		for (size_t j = 0; j < matrix[k].size(); j++) {
			size += matrix[k][j]->size;
		}
		chain_info[k] = size;

//		matrix[k].clear();
	}
*/
	double *chain_info_ptr = &chain_info[chain_info.size() - 1];
	for (auto row = ++matrix.rbegin(); row != matrix.rend(); row++, chain_info_ptr--) {
		double size = 0;
		for (auto col = row->begin(); col != row->end(); col++) {
			size += (*col)->size;
		}
		*chain_info_ptr = size;
		row->clear();
	}
	matrix.clear();

	for (size_t k = 0; k < chain_.size(); k++) {
		for (size_t j = 0; j < chunk_size; j++) {
			chain_[k][j].head.common_seed_list.clear();
		}
		delete[] chain_[k];
	}
}


void Compatible_chain::Show()
{
	if (chain_info[0] == 0)	{
		message_stream << "-- NO COMPATIBLE CHAIN WAS FOUND" << endl;
		return;
	}

	if (!config.verbose) {
		message_stream << "-- Compatible chain length: " << chain_info.size() << endl;
		return;
	}

	const int chunk = 7;
	const int blocks = 5;
	int size = (int)chain_info.size();
	const int step = (int)std::ceil((double)(size - chunk) / (blocks - 1));
	message_stream << "\nChain" << endl;
	message_stream << "\nLength  Numbers" << endl;
	message_stream <<   "______  ________________" << endl;
	for (int k = 0, begin = 0, end = std::min(size, chunk); k < blocks;
		k++, begin = std::max(end, std::min(size - chunk, k * step)), end = std::min(size, begin + chunk))
	{
		for (size_t i = begin; i < end; i++) {
			message_stream << i + 1 << '\t' << chain_info[i] << endl;
		}

		if (size > blocks * chunk && k < blocks - 1 && end - begin <= chunk) message_stream << ".       ." << endl;
	}
}

void Compatible_chain::chain_worker(Chain_context *context, Common_seed_ *common_seed_ptr, volatile bool *done)
{
	list <list <Chain_ *>> &matrix = *context->matrix;
	vector <Chain_ *> &chain_ = *context->chain_;
	uint16_t &chain_idx = *context->chain_idx;
	Chain_ *&best_chain_ = *context->best_chain_;
	tthread::mutex &mx = context->mx;
	
	auto block_mx = context->mxs->rbegin(), prev_block_mx = block_mx++;
	(*block_mx)->lock();
	context->sem.notify();

	size_t block_idx = (matrix.size() - 1) % block_size + 1;
	bool chained = false, first_row = true;;
	for (auto row = ++matrix.rbegin(), prev_row = row; row != matrix.rend(); prev_row = row++)
	{
		if (--block_idx == 0)
		{
			prev_block_mx = block_mx++;
			(*block_mx)->lock();
		}

		if (first_row) {
			for (auto col = row->begin(); col != row->end(); col++)
			{
				size_t offset;
				if (offset = (*col)->head.merge(common_seed_ptr))
				{
					(*col)->score += (1.0f + offset) / ::shapes[Sorted_common_seed::sid].length_;

					mx.lock();
					if ((*col)->score > best_chain_->score) best_chain_ = *col;
					mx.unlock();

					chained = true;
					break;
				}
			}
			if (chained) break;
			first_row = false;
		}

		vector <Chain_ *> children(row->size());
		vector <size_t> offset(row->size());
		size_t children_size = 0;
		for (auto col = row->begin(); col != row->end(); col++)
		{
			if (*common_seed_ptr >= *(*col)->head.common_seed_list.back())
			{
				children[children_size] = *col;
				offset[children_size++] = (*col)->head.common_seed_list.size() - 1;
			}
			else if ((*col)->head.common_seed_list.size() > 1 && *common_seed_ptr >= *(*col)->head.common_seed_list.front())
			{
				children[children_size] = *col;
				size_t begin = 1, end = (*col)->head.common_seed_list.size() - 2;
				while (begin <= end)
				{
					size_t mid = (begin + end) / 2;
					if (*common_seed_ptr >= *(*col)->head.common_seed_list[mid]) begin = mid + 1;
					else                                                         end = mid - 1;
				}
				offset[children_size++] = begin - 1;
			}
		}
		if (children_size > 0)
		{
			mx.lock();
			if (chain_idx == 0) chain_.push_back(new Chain_[chunk_size]);

			Chain_ &chain = chain_.back()[chain_idx++];
			mx.unlock();

			chain.head.set(common_seed_ptr);
			chain.size = 0;
			chain.tail.resize(children_size);
			chain.offset.resize(children_size);
			for (uint16_t n = 0; n < children_size; n++)
			{
				Chain_ *child = children[n];
				chain.tail[n] = child;
				chain.offset[n] = offset[n];
				chain.size += child->size;
				float score = child->score - (child->head.common_seed_list.size() - 1 - offset[n] + 
					                          child->head.common_seed_list.back()->pos(0) -
					                          child->head.common_seed_list[offset[n]]->pos(0)) / ::shapes[Sorted_common_seed::sid].length_;
				if (/*child->*/score > chain.score)
				{
					chain.score = /*child->*/score;
					chain.best_score_idx = n;
				}
			}
			chain.score += 1;

			mx.lock();
			if (chain.score > best_chain_->score) best_chain_ = &chain;
			mx.unlock();

			if (row == ++matrix.rbegin())
			{
				matrix.back().push_back(&chain);

				matrix.push_back(list <Chain_ *>());
			}
			else prev_row->push_back(&chain);

			chained = true;
			break;
		}

		if (block_idx == 0)
		{
			block_idx = block_size;
			(*prev_block_mx)->unlock();
		}
	}

	if (!chained)
	{
		mx.lock();
		if (chain_idx == 0) chain_.push_back(new Chain_[chunk_size]);
		Chain_ &chain = chain_.back()[chain_idx++];
		mx.unlock();
		chain.head.set(common_seed_ptr);
		matrix.front().push_back(&chain);
	}
	*done = true;
	if (block_idx == 0)	(*prev_block_mx)->unlock();
	(*block_mx)->unlock();
}
