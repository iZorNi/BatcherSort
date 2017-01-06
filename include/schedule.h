#ifndef BATCHER_H
#define BATCHER_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Schedule {
	size_t capacity;
	size_t length;
	struct cas_t {
		int v[2];
	} * cas;
} Schedule;

Schedule generateSchedule(int n);
void freeSchedule(Schedule schedule);


#endif
