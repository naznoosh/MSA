#include <iomanip>

#include "../Diamond/basic/config.h"
#include "../Diamond/basic/shape_config.h"
#include "../malign/subseq_align.h"
#include "../malign/temp_file_ex.h"
#include "../malign/progress.h"

Subseq_align::Subseq_align(Compatible_chain &compatible_chain, Sequence_set *seqs, String_set<0> *ids, size_t sid) :
	seqs(seqs), ids(ids), sid(sid),
	chain(*compatible_chain.best_longest_chain())
{
	const size_t chain_size = chain.size();

	vector <size_t> seg_size(chain_size + 1);
	aligned.resize(chain_size + 1);
	for (size_t i = 0; i <= chain_size; i++)
		aligned[i].resize(seqs->get_length());

	vector<size_t> p_range(config.threads_ + 1);
	for (size_t i = 1, r = (chain_size + 1) % config.threads_; r > 0; i++, r--) p_range[i]++;
	for (size_t i = 1, m = (chain_size + 1) / config.threads_; i <= config.threads_; i++) p_range[i] += p_range[i - 1] + m;

	vector <Subseq_context> context(config.threads_);
	Thread_pool threads;
	for (size_t i = 0; i < config.threads_; ++i)
	{
		threads.push_back(launch_thread(subseq_worker, &context[i], std::make_pair(p_range[i], p_range[i+1]), seqs, &chain, &aligned[p_range[i]], &seg_size[p_range[i]]));
//		subseq_worker(&context[i], std::make_pair(p_range[i], p_range[i + 1]), seqs, &chain, &aligned[p_range[i]], &seg_size[p_range[i]]);
	}
	threads.join_all();

	size_t n = 0;
	for (size_t i = 0; i < config.threads_; ++i) n += context[i].aligned_idx.size();

	vector <string> res_file_names(n);
	vector <string> data_file_names(n);
	vector <size_t> aligned_idx(n);

	struct Seg_len_index {
		size_t len, index;
		bool operator < (const Seg_len_index &right)
		{
			return right.len < len;
		}
	};
	vector <Seg_len_index>  seg_len_index(n);

	for (size_t i = 0, n = 0; i < config.threads_; ++i)
	{
		Subseq_context *cntxt = &context[i];
		for (size_t k = 0; k < cntxt->aligned_idx.size(); k++, n++)
		{
			res_file_names[n] = cntxt->res_file_names[k];
			data_file_names[n] = cntxt->data_file_names[k];
			aligned_idx[n] = cntxt->aligned_idx[k];
			seg_len_index[n] = { cntxt->seg_len[k], n };
		}
	}
	context.clear();

	std::sort(&seg_len_index[0], &seg_len_index[seg_len_index.size() - 1] + 1);
	vector <vector<size_t>> files_names_index(config.run_parallel);
	vector <size_t> files_size(config.run_parallel);
	vector <size_t> tmp_files_len(config.run_parallel);
	for (size_t i = 0; i < seg_len_index.size(); i++)
	{
		size_t idx = 0;
		for (size_t k = 1; k < config.run_parallel; k++)
			if (tmp_files_len[k] < tmp_files_len[idx]) idx = k;
		tmp_files_len[idx] += seg_len_index[i].len;

		files_size[idx]++;
		files_names_index[idx].push_back(seg_len_index[i].index);
	}

	vector <string> files_names(config.run_parallel);
	for (size_t i = 0; i < config.run_parallel; i++)
	{
		Temp_file_ex tmp_file_names;
		std::sort(&files_names_index[i][0], &files_names_index[i][files_size[i] - 1] + 1);

		for (size_t k = 0; k < files_size[i]; k++)
		{
			tmp_file_names.write_raw(data_file_names[files_names_index[i][k]].c_str(), data_file_names[files_names_index[i][k]].size());
			tmp_file_names.write_raw("\n", 1);
			tmp_file_names.write_raw(res_file_names[files_names_index[i][k]].c_str(), res_file_names[files_names_index[i][k]].size());
			tmp_file_names.write_raw("\n", 1);
		}

		tmp_file_names.close();
		files_names[i] = tmp_file_names.file_name();
	}

	if (config.pkg != config.NOALIGN)
	{
		Pkg_context context(files_names, files_size);
		launch_thread_pool(context, config.run_parallel);
	}

	for (size_t i = 0; i < res_file_names.size(); i++)
	{
		const size_t k = aligned_idx[i];
		string *ptr_aligned = aligned[k].data();
		size_t &seg_size_k = seg_size[k];
		Input_stream aligned_file(res_file_names[i]);
		aligned_file.getline();
		for (size_t n = 0, s; n < seqs->get_length(); n++)
		{
			s = atoi(aligned_file.line.c_str() + 1);
			aligned_file.getline();
			string line;
			while (!aligned_file.eof() && aligned_file.line[0] != '>') {
				line.append(aligned_file.line);
				aligned_file.getline();
			}

			ptr_aligned[s] = line;
			if (line.length() > seg_size_k) seg_size_k = line.length();
		}
		aligned_file.close();

		if (seg_size_k == 0)
		{
			Input_stream input_file(data_file_names[i]);
			for (size_t n = 0, s; n < seqs->get_length(); n++)
			{
				input_file.getline();
				s = atoi(input_file.line.c_str() + 1);
				input_file.getline();
				ptr_aligned[s] = input_file.line;
				if (input_file.line.length() > seg_size_k) seg_size_k = input_file.line.length();
			}
			input_file.close();

			non_aligned_len += seg_size_k;
			non_aligned_seg_idx.push_back(k);
		}
	}

	for (size_t i = 0; i <= chain_size; i++)
	{
		size_t sz = seg_size[i];
		if (sz == 0) continue;

		for (size_t s = 0; s < seqs->get_length(); s++)
		{
			if (aligned[i][s].length() != sz)
				aligned[i][s].append(sz - aligned[i][s].length(), '-');
		}
	}

	Output_stream out(config.output_file);
	for (size_t s = 0; s < seqs->get_length(); s++)
	{
		out.write_raw(">", 1);
		out.write_raw(ids->ptr(s), ids->length(s));
		out.write_raw("\n", 1);

		for (size_t i = 0; i < chain_size; i++)
		{
			out.write_raw(aligned[i][s].c_str(), aligned[i][s].length());

			char * p = seqs->ptr(s) + chain[i].pos[s];
			const size_t len = chain[i].length;
			string seg(len, ' ');
			for (size_t k = 0; k < len; k++)
			{
				seg[k] = to_char(*p++);// input_value_traits.alphabet[*p++];
			}
			out.write_raw(seg.c_str(), seg.length());
		}
		out.write_raw(aligned[chain_size][s].c_str(), aligned[chain_size][s].length());

		out.write_raw("\n", 1);
	}
	out.close();
#ifndef _MSC_VER
	string cmd = "rm ";
	cmd.append(config.tmpdir).append("/chain*.*");
#else
	string cmd = "del ";
	cmd.append(config.tmpdir).append("\\chain*.*");
#endif

	system(cmd.c_str());

	for (size_t i = 0; i < chain_size; i++)
	{
		total_len += aligned[i][0].length();
		total_len += chain[i].length;
	}
	total_len += aligned[chain_size][0].length();
}


