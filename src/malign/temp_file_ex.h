#ifndef TEMP_FILE_EX_H_
#define TEMP_FILE_EX_H_

#include "../Diamond/util/binary_file.h"
#include "../Diamond/util/tinythread.h"

struct Temp_file_ex : public Output_stream
{
	Temp_file_ex();

#ifndef _MSC_VER
	virtual ~Temp_file_ex()
	{}
#endif


	string file_name()
	{
		return file_name_;
	}

protected:
	static tthread::mutex mtx_;

//	static string get_temp_dir();
	static unsigned n;
	static uint64_t hash_key;
};

#endif

