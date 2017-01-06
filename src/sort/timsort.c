#include <assert.h>		// assert
#include <errno.h>		// EINVAL
#if defined(_MSC_VER)
# include <malloc.h>		// _alloca
#endif
#include <stddef.h>		// size_t, NULL
#include <stdlib.h>		// malloc, free
#include <string.h>		// memcpy, memmove
#include "timsort.h"
#define MIN_MERGE 32

#define MIN_GALLOP 7

#define INITIAL_TMP_STORAGE_LENGTH 256

#define MAX_STACK 85

#if defined(_MSC_VER)
# define DEFINE_TEMP(temp) void *temp = _alloca(WIDTH)
#else
# define DEFINE_TEMP(temp) char temp[WIDTH]
#endif

#define ASSIGN(x, y) memcpy(x, y, WIDTH)
#define INCPTR(x) ((void *)((char *)(x) + WIDTH))
#define DECPTR(x) ((void *)((char *)(x) - WIDTH))
#define ELEM(a,i) ((char *)(a) + (i) * WIDTH)
#define LEN(n) ((n) * WIDTH)

#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#define SUCCESS 0
#define FAILURE (-1)

#define CONCAT(x, y) x ## _ ## y
#define MAKE_STR(x, y) CONCAT(x,y)
#define NAME(x) MAKE_STR(x, WIDTH)
#define CALL(x) NAME(x)

typedef int (*comparator) (const void *x, const void *y);
#define CMPPARAMS(compar, thunk) comparator compar
#define CMPARGS(compar, thunk) (compar)
#define CMP(compar, thunk, x, y) (compar((x), (y)))
#define TIMSORT timsort


struct timsort_run {
	void *base;
	size_t len;
};

struct timsort {
	void *a;
	size_t a_length;
	comparator c;
#ifdef IS_TIMSORT_R
	void *carg;
#endif
	size_t minGallop;
	void *tmp;
	size_t tmp_length;
	size_t stackSize;	// Number of pending runs on stack
	size_t stackLen;	// maximum stack size
#ifdef MALLOC_STACK
	struct timsort_run *run;
#else
	struct timsort_run run[MAX_STACK];
#endif
};

static int timsort_init(struct timsort *ts, void *a, size_t len,
			CMPPARAMS(c, carg),
			size_t width);
static void timsort_deinit(struct timsort *ts);
static size_t minRunLength(size_t n);
static void pushRun(struct timsort *ts, void *runBase, size_t runLen);
static void *ensureCapacity(struct timsort *ts, size_t minCapacity,
			    size_t width);

static int timsort_init(struct timsort *ts, void *a, size_t len,
			CMPPARAMS(c, carg),
			size_t width)
{
	int err = 0;

	assert(ts);
	assert(a || !len);
	assert(c);
	assert(width);

	ts->minGallop = MIN_GALLOP;
	ts->stackSize = 0;

	ts->a = a;
	ts->a_length = len;
	ts->c = c;
#ifdef IS_TIMSORT_R
	ts->carg = carg;
#endif

	// Allocate temp storage (which may be increased later if necessary)
	ts->tmp_length = (len < 2 * INITIAL_TMP_STORAGE_LENGTH ?
			  len >> 1 : INITIAL_TMP_STORAGE_LENGTH);
	if (ts->tmp_length) {
		ts->tmp = malloc(ts->tmp_length * width);
		err |= ts->tmp == NULL;
	} else {
		ts->tmp = NULL;
	}


#ifdef MALLOC_STACK
	ts->stackLen = (len < 359 ? 5
			: len < 4220 ? 10
			: len < 76210 ? 16 : len < 4885703256ULL ? 39 : 85);

	ts->run = malloc(ts->stackLen * sizeof(ts->run[0]));
	err |= ts->run == NULL;
#else
	ts->stackLen = MAX_STACK;
#endif

	if (!err) {
		return SUCCESS;
	} else {
		timsort_deinit(ts);
		return FAILURE;
	}
}

static void timsort_deinit(struct timsort *ts)
{
	free(ts->tmp);
#ifdef MALLOC_STACK
	free(ts->run);
#endif
}

static size_t minRunLength(size_t n)
{
	size_t r = 0;		// Becomes 1 if any 1 bits are shifted off
	while (n >= MIN_MERGE) {
		r |= (n & 1);
		n >>= 1;
	}
	return n + r;
}


static void pushRun(struct timsort *ts, void *runBase, size_t runLen)
{
	assert(ts->stackSize < ts->stackLen);

	ts->run[ts->stackSize].base = runBase;
	ts->run[ts->stackSize].len = runLen;
	ts->stackSize++;
}


static void *ensureCapacity(struct timsort *ts, size_t minCapacity,
			    size_t width)
{
	if (ts->tmp_length < minCapacity) {
		size_t newSize = minCapacity;
		newSize |= newSize >> 1;
		newSize |= newSize >> 2;
		newSize |= newSize >> 4;
		newSize |= newSize >> 8;
		newSize |= newSize >> 16;
		if (sizeof(newSize) > 4)
			newSize |= newSize >> 32;

		newSize++;
		newSize = MIN(newSize, ts->a_length >> 1);
		if (newSize == 0) {
			newSize = minCapacity;
		}

		free(ts->tmp);
		ts->tmp_length = newSize;
		ts->tmp = malloc(ts->tmp_length * width);
	}

	return ts->tmp;
}

#define WIDTH 4
#include "timsort-impl.h"
#undef WIDTH

#define WIDTH 8
#include "timsort-impl.h"
#undef WIDTH

#define WIDTH 16
#include "timsort-impl.h"
#undef WIDTH

#define WIDTH width
#include "timsort-impl.h"
#undef WIDTH


int TIMSORT(void *a, size_t nel, size_t width, CMPPARAMS(c, carg))
{
	switch (width) {
	case 4:
		return timsort_4(a, nel, width, CMPARGS(c, carg));
	case 8:
		return timsort_8(a, nel, width, CMPARGS(c, carg));
	case 16:
		return timsort_16(a, nel, width, CMPARGS(c, carg));
	default:
		return timsort_width(a, nel, width, CMPARGS(c, carg));
	}
}
