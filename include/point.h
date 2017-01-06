#ifndef POINT_H
#define POINT_H

#include <stdlib.h>

#define MASTER 0

typedef struct Point {
	int index;
	float coord[2];
} Point;

extern const Point FakePoint;

typedef int (*pointsComparator)(const void *a, const void *b);
int pointCmpX(const void *a, const void *b);
int pointCmpY(const void *a, const void *b);

Point getRandomPoint(const float dev);

void mergePointsFromBegin(Point *a, Point *b, Point *res, size_t len, pointsComparator cmp);
void mergePointsFromEnd(Point *a, Point *b, Point *res, size_t len, pointsComparator cmp);

#endif
