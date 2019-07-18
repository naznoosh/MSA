/*
#include "../util/ptr_vector.h"
#include "queries.h"
*/
#include "sorted_list_ex.h"

Sorted_seqs_seed::Sorted_seqs_seed()
{}

Sorted_seqs_seed::~Sorted_seqs_seed()
{
	for (size_t i = 0; i < seqs_entry.size(); i++)
	{
		if(seqs_entry[i] != NULL) delete [] seqs_entry[i];
	}
}

Sorted_seqs_seed::Sorted_seqs_seed(const Sequence_set *seqs, size_t sid, const Seqs_histogram &hst, Common_noise *filter) ://const _filter *filter) :
	seqs(seqs),
	sid(sid),
	noise(filter),
	seqs_limits(seqs->get_length()),
	seqs_entry(seqs->get_length())
{
	const shape_histogram & seqs_hst(hst.get((unsigned)sid));
	for (size_t i = 0; i < seqs_limits.size(); i++)
	{
		seqs_limits[i][0] = 0;
		for (size_t j = 0; j < Const::seedp; j++)
		{
			seqs_limits[i][j + 1] = seqs_limits[i][j] + seqs_hst[i][j];
		}
	}

	for (size_t i = 0; i < seqs_entry.size(); i++)
	{
		seqs_entry[i] = new entry[seqs_limits[i][Const::seedp]];
	}

	task_timer timer("Building seed list", 3);
	Thread_pool threads;
	const vector<size_t> &p = hst.get_partition();
	for (size_t i = 0; i < p.size() - 1; ++i)
	{
		threads.push_back(launch_thread(seqs_worker<Common_noise>, seqs, (unsigned)p[i], (unsigned)p[i + 1], &seqs_entry, &seqs_limits, filter));
	}
	threads.join_all();

	timer.go("Sorting seed list");
	for (size_t i = 0; i < seqs->get_length(); ++i)
	{
		Sort_context sort_context(seqs_entry[i], seqs_limits[i]);
		launch_scheduled_thread_pool(sort_context, Const::seedp, config.threads_);
	}
}


void Sorted_seqs_seed::Show()
{
	size_t total_common_seeds = noise->get_length(sid);
	size_t seqs_size = seqs->get_length();
	if (noise->is_too_frequent_seeds(sid)) {
		message_stream << "-- TOO FREQUENT SEEDS" << endl;
		return;
	}
	if (total_common_seeds == 0) {
		message_stream << "-- NO COMMON SEEDS WERE FOUND" << endl;
		return;
	}

	if (!config.verbose) {
		message_stream << "-- Number of common seeds: " << total_common_seeds << endl;
		return;
	}

	const size_t max_len = 64;
	const size_t chunk_size = 7;

	message_stream << "\nSeed";
	for (size_t i = 4; i < config.seed_pattern.size(); i++) message_stream << " ";
	message_stream << " Positions" << endl;
	for (size_t i = 0; i < config.seed_pattern.size(); i++) message_stream << "_";
	message_stream << " __________________________________________________________" << endl;

	vector<size_t> idx_cur(seqs_size);
	vector<size_t> idx_end(seqs_size);
	for (size_t i = 0, k = 0; i < Const::seedp && k < max_len; i++)
	{
		if (seqs_limits[0][i] == seqs_limits[0][i + 1]) continue;

		for (size_t s = 0; s < seqs_size; s++) idx_end[s] = seqs_limits[s][i + 1];

		while (idx_cur[0] < idx_end[0])
		{
			unsigned key = seqs_entry[0][idx_cur[0]].key;
			const char * p = seqs->ptr(0) + seqs_entry[0][idx_cur[0]].value.low;
			for (size_t j = 0; j < config.seed_pattern.size(); j++, p++)
			{
				if(config.seed_pattern[j] == '1') message_stream << to_char(*p);// input_value_traits.alphabet[*p];
				else                              message_stream << ' ';
			}
			message_stream << ' ';
			for (size_t s = 0; s < std::min(seqs_size, chunk_size); s++)
			{
				message_stream << "[" << seqs_entry[s][idx_cur[s]].value.low + 1;
				idx_cur[s]++;
				while (idx_cur[s] < idx_end[s] && seqs_entry[s][idx_cur[s]].key == key)
				{
					message_stream << "," << seqs_entry[s][idx_cur[s]].value.low + 1;
					idx_cur[s]++;
				}
				message_stream << "]";
			}
			if (seqs_size > 2 * chunk_size)	message_stream << "...";
			for (size_t s = std::max(seqs_size - chunk_size, chunk_size); s < seqs_size; s++)
			{
				message_stream << "[" << seqs_entry[s][idx_cur[s]].value.low + 1;
				idx_cur[s]++;
				while (idx_cur[s] < idx_end[s] && seqs_entry[s][idx_cur[s]].key == key)
				{
					message_stream << "," << seqs_entry[s][idx_cur[s]].value.low + 1;
					idx_cur[s]++;
				}
				message_stream << "]";
			}
			message_stream << endl;

			if (++k >= max_len) break;
		}
	}
	if (total_common_seeds > max_len)
	{
		message_stream << "." << endl << "[+" << total_common_seeds - max_len << " more common seeds]" << endl << "." << endl;
	}
}

/*

char* sorted_list::alloc_buffer(const Partitioned_histogram &hst)
{
	return new char[sizeof(entry) * hst.max_chunk_size()];
}

sorted_list::sorted_list()
{}

sorted_list::const_iterator sorted_list::get_partition_cbegin(unsigned p) const
{
	return const_iterator(cptr_begin(p), cptr_end(p));
}

sorted_list::iterator sorted_list::get_partition_begin(unsigned p) const
{
	return iterator(ptr_begin(p), ptr_end(p));
}

sorted_list::Random_access_iterator sorted_list::random_access(unsigned p, size_t offset) const
{
	return Random_access_iterator(cptr_begin(p) + offset, cptr_end(p));
}

entry* sorted_seqs_seed::ptr_begin(unsigned i) const
{
	return seqs_entry[i];
}

entry* sorted_seqs_seed::ptr_end(unsigned i) const
{
	return &data_[limits_[i + 1]];
}

const entry* sorted_seqs_seed::cptr_begin(unsigned i) const
{
	return &data_[limits_[i]];
}

const entry* sorted_seqs_seed::cptr_end(unsigned i) const
{
	return &data_[limits_[i + 1]];
}

sorted_seqs_seed::Ptr_set sorted_seqs_seed::build_iterators(const shape_histogram &hst) const
{
	Ptr_set iterators(hst.size());
	for (unsigned i = 0; i < Const::seedp; ++i)
	{
		iterators[0][i] = seqs_entry[i];
		for (unsigned j = 0; j < Const::seedp; ++j)
			iterators[i][j] = iterators[i - 1][j] + seqs_limits[i - 1][j];
	}
	return iterators;
}
*/