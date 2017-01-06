#include <mpi.h>

#include "point.h"
#include "schedule.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: batcher <input file>\n");
		exit(EXIT_FAILURE);
	}
	MPI_Init(&argc, &argv);
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	sortPoints(argv[1], rank, size, pointCmpX);
	MPI_Finalize();
	return 0;
}
