#ifndef SUBSEQ_ALIGN_H_
#define SUBSEQ_ALIGN_H_

#include "../Diamond/data/sequence_set.h"
#include "../malign/compatible_chain.h"

struct Subseq_align
{
	Subseq_align(Compatible_chain &compatible_chain, Sequence_set *seqs, String_set<0> *ids, size_t sid);
	void Show();
private:
	typedef Compatible_chain::Chain Chain;
	typedef Compatible_chain::Common_seed Common_seed;

	Sequence_set *seqs;
	String_set<0> *ids;
	size_t sid;
	const Chain &chain;
	vector <vector <string>> aligned;
	uint64_t total_len = 0;
	vector <size_t> non_aligned_seg_idx;
	uint64_t non_aligned_len = 0;


	struct Pkg_context
	{
		Pkg_context(vector <string> &files_names, vector <size_t> &files_size) :
			files_names(files_names),
			files_size(files_size)
		{ }

		void operator()(unsigned thread_id) const;

	protected:
		vector <string> &files_names;
		vector <size_t> &files_size;
	};

	struct Subseq_context
	{
//		vector <vector <string>> *aligned;
		vector <string> data_file_names;
		vector <string> res_file_names;
		vector <size_t> aligned_idx;
		vector <size_t> seg_len;
//		vector <size_t> *seg_size;
	};

	static void subseq_worker(Subseq_context *context, pair<size_t,size_t> range, Sequence_set *seqs, const Chain *chain_, vector <string > *aligned, size_t *seg_size);
};

#endif /* SUBSEQ_ALIGN_H_ */
