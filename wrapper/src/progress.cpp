#include "progress.h"
#include <stdio.h>

void Progress::progress(size_t val)
{
	size_t percent = val * 10000 / total;
	if (percent_ == percent) return;
	
	percent_ = percent;
	len_ = fprintf(stderr, "  %05.2f%%", percent/100.0f);
	for (size_t i = 0; i < len_; i++) fprintf(stderr, "\b");
}


void Progress::finish()
{
	fprintf(stderr, "% *s", len_, "");
	for (size_t i = 0; i < len_; i++) fprintf(stderr, "\b");
}