void Subseq_align::Show()
{
	if (non_aligned_len > 0)
	{
//		message_stream << "\n** CAUTION: approximately " << /*std::setprecision(2) << */(total_len - non_aligned_len) * 100.0 / total_len << "% alignment has been achived!\n";
		printf("\n** CAUTION: approximately %5.2f%% alignment has been achived!\n", (total_len - non_aligned_len) * 100.0 / total_len);
		if (config.verbose)
		{
			message_stream << "\n\n UNALIGNED SEGMENTS             Positions";
			message_stream << "\n                        ________________________";
			message_stream << "\n                        from            to              length";
			message_stream << "\n                        ________        ________        ________";
			size_t from = 1;
			size_t i = 0;
			size_t sum = 0, min = -1, max = 0;
			for (size_t k = 0; k < non_aligned_seg_idx.size(); k++)
			{
				for (; i < non_aligned_seg_idx[k]; i++)
				{
					from += aligned[i][0].length();
					from += chain[i].length;
				}
				message_stream << "\n                        " << from << "\t\t";
				size_t len = aligned[i][0].length();
				from += len;
				sum += len;
				if (len < min) min = len;
				if (len > max) max = len;
				message_stream << from << "\t\t" << len;
				if (i < chain.size()) from += chain[i].length;
				i++;
			}
			message_stream << "\n                        ________________________________________";
			message_stream << "\n                                        Minimum Length: " << min;
			message_stream << "\n                                        Maximum Length: " << max;
			message_stream << "\n                                        Average Length: " << (size_t)std::ceil((double)sum / non_aligned_seg_idx.size());
		}
	}

}


