#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "point.h"

const Point FakePoint = {
    .coord = {FLT_MIN, FLT_MIN},
    .index = -1,
};


static int comparePoints(const void *a, const void *b, const int coord)
{
	Point *point_a = (Point *)a;
	Point *point_b = (Point *)b;

	if (point_a->index == -1 || point_b->index == -1) {
		return point_a->index - point_b->index;
	}
	
	if (point_a->coord[coord] < point_b->coord[coord])
		return -1;
	if (point_a->coord[coord] > point_b->coord[coord])
		return +1;
	return 0;
}

int pointCmpX(const void *a, const void *b)
{
	return comparePoints(a, b, 0);
}

int pointCmpY(const void *a, const void *b)
{
	return comparePoints(a, b, 1);
}

Point getRandomPoint(const float dev)
{
	static int index = 0;
	float x = (float)rand() / (float)(RAND_MAX / dev) - dev / 2;
	float y = (float)rand() / (float)(RAND_MAX / dev) - dev / 2;
	Point ret = {
	    .coord = {x, y},
	    .index = index++,
	};
	return ret;
}

void mergePointsFromBegin(Point *a, Point *b, Point *res, size_t len, pointsComparator cmp)
{
	Point *end = res + len;
	for (; res < end; res++) {
		*res = cmp(a, b) < 0 ? *a++ : *b++;
	}
}

void mergePointsFromEnd(Point *a, Point *b, Point *res, size_t len, pointsComparator cmp)
{
	Point *end = res;
	a += len - 1;
	b += len - 1;
	res += len - 1;
	for (; res >= end; res--) {
		*res = cmp(a, b) > 0 ? *a-- : *b--;
	}
}
