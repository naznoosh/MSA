#include <limits>
#include <iostream>
#include <sstream>
#include <set>

//#include "../Diamond/src/basic/config.h"
//#include "../Diamond/src/basic/statistics.h"
#include "../Diamond/data/load_seqs.h"
//#include "../Diamond/src/util/seq_file_format.h"
//#include "../Diamond/src/util/log_stream.h"
//#include "../Diamond/src/basic/masking.h"
#include "../malign/seed_set_ex.h"
#include "../malign/common_noise.h"
#include "../malign/seed_histogram_ex.h"
#include "../malign/sorted_list_ex.h"
#include "../malign/common_seed.h"
#include "../malign/compatible_chain.h"
#include "../malign/subseq_align.h"

void malign()
{
	Timer total;
	total.start();

	if (config.alphabet_reduction) Reduction::reduction = Reduction("A KR EDNQ C G H ILVM FYW P ST");

	const unsigned shape_mode = 12;
	unsigned shape_count = 1;
/*
	for (; shape_count < Const::max_shapes; shape_count++)
	{
		if (shape_codes[shape_mode][shape_count] == 0)
			break;
	}
	::shapes = shape_config(shape_mode, shape_count, vector<string>());
*/
	message_stream << "Sequences file: " << config.input_ref_file << endl;

	if (config.input_ref_file == "")
		std::cerr << "Input file parameter (--in) is missing. Input will be read from stdin." << endl;
	task_timer timer("Opening the sequences file", true);
	auto_ptr<Input_stream> db_file(Compressed_istream::auto_detect(config.input_ref_file));


	size_t letters = 0, n = 0, n_seqs = 0;
	Sequence_set *seqs;
//	Sequence_set *code_seqs;
	Hashed_common_seed_set *common_seeds_hashed = NULL;
	Common_noise *common_noise = NULL;
	String_set<0> *ids;
	const FASTA_format format;
	Seqs_histogram *seqs_hst = NULL;
//	vector <vector <SeedInfo>> seqs_seeds;
	try {
//		while ((timer.go("Loading sequences"), n = load_seqs(*db_file, format, &seqs, ids, 0, (size_t)(1e9), string())) > 0) {
		timer.go("Loading sequences");
//		if ((n = load_seqs(*db_file, format, &code_seqs, ids, &seqs, (size_t)(1e9), string())) > 0) {
		if ((n = load_seqs(*db_file, format, &seqs, ids, 0, (size_t)(1e9), string())) > 0) {
//			timer.finish();
			//			Spaced_seed_set *seqs_seeds = new Spaced_seed_set(*seqs, 2);
			if (config.seed_pattern.length() == 0)
			{
				double avg_seqs_length = 0;
				size_t size = seqs->get_length();
				for (size_t i = 0; i < size; i++)
				{
					avg_seqs_length += (double)seqs->length(i)/size;
				}
				config.seed_pattern.assign(std::min((int)std::log2(avg_seqs_length), Const::max_seed_weight - 1), '1');
//				config.seed_pattern.append("0");
			}
//config.seed_pattern.append("0");
			shape_codes[shape_mode][shape_count - 1] = config.seed_pattern.c_str();
			::shapes = shape_config(shape_mode, shape_count, vector<string>());

			timer.finish();
			message_stream << "-- Seed pattern: " << config.seed_pattern << endl;
//			message_stream << "-- Seed pattern: " << config.seed_pattern.erase(config.seed_pattern.find_last_not_of('0') + 1) << endl;

			timer.go("Finding common seeds");
			common_seeds_hashed = new Hashed_common_seed_set(*seqs);

			if (config.verbose) timer.go("Removing common noise");
			common_noise = new Common_noise(*seqs, common_seeds_hashed);
delete common_seeds_hashed;
			if (config.verbose) timer.go("Building sequences histograms");
			seqs_hst = new Seqs_histogram(*seqs, common_noise);
//			seqs_hst = new Seqs_histogram(*seqs, common_seeds_hashed);
//			sorted_seqs_seed(const Sequence_set &seqs, size_t sid, const Seqs_histogram &hst, const _filter *filter) :

			for (size_t sid = 0; sid < shape_count; sid++)
			{
				if (config.verbose) timer.go("Extracting common seed positions");
				Sorted_seqs_seed *seqs_seed = new Sorted_seqs_seed(seqs, sid, *seqs_hst, common_noise);
//				Sorted_seqs_seed seqs_seed(*seqs, sid, *seqs_hst, common_seeds_hashed);

				if (config.verbose) timer.go("Sorting common seeds");
				Sorted_common_seed *common_seed = new Sorted_common_seed(*seqs_seed, sid, *seqs_hst, *common_noise);// &default_noise_filter);
//				Sorted_common_seed common_seed(seqs_seed, sid, &no_noise_filter);
				timer.finish();
				seqs_seed->Show();
delete common_noise;
delete seqs_hst;
				delete seqs_seed;
//				common_seed.Show();

				timer.go("Making compatible chain");
				Compatible_chain *compatible_chain = new Compatible_chain(*common_seed, sid);
				timer.finish();
				delete common_seed;
				compatible_chain->Show();

				if (compatible_chain->best_longest_chain()->size() > 0)// common_seed->total_common_seeds > 0)
				{
					message_stream << config.pkg_name << " - ";
					timer.go("Aligning sequences");
					Subseq_align *subseq_align = new Subseq_align(*compatible_chain, seqs, ids, sid);
					timer.finish();
					subseq_align->Show();
					delete compatible_chain;
					delete subseq_align;
				}
			}
		}
	}
	catch (std::exception&) {
		throw;
	}

	total.stop();
	double total_time = total.getElapsedTime();
	message_stream << "\n\nTotal Time: [";
	if ((int)total_time / 3600 > 0)
	{
		message_stream << (int)total_time / 3600 << "h ";
		total_time -= ((int)total_time / 3600) * 3600;
	}
	if ((int)total_time / 60 > 0)
	{
		message_stream << (int)total_time / 60 << "m ";
		total_time -= ((int)total_time / 60) * 60;
	}
	message_stream << total_time << "s]\n\n";

//	delete seqs_hst;
//	delete common_noise;
//	delete common_seeds_hashed;
}