void Subseq_align::subseq_worker(Subseq_context *context, pair<size_t,size_t> range, Sequence_set *seqs, const Chain *chain_, vector <string > *aligned, size_t *seg_size)
{
	const Chain &chain = *chain_;
	const size_t seq_length = seqs->get_length();
	const size_t chain_size = chain.size();
	vector <uint64_t> s_pos(seq_length);
	
	for (size_t i = 0; i < std::min(range.first, chain_size); i++)
	{
		const Common_seed *chain_i = &chain[i];
		for (size_t s = 0; s < seq_length; s++)
		{
				s_pos[s] = chain_i->pos[s] + chain_i->length;
		}
	}

	for (size_t i = range.first; i < range.second; i++, aligned++, seg_size++)
	{
		string *ptr_aligned = aligned->data();
		bool same = true;
		size_t len = (i < chain_size ? chain[i].pos[0] : seqs->length(0)) - s_pos[0];
		for (size_t s = 1; s < seq_length; s++)
		{
			if (len != (i < chain_size ? chain[i].pos[s] : seqs->length(s)) - s_pos[s])
			{
				same = false;
				break;
			}
		}
		if (same)
		{
			vector <char *> p(seq_length);
			for (size_t s = 0; s < seq_length; s++)
			{
				p[s] = seqs->ptr(s) + s_pos[s];
			}

			string seg(len, ' ');
			for (size_t k = 0; k < len; k++)
			{
				char ch = *p[0]++;
				for (size_t s = 1; s < seq_length; s++)
				{
					if (ch != *p[s]++)
					{
						same = false;
						break;
					}
				}
				if (!same) break;

				seg[k] = to_char(ch);// input_value_traits.alphabet[ch];
			}
			if (same)
			{
				for (size_t s = 0; s < seq_length; s++)
				{
					ptr_aligned[s] = seg;

					if (i < chain_size)
						s_pos[s] = chain[i].pos[s] + chain[i].length;
				}
//				(*context->seg_size)[i] = len;
				*seg_size = len;
				continue;
			}
		}
		else {
			bool hit = false;
			size_t idx;
			for (size_t s = 0; s < seq_length; s++)
			{
				if ((i < chain_size ? chain[i].pos[s] : seqs->length(s)) - s_pos[s] > 0)
				{
					if (!hit) {
						hit = true;
						idx = s;
					}
					else {
						hit = false;
						break;
					}
				}
			}
			if (hit) {
				size_t len = (i < chain_size ? chain[i].pos[idx] : seqs->length(idx)) - s_pos[idx];
				char * p = seqs->ptr(idx) + s_pos[idx];
				string seg(len, ' ');
				for (size_t k = 0; k < len; k++) {
					seg[k] = to_char(*p++);// input_value_traits.alphabet[*p++];
				}
				ptr_aligned[idx] = seg;
				*seg_size = len;

				if (i < chain_size) {
					for (size_t s = 0; s < seq_length; s++) {
						s_pos[s] = chain[i].pos[s] + chain[i].length;
					}
				}
				continue;
			}
		}

		Temp_file_ex tmp_data_file;
		Temp_file_ex tmp_res_file;
		size_t max_len = 0;

		for (size_t s = 0; s < seq_length; s++)
		{
			char * p = seqs->ptr(s) + s_pos[s];
			size_t len = (i < chain_size ? chain[i].pos[s] : seqs->length(s)) - s_pos[s];
			if (len > 0) {
				string s_no = to_string(s);
				tmp_data_file.write_raw(">", 1);
				tmp_data_file.write_raw(s_no.c_str(), s_no.length());
				tmp_data_file.write_raw("\n", 1);
				if (len > max_len) max_len = len;

				string seg(len + 1, '\n');
				for (size_t k = 0; k < len; k++) {
					seg[k] = to_char(*p++);// input_value_traits.alphabet[*p++];
				}
//				seg.push_back('\n');
				tmp_data_file.write_raw(seg.c_str(), seg.length());
			}
			if (i < chain_size)	s_pos[s] = chain[i].pos[s] + chain[i].length;
		}
		tmp_data_file.close();
		tmp_res_file.close();

		context->seg_len.push_back(max_len);
		context->data_file_names.push_back(tmp_data_file.file_name().c_str());
		context->res_file_names.push_back(tmp_res_file.file_name().c_str());
		context->aligned_idx.push_back(i);
	}
}


