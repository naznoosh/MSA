
#include "../Diamond/basic/config.h"
#include "../Diamond/util/util.h"

#ifndef _MSC_VER
#include <fcntl.h>
//#include <unistd.h>
//#include <errno.h>
//#include <sys/stat.h>
#include <sys/time.h>
#endif

#include "temp_file_ex.h"

#ifdef __linux__
#define POSIX_OPEN(x,y,z) open64(x,y,z)
#define POSIX_OPEN2(x,y) open64(x,y)
#else
#define POSIX_OPEN(x,y,z) open(x,y,z)
#define POSIX_OPEN2(x,y) open(x,y)
#endif


tthread::mutex Temp_file_ex::mtx_;
unsigned Temp_file_ex::n = 0;
uint64_t Temp_file_ex::hash_key;


Temp_file_ex::Temp_file_ex()
{
	mtx_.lock();
	if (n == 0) {
#ifdef _MSC_VER
		LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		hash_key = (uint64_t)(count.HighPart + count.LowPart + count.QuadPart + GetCurrentProcessId());
#else
		timeval count;
		gettimeofday(&count, NULL);
		hash_key = count.tv_sec + count.tv_usec + getpid();
#endif
	}
	std::stringstream ss;
	ss.setf(std::ios::hex, std::ios::basefield);
	if (config.tmpdir != "")
		ss << config.tmpdir << dir_separator;
	ss << "chain-" << hash_key << "-" << to_string(n++) << ".tmp";
	ss >> this->file_name_;
#ifdef _MSC_VER
	this->f_ = fopen(this->file_name_.c_str(), "w+b");
	if (this->f_ == 0) {
		perror(0);
		throw std::runtime_error("Error opening temporary file: " + this->file_name_);
	}
#else
	int fd = POSIX_OPEN(this->file_name_.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd < 0) {
		perror(0);
		throw std::runtime_error(string("Error opening temporary file ") + this->file_name_);
	}
	this->f_ = fdopen(fd, "w+b");
	if (this->f_ == 0) {
		perror(0);
		throw std::runtime_error("Error opening temporary file: " + this->file_name_);
	}
#endif
	mtx_.unlock();
}
