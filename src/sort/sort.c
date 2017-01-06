#include <assert.h>
#include <mpi.h>

#include "schedule.h"
#include "point.h"
#include "point_mpi.h"
#include "timsort.h"
static void swapPtr(void *ptr1, void *ptr2)
{
	void **ptr1_ = (void **)ptr1;
	void **ptr2_ = (void **)ptr2;

	void *tmp = *ptr1_;
	*ptr1_ = *ptr2_;
	*ptr2_ = tmp;
}

static void cleanupPtr(Point **ptr)
{
	free(*ptr);
}

static int sorted(Point *points, int n, pointsComparator cmp)
{
	for (int i = 0; i < n - 1; i++) {
		if (cmp(points + i, points + i + 1) > 0) {
			return -1;
		}
	}
	return 0;
}

static void batcherSort(Point **points, int n, int rank, int size, pointsComparator cmp)
{
	Point *buf __attribute__((__cleanup__(cleanupPtr))) = calloc(n, sizeof(Point));
	assert(buf);
	Point *res __attribute__((__cleanup__(cleanupPtr))) = calloc(n, sizeof(Point));
	assert(res);

	MPI_Datatype mpiPoint;
	initPointMPI(&mpiPoint);
	timsort(*points, n, sizeof(Point), cmp);
	Schedule s = generateSchedule(size);
	MPI_Status status;
	for (struct cas_t *cas = s.cas; cas < s.cas + s.length; cas++) {
		if (rank == cas->v[0]) {
			MPI_Send(*points, n, mpiPoint, cas->v[1], 0, MPI_COMM_WORLD);
			MPI_Recv(buf, n, mpiPoint, cas->v[1], 0, MPI_COMM_WORLD, &status);
			mergePointsFromBegin(*points, buf, res, n, cmp);
			swapPtr(points, &res);
		}
		else if (rank == cas->v[1]) {
			MPI_Recv(buf, n, mpiPoint, cas->v[0], 0, MPI_COMM_WORLD, &status);
			MPI_Send(*points, n, mpiPoint, cas->v[0], 0, MPI_COMM_WORLD);
			mergePointsFromEnd(*points, buf, res, n, cmp);
			swapPtr(points, &res);
		}
	}
	
	MPI_Barrier(MPI_COMM_WORLD);


	freeSchedule(s);
	freePointMPI(&mpiPoint);
}


static int verify(Point *points, int n, int rank, int size, pointsComparator cmp)
{
	MPI_Datatype mpiPoint;
	initPointMPI(&mpiPoint);
	int a = sorted(points, n, cmp);
	int ret;
	MPI_Allreduce(&a, &ret, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	if (ret) {
		return ret;
	}
	Point *buf __attribute__((__cleanup__(cleanupPtr))) = calloc(2 * size, sizeof(Point));
	assert(buf);
	Point *pair __attribute__((__cleanup__(cleanupPtr))) = calloc(2, sizeof(Point));
	assert(pair);
	pair[0] = points[0];
	pair[1] = points[n - 1];
	MPI_Gather(pair, 2, mpiPoint, buf, 2, mpiPoint, MASTER, MPI_COMM_WORLD);
	if (rank == MASTER) {
		ret = sorted(buf, 2 * size, cmp);
	}
	MPI_Bcast(&ret, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
	freePointMPI(&mpiPoint);
	if (rank == MASTER) {
		if (ret) {
			printf("verify batcher sort: fail\n");
		}
		else {
			printf("verify batcher sort: ok\n");
		}
	}
	return ret;
}

static void timer(int rank, char *name, void (*cb)(void))
{
	double time = MPI_Wtime();
	cb();
	time = MPI_Wtime() - time;
	if (rank == MASTER)
		printf("%s time: %lfs\n", name, time);
}

static Point *readFileSequent(char *filename, int *n)
{
	FILE *input = fopen(filename, "rb");
	if (input == NULL) {
		fprintf(stderr, "Can't open file `%s'\n", filename);
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

	if (fread(n, sizeof(*n), 1, input) != 1) {
		fprintf(stderr, "Error reading file `%s'\n", filename);
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}
	Point *points = calloc(*n, sizeof(Point));
	if (points == NULL) {
		fprintf(stderr, "Cannot alloc memory for qsort");
		return NULL;
	}
	for (Point *p = points; p < points + *n; p++) {
		if (fread(p, sizeof(Point), 1, input) != 1) {
			fprintf(stderr, "Error reading file `%s'\n", filename);
			MPI_Finalize();
			exit(EXIT_FAILURE);
		}
	}
	return points;
}

static Point *readFileParallel(char *filename, int *n, int rank, int size)
{
	MPI_File input;
	int ret = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &input);
	if (ret != MPI_SUCCESS) {
		if (rank == 0) {
			char buf[1024];
			int len;
			MPI_Error_string(ret, buf, &len);
			fprintf(stderr, "File `%s' can't be opened: %s.\n", filename, buf);
		}
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}
	MPI_Status status;
	long count;
	MPI_File_read(input, &count, 1, MPI_LONG, &status);
	*n = count / size;
	if (count % size) {
		*n += size - count % size;
	}
	MPI_File_seek_shared(input, sizeof(count), MPI_SEEK_SET);

	MPI_Datatype mpiPoint;
	initPointMPI(&mpiPoint);

	Point *points = malloc(*n * sizeof(Point));
	assert(points);
	memset(points, -1, *n * sizeof(Point));

	MPI_File_read_ordered(input, points, *n, mpiPoint, &status);
	MPI_File_close(&input);
	freePointMPI(&mpiPoint);
	return points;
}

static void sequentSort(char *filename, int rank, pointsComparator cmp)
{
	if (rank != MASTER) {
		return;
	}
	int n;
	Point *points;
	void readWrapper()
	{
		points = readFileSequent(filename, &n);
	}
	void timsortWrapper()
	{
		timsort(points, n, sizeof(Point), cmp);
	}		void sortWrapper()	{		qsort(points, n, sizeof(Point), cmp);	}	
	void verifyWrapper()
	{
		int r = sorted(points, n, cmp);
		if (r) {
			printf("verify qsort: fail\n");
		}
		else {
			printf("verify qsort: ok\n");
		}
	}
	timer(rank, "sequential read", readWrapper);
	if (points == NULL) {
		return;
	}	timer(rank, "timsort", timsortWrapper);	free(points);	timer(rank, "sequential read", readWrapper);	if (points == NULL) {		return;	}	timer(rank, "qsort", sortWrapper);
	timer(rank, "sequential verify", verifyWrapper);
	free(points);
}

void sortPoints(char *filename, int rank, int size, pointsComparator cmp)
{
	MPI_Datatype mpiPoint;
	initPointMPI(&mpiPoint);
	int n;
	Point *points;
	void readWrapper()
	{
		points = readFileParallel(filename, &n, rank, size);
	}
	void batcherSortWrapper()
	{
		batcherSort(&points, n, rank, size, cmp);
	}

	void verifyWrapper()
	{
		verify(points, n, rank, size, cmp);
	}

	timer(rank, "parallel read", readWrapper);
	if (rank == MASTER) {
		printf("sorting %d points\n", n * size);
	}
	
	timer(rank, "batcher sort", batcherSortWrapper);
	timer(rank, "verify", verifyWrapper);
	free(points);
	freePointMPI(&mpiPoint);
	sequentSort(filename, rank, cmp);
}
