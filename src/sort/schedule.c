#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "schedule.h"

static Schedule newSchedule()
{
	Schedule r;
	r.length = 0;
	r.capacity = 1;
	r.cas = malloc(r.capacity * sizeof(struct cas_t));
	assert(r.cas);
	return r;
}

static void adjustCapacity(Schedule *schedule)
{
	if (schedule->length < schedule->capacity)
		return;
	schedule->capacity *= 2;
	struct cas_t *f = realloc(schedule->cas, schedule->capacity * sizeof(struct cas_t));
	assert(f);
	schedule->cas = f;
}

static void appendToSchedule(Schedule *schedule, struct cas_t val)
{
	adjustCapacity(schedule);
	schedule->cas[schedule->length] = val;
	schedule->length += 1;
}

void freeSchedule(Schedule schedule)
{
	free(schedule.cas);
}

static void cleanupPtr(int **ptr)
{
	free(*ptr);
}

static int s(Schedule *schedule, int *up, size_t lengthUp, int *down, size_t lengthDown, int depth)
{
	if (lengthUp + lengthDown <= 1) {
		return depth - 1;
	}
	if (lengthUp == 1 && lengthDown == 1) {
		struct cas_t c = {
		    .v = {*up, *down}};
		appendToSchedule(schedule, c);
		return depth;
	}
	int *up_odd __attribute__((__cleanup__(cleanupPtr))) = calloc(lengthUp / 2, sizeof(int));
	assert(up_odd);
	int *up_even __attribute__((__cleanup__(cleanupPtr))) = calloc(lengthUp / 2 + lengthUp % 2, sizeof(int));
	assert(up_even);

	for (uint i = 0; i < lengthUp; i++) {
		if (i % 2 == 0) {
			up_even[i / 2] = up[i];
		}
		else {
			up_odd[i / 2] = up[i];
		}
	}

	int *down_odd __attribute__((__cleanup__(cleanupPtr))) = calloc(lengthDown / 2, sizeof(int));
	assert(down_odd);
	int *down_even __attribute__((__cleanup__(cleanupPtr))) = calloc(lengthDown / 2 + lengthDown % 2, sizeof(int));
	assert(down_even);
	for (uint i = 0; i < lengthDown; i++) {
		if (i % 2 == 0) {
			down_even[i / 2] = down[i];
		}
		else {
			down_odd[i / 2] = down[i];
		}
	}
	int d1, d2;
	d1 = s(schedule, up_odd, lengthUp / 2, down_odd, lengthDown / 2, depth + 1);
	d2 = s(schedule, up_even, lengthUp / 2 + lengthUp % 2,
	       down_even, lengthDown / 2 + lengthDown % 2, depth + 1);
	int *t __attribute__((__cleanup__(cleanupPtr))) = calloc(lengthUp + lengthDown, sizeof(int));
	assert(t);

	memcpy(t, up, lengthUp * sizeof(int));
	memcpy(t + lengthUp, down, lengthDown * sizeof(int));
	for (uint i = 1; i + 1 < lengthUp + lengthDown; i += 2) {
		struct cas_t c = {
		    .v = {t[i], t[i + 1]}};
		appendToSchedule(schedule, c);
	}
	if (d1 > d2) {
		return d1;
	}
	return d2;
}

static void b(Schedule *schedule, int *up, size_t lengthUp, int *down, size_t lengthDown)
{
	if (lengthUp + lengthDown == 1)
		return;

	b(schedule, up, lengthUp / 2, up + lengthUp / 2, lengthUp / 2 + lengthUp % 2);
	b(schedule, down, lengthDown / 2, down + lengthDown / 2, lengthDown / 2 + lengthDown % 2);
	s(schedule, up, lengthUp, down, lengthDown, 1);
}

Schedule generateSchedule(int n)
{
	Schedule schedule = newSchedule();
	int *d __attribute__((__cleanup__(cleanupPtr))) = calloc(n, sizeof(int));
	for (int i = 0; i < n; i++) {
		d[i] = i;
	}
	b(&schedule, d, n / 2, d + n / 2, n / 2 + n % 2);

	return schedule;
}
