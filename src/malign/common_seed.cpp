#include "../malign/common_seed.h"

//const size_t too_frequent = 1024;
Noise_filter no_noise_filter;
//Default_noise_filter default_noise_filter;
size_t Sorted_common_seed::seqs_size = 0;
size_t Sorted_common_seed::pivot_seq = 0;
size_t Sorted_common_seed::sid = 0;

//Sorted_common_seed::Sorted_common_seed(Sorted_seqs_seed &seqs, size_t sid, const Noise_filter *filter) 
Sorted_common_seed::Sorted_common_seed(Sorted_seqs_seed &seqs, size_t sid, Seqs_histogram &hst, Common_noise &noise) :
	communal_list(&seqs)
{
	seqs_size = seqs.seqs_limits.size();
	Sorted_common_seed::sid = sid;

//	vector <entry *> ptr(seqs_size);
//	vector <entry *> ptr_end(seqs_size);
	size_t total_size = noise.get_length(sid);
//	for (unsigned i = 0; i < Const::seedp; i++) total_size += common_seed_hst[i];
	for (size_t i = 0; i < total_size; i++) total_common_seeds += noise.get_frequency(sid, i);
	if (total_common_seeds == 0) return;

	uint64_t min_pos_size = -1;
	for (size_t k = 0; k < seqs_size; k++)
	{
		uint64_t pos_size = 0;
		const unsigned *ptr_pos_hst = hst.get((unsigned)sid)[k].data_; //common_seed_pos_hst[k];
		for (size_t i = 0; i < Const::seedp; i++) pos_size += *ptr_pos_hst++;// common_seed_pos_hst[k][i];

		if (pos_size < min_pos_size)
		{
			min_pos_size = pos_size;
			pivot_seq = k;
		}
	}


//	data_.resize(total_size);

	communal_list.resize(total_size);
	index_.resize(min_pos_size + 1);

	vector<size_t> p_range(config.threads_ + 1);
	for (size_t i = 1, r = Const::seedp % config.threads_; r > 0; i++, r--) p_range[i]++;
	for (size_t i = 1, m = Const::seedp / config.threads_; i <= config.threads_; i++) p_range[i] += p_range[i - 1] + m;

	vector<size_t> p_list(config.threads_);
	entry *entry_cur = &seqs.seqs_entry[pivot_seq][0];
	for (size_t k = 1; k < config.threads_; k++)
	{
		size_t idx_cur = seqs.seqs_limits[pivot_seq][p_range[k-1]];
		size_t num = 0;
		for (size_t i = p_range[k-1]; i < p_range[k]; i++)
		{
			size_t idx_end = seqs.seqs_limits[pivot_seq][i + 1];
			while (idx_cur < idx_end)
			{
				num++;
				unsigned key = entry_cur->key;
				while (idx_cur < idx_end && entry_cur->key == key)
				{
					idx_cur++;
					entry_cur++;
				}
			}
		}
		p_list[k] = p_list[k-1] + num;
	}

	Thread_pool threads;
	for (size_t i = 0; i < config.threads_; ++i)
	{
		if (p_list[i] < total_size) {
			threads.push_back(launch_thread(seed_worker, &seqs, p_range[i], p_range[i + 1], communal_list[p_list[i]].data(), p_list[i], &index_[seqs.seqs_limits[pivot_seq][p_range[i]]]));
//			seed_worker(&seqs, p[i], p[i + 1], communal_list[seed_p[i]].data(), seed_p[i], &index_[seqs.seqs_limits[pivot_seq][p[i]]]);
		}
	}
	threads.join_all();

	std::sort(&index_[0], &index_[index_.size() - 1]);

	for (size_t k = 1, i = 0, sm = 0, chunk = total_common_seeds / config.threads_; k < config.threads_; k++)
	{
		while(sm < chunk) sm += index_[i++].freq;
		p_list[k] = sm;
		p_range[k] = i;
		chunk += total_common_seeds / config.threads_;
	}
	p_range[config.threads_] = min_pos_size;

	common_seed.resize(total_common_seeds);
	pos_list = new uint32_t[total_common_seeds*seqs_size];
/*
	size_t seed_pos_idx = 0;
	for (size_t k = 0; k < index_.size(); k++)
	{
		uint64_t seed = rebuild_seed(index_[k].seedp, communal_list[index_[k].index][0].key);
		Iterator it(communal_list[index_[k].index], index_[k].pos, index_[k].seedp, &pos_list[seed_pos_idx*seqs_size]);
		for (; it.good(); it.next())
		{
			common_seed[seed_pos_idx++] = Common_seed(seed, it.get());
		}
	}
*/
	threads.clear();
	for (size_t i = 0; i < config.threads_; ++i)
	{
		threads.push_back(launch_thread(common_seed_worker, &communal_list, &common_seed[p_list[i]], &index_[p_range[i]], &pos_list[p_list[i] * seqs_size], p_range[i], p_range[i + 1]));
//		common_seed_worker(&communal_list, &common_seed[p_list[i]], &index_[p_range[i]], &pos_list[p_list[i]*seqs_size], p_range[i], p_range[i + 1]);
	}
	threads.join_all();

	communal_list.clear();
	index_.clear();
}


