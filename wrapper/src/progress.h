#pragma once
#ifndef PROGRESS_H_
#define PROGRESS_H_

#include <stddef.h>

struct Progress
{
	Progress(size_t total) :
		total (total) {}

	void progress(size_t val);

	void finish();
private:
	size_t total;
	int len_ = 0;
	size_t percent_ = -1;
};

#endif

