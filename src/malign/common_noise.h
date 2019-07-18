#ifndef COMMON_NOISE_H_
#define COMMON_NOISE_H_

#include <vector>
#include <algorithm> 

#include "../Diamond/data/seed_set.h"
//#include "../Diamond/data/sorted_list.h"
//#include "../Diamond/util/ptr_vector.h"
//#include "../malign/seed_histogram_ex.h"
using std::vector;


extern const size_t too_frequent;
struct Noise_filter
{
	virtual bool valid(size_t freq) const
	{
		return freq <= too_frequent;
	}
};
extern Noise_filter no_noise_filter;

struct Default_noise_filter : Noise_filter
{
	virtual bool valid(size_t freq) const
	{
		return freq <= threshold;
	}

	void set_threshold(size_t thr)
	{
		threshold = std::min(thr, too_frequent);
	}
private:

	size_t threshold = 1;
};
extern Default_noise_filter default_noise_filter;


struct noise
{
	uint64_t seed;
	size_t freq;
	bool operator<(const noise &rhs) const
	{
		return seed < rhs.seed;
	}
};

struct Common_noise
{
//	friend struct Sorted_common_seed;

	Common_noise();
	~Common_noise();

	template<typename _filter>
	Common_noise(const Sequence_set &seqs, const _filter *filter) :
		_noise_entry(shapes.count()),
		_limit(shapes.count()),
		_too_frequent_seeds(shapes.count())
	{
		size_t max_len = seqs.length(0);
		for (size_t k = 1; k < seqs.get_length(); k++)
		{
			if (seqs.length(k) > max_len)
			{
				max_len = seqs.length(k);
			}
		}

		for (size_t i = 0; i < _noise_entry.size(); i++)
		{
			_noise_entry[i] = new noise[max_len];
		}
//		vector<size_t> p{ 0, 1 };
//		Ptr_vector<Callback> cb;
//		cb.push_back(new Callback(_noise_entry, _limit));
//		seqs.enum_seeds(cb, p, 0, shapes.count(), filter);
		Callback cb(_noise_entry, _limit);
		seqs.enum_seeds(&cb, 0, 1, std::make_pair(0, shapes.count()), filter);

		for (size_t i = 0; i < _noise_entry.size(); i++)
		{
			noise *_noise_entry_i = _noise_entry[i];
			size_t _limit_i = _limit[i];
			std::sort(&_noise_entry_i[0], &_noise_entry_i[_limit_i]);
			size_t _idx = 0, next_idx = 0;
			while (next_idx < _limit_i)
			{
				_noise_entry_i[_idx].seed = _noise_entry_i[next_idx++].seed;
				size_t freq = 1;
				while (next_idx < _limit_i && _noise_entry_i[_idx].seed == _noise_entry_i[next_idx].seed)
				{
					freq++;
					next_idx++;
				}
				_noise_entry_i[_idx++].freq = freq;
			}
			_limit[i] = _idx;
		}

		vector<size_t> temp_limit(shapes.count());
		vector<noise *>temp_entry(shapes.count());
		for (size_t i = 0; i < temp_entry.size(); i++)
		{
			temp_entry[i] = new noise[max_len];
		}

		for (unsigned k = 1; k < seqs.get_length(); k++)
		{
//			p[0] = k; p[1] = k + 1;
//			cb.clear();
//			cb.push_back(new Callback(temp_entry, temp_limit));
//			seqs.enum_seeds(cb, p, 0, shapes.count(), filter);
			Callback cb(temp_entry, temp_limit);
			seqs.enum_seeds(&cb, k, k + 1, std::make_pair(0, shapes.count()), filter);

			for (size_t i = 0; i < temp_entry.size(); i++)
			{
				noise *temp_entry_i = temp_entry[i];
				size_t  temp_limit_i = temp_limit[i];
				std::sort(&temp_entry_i[0], &temp_entry_i[temp_limit_i]);

				size_t temp_idx = 0, _idx = 0, next_idx = 0;
				size_t  _limit_i = _limit[i];
				noise *_noise_entry_i = _noise_entry[i];
				while (true)
				{
					while (temp_idx < temp_limit_i && temp_entry_i[temp_idx].seed < _noise_entry_i[next_idx].seed) temp_idx++;
					if (temp_idx >= temp_limit_i) break;
					while (next_idx < _limit_i && _noise_entry_i[next_idx].seed < temp_entry_i[temp_idx].seed) next_idx++;
					if (next_idx >= _limit_i) break;
					if (_noise_entry_i[next_idx].seed == temp_entry_i[temp_idx].seed)
					{
						_noise_entry_i[_idx] = _noise_entry_i[next_idx];
						next_idx++;
						size_t freq = 0;
						while (temp_idx < temp_limit_i && _noise_entry_i[_idx].seed == temp_entry_i[temp_idx].seed)
						{
							freq++;
							temp_idx++;
						}
						_noise_entry_i[_idx].freq = std::min(_noise_entry_i[_idx].freq * freq, too_frequent);
						_idx++;
					}
				}
				temp_limit[i] = 0;
				_limit[i] = _idx;
			}
		}

		for (size_t i = 0; i < temp_entry.size(); i++)
		{
			delete[] temp_entry[i];
		}

		for (size_t i = 0; i < _noise_entry.size(); i++)
		{
			Sd statis;
			noise *_noise_entry_i = _noise_entry[i];
			for (size_t k = 0; k < _limit[i]; k++)
			{
				if (_noise_entry_i[k].freq < too_frequent) statis.add((double)_noise_entry_i[k].freq);
			}
			default_noise_filter.set_threshold((size_t)ceil(sqrt(statis.mean())));


			size_t max_freq = 0;
			for (size_t k = 0; k < _limit[i]; k++) {
				if (_noise_entry_i[k].freq > max_freq) max_freq = _noise_entry_i[k].freq;
			}

			vector <size_t> ferq_hst(max_freq + 1);
			size_t n = 0;
			for (size_t k = 0; k < _limit[i]; k++) {
				ferq_hst[_noise_entry_i[k].freq]++;
				n += _noise_entry_i[k].freq;
			}

			double total_entropy = 0;
			for (size_t k = 1; k <= max_freq; k++) {
				if (ferq_hst[k] == 0) continue;
				double term = (double)k / n;
				total_entropy -= ferq_hst[k] * term * std::log(term);
			}

			double c_entropy = 0, avg_entropy = total_entropy / max_freq, mx=0;
			size_t t_freq = 0;
			for (size_t k = 1; k < max_freq; k++) {
				if (ferq_hst[k] == 0) continue;
				double term = (double)k / n;
				c_entropy -= ferq_hst[k] * term * std::log(term);
				double m = c_entropy / (avg_entropy * k);
				if (m < mx) break;
				mx = m;
				t_freq = k;
			}

			total_entropy *= 0.2;
			double cur_entropy = 0;
			size_t thr_freq = 0;
			for (size_t k = 1; k < max_freq; k++) {
				if (ferq_hst[k] == 0) continue;
				double term = (double)k / n;
				cur_entropy -= ferq_hst[k] * term * std::log(term);
				if (cur_entropy > total_entropy) break;
				thr_freq = k;
			}

			double entropy = 0, max_entropy = 0;
			size_t opt_freq;
			for (size_t k = 1; k < max_freq; k++) {
				if (ferq_hst[k] == 0) continue;
//				double term = (double)ferq_hst[k] / _limit[i];
//				entropy -= term * std::log(term);
//				if (3 * entropy / k <= max_entropy) break;
				double term = (double)k / n;
				entropy -= ferq_hst[k] * term * std::log(term);
				if (5*entropy / k <= max_entropy) break;
				max_entropy = entropy;
				opt_freq = k;
			}
/*
			entropy = std::log(n) - entropy / n;
			
			double entropy_k = 0;
			size_t thr;
			for (size_t k = 0, n = 0; k < max_freq; k++) {
				entropy_k += ferq_hst[k] * std::log((double)ferq_hst[k]);
				n += ferq_hst[k];
				if()
			}
			entropy = std::log(n) - entropy / n;
*/
			size_t limit_ = 0;
			for (size_t k = 0; k < _limit[i]; k++)
			{
//				if (default_noise_filter.valid(_noise_entry_i[k].freq))
				if (_noise_entry_i[k].freq <= opt_freq)
				{
					_noise_entry_i[limit_] = _noise_entry_i[k];
					limit_++;
				}
			}
			if (_limit[i] != 0 && limit_ == 0) _too_frequent_seeds[i] = true;
			else                               _too_frequent_seeds[i] = false;
			_limit[i] = limit_;
		}
	}

	bool contains(uint64_t key, uint64_t shape) const;
	size_t get_length(size_t sid)
	{
		return _limit[sid];
	}

	size_t get_frequency(size_t sid, size_t i)
	{
		return _noise_entry[sid][i].freq;
	}

	bool is_too_frequent_seeds(size_t sid)
	{
		return _too_frequent_seeds[sid];
	}
private:
	struct Callback
	{
		Callback(vector<noise *> &_entry, vector<size_t> &_limit) :
			_entry(_entry),
			_limit(_limit) 
		{}

		bool operator()(uint64_t seed, uint64_t pos, size_t shape)
		{
			size_t &limit_ = _limit[shape];
			noise &entry_ = _entry[shape][limit_];
			entry_.seed = seed;
//			entry_.freq = 1;
			limit_++;
			return true;
		}

		void finish() const
		{
		}
	private:
		vector<size_t> &_limit;
		vector<noise *> &_entry;
	};

	vector<size_t> _limit;
	vector<noise *>_noise_entry;
	vector<bool> _too_frequent_seeds;
};

#endif /* SORTED_LIST_EX_H_ */