void Sorted_common_seed::seed_worker(const Sorted_seqs_seed *seqs, size_t begin, size_t end, uint32_t *communal_idx, size_t list_idx, Communal_seed_index *index_ptr)
{
	size_t seqs_size = seqs->seqs_limits.size();
	vector<size_t> idx_cur(seqs_size);
	vector<size_t> idx_end(seqs_size);
	for (size_t s = 0; s < seqs_size; s++)
	{
		idx_cur[s] = seqs->seqs_limits[s][begin];
	}
	for (size_t i = begin; i < end; i++)
	{
		if (seqs->seqs_limits[0][i] == seqs->seqs_limits[0][i + 1]) continue;

		for (size_t s = 0; s < seqs_size; s++)
		{
			idx_end[s] = seqs->seqs_limits[s][i + 1];
		}

		while (idx_cur[0] < idx_end[0])
		{
			unsigned key = seqs->seqs_entry[0][idx_cur[0]].key;
//			Position_list_proxy *common_seed_pos = common_seed_pos_ptr;
			size_t freq = 1, num = 0;
			entry *entry_piv = NULL;
			for (size_t s = 0; s < seqs_size; s++, communal_idx++)
			{
				size_t idx = idx_cur[s];
				*communal_idx = (uint32_t)idx;
				size_t idx_end_s = idx_end[s];
				entry *entry_cur = &seqs->seqs_entry[s][idx_cur[s]];
				if (s == pivot_seq) entry_piv = entry_cur;
				while (idx < idx_end_s && entry_cur->key == key)
				{
					idx++;
					entry_cur++;
				}
				if (s != pivot_seq) freq *= idx - idx_cur[s];
				else                num = idx - idx_cur[s];
				idx_cur[s] = idx;
			}
			for (size_t k = 0; k < num; k++, index_ptr++)
			{
				*index_ptr = Communal_seed_index(entry_piv[k].value.low, (uint16_t)freq, (uint16_t)i, list_idx);
			}
			list_idx++;
		}
	}
}

void Sorted_common_seed::common_seed_worker(Communal_list_proxy *communal_list, Common_seed *common_seed, 
	Communal_seed_index *index_, uint32_t *pos_list, size_t begin, size_t end)
{
	for (size_t k = begin; k < end; k++, index_++)
	{
		uint64_t seed = rebuild_seed(index_->seedp, (*communal_list)[index_->index][0].key);
		Iterator it((*communal_list)[index_->index], index_->pos, index_->seedp, pos_list);
		for (; it.good(); it.next())
		{
			*common_seed++ = Common_seed(seed, it.get());
			pos_list += seqs_size;
		}
	}
}

/*
void Sorted_common_seed::Show()
{
	if (!config.verbose)
	{
		message_stream << "-- Number of common seeds: " << data_.size() << endl;
		return;
	}
	const size_t max_len = 64;
	const size_t chunk_size = 8;
	message_stream << "\nSeed:\tPositions" << endl;
	message_stream << "_____\t__________________________________________________________" << endl;

	for (size_t i = 0; i < std::min(data_.size(), max_len); i++)
	{
		message_stream << data_[i].seed << ": ";
		for (unsigned s = 0; s < std::min(seqs_size, chunk_size); s++)
		{
			message_stream << "[" << data_[i].pos[s][0];
			for (unsigned k = 1; k < data_[i].pos[s].size(); k++)
			{
				message_stream << "," << data_[i].pos[s][k];
			}
			message_stream << "]";
		}
		
		if (seqs_size > 2 * chunk_size)	message_stream << "...";
		
		for (size_t s = std::max(seqs_size - chunk_size, chunk_size); s < seqs_size; s++)
		{
			message_stream << "[" << data_[i].pos[s][0];
			for (unsigned k = 1; k < data_[i].pos[s].size(); k++)
			{
				message_stream << "," << data_[i].pos[s][k];
			}
			message_stream << "]";
		}
		message_stream << endl;
	}
	if (data_.size() > max_len)
	{
		message_stream << "." << endl << "[+" << data_.size() - max_len << " more common seeds]" << endl << "." << endl;
	}

}
*/