#ifndef POINT_MPI_H
#define POINT_MPI_H

#include <mpi.h>

int initPointMPI(MPI_Datatype *datatype);
int freePointMPI(MPI_Datatype *datatype);

#endif
