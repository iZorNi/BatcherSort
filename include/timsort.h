#ifndef TIMSORT_H
#define TIMSORT_H

#include "point.h"

int timsort(void *base, size_t nel, size_t width,
	    int (*compar) (const void *, const void *));

#endif
