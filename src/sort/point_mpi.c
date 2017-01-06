#include "point_mpi.h"

int initPointMPI(MPI_Datatype *datatype)
{
	int blocks[2] = {1, 2};
	MPI_Datatype types[2] = {MPI_INT, MPI_FLOAT};
	MPI_Aint intex;
	MPI_Type_extent(MPI_INT, &intex);
	MPI_Aint displacements[2];
	displacements[0] = 0;
	displacements[1] = intex;
	MPI_Type_struct(2, blocks, displacements, types, datatype);
	return MPI_Type_commit(datatype);
}

int freePointMPI(MPI_Datatype *datatype)
{
	return MPI_Type_free(datatype);
}