void Subseq_align::Pkg_context::operator()(unsigned thread_id) const
{
	string cmnd;
#ifndef _MSC_VER
	if (config.verbose) cmnd.append("gnome-terminal --disable-factory -e '");
	switch (config.pkg)
	{
	case Config::HALIGN:
		cmnd.append("java -cp ")
			.append(config.root).append("/packages/HAlign:")
			.append(config.root).append("/packages/HAlign/HAlign2.1.jar halignWrapper \"");
		break;
	case Config::FAMSA:
		cmnd.append(config.root).append("/packages/FAMSA/famsaWrapper \"");
		break;
	case Config::MAFFT:
		cmnd.append(config.root).append("/packages/MAFFT/mafftWrapper \"");
		break;
	case Config::MUSCLE:
		cmnd.append(config.root).append("/packages/MUSCLE/muscleWrapper \"");
		break;
	case Config::CLUSTALO:
		cmnd.append(config.root).append("/packages/ClustalO/clustaloWrapper \"");
		break;
	case Config::CLUSTALW:
		cmnd.append(config.root).append("/packages/ClustalW/clustalwWrapper \"");
		break;
	case Config::KALIGN:
		cmnd.append(config.root).append("/packages/Kalign/kalignWrapper \"");
		break;
	}
#else
	if (config.verbose) cmnd.append("start /WAIT ");
	switch (config.pkg)
	{
	case Config::HALIGN:
		cmnd.append("java -cp \"")
			.append(config.root).append("\\packages\\HAlign;")
			.append(config.root).append("\\packages\\HAlign\\HAlign2.1.jar\" halignWrapper \"");
		break;
	case Config::FAMSA:
		cmnd.append(config.root).append("\\packages\\FAMSA\\famsaWrapper.exe \"");
		break;
	case Config::MAFFT:
		cmnd.append(config.root).append("\\packages\\MAFFT\\mafftWrapper.exe \"");
		break;
	case Config::MUSCLE:
		cmnd.append(config.root).append("\\packages\\MUSCLE\\muscleWrapper.exe \"");
		break;
	case Config::CLUSTALO:
		cmnd.append(config.root).append("\\packages\\ClustalO\\clustaloWrapper.exe \"");
		break;
	case Config::CLUSTALW:
		cmnd.append(config.root).append("\\packages\\ClustalW\\clustalwWrapper.exe \"");
		break;
	case Config::KALIGN:
		cmnd.append(config.root).append("\\packages\\Kalign\\kalignWrapper.exe \"");
		break;
}
#endif
	cmnd.append(files_names[thread_id].c_str()).append("\" ")
		.append(to_string(files_size[thread_id])).append(" ")
		.append(to_string(thread_id))
//		.append(to_string(thread_id)).append(" ").append(to_string(config.threads_))
//		.append(" ").append(to_string(total))
		.append(" ").append(to_string(config.verbose))
		.append(" ").append(to_string(config.threads_));
#ifndef _MSC_VER
	if (config.verbose) cmnd.append("'");
	else cmnd.append(" > /dev/null");
#else
	if (!config.verbose) cmnd.append(" > NUL");
#endif
	system(cmnd.c_str());
}
