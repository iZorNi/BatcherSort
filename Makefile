CC = gcc

BGCC = mpixlc_r
MPICC = mpicc


CFLAGS = -std=gnu99\
	-ggdb\
	-Wall -Wextra\
	-O3\
	-Iinclude


BGFLAGS = -O5\
	-qsmp=omp\
	-Iinclude

BATCHER = $(shell find src/sort -name '*.c')

GEN = src/gen/gen.c src/sort/point.c

gen/bin:
	$(CC) $(CFLAGS) $(GEN) -o bin/gen	
gen: gen/bin
	$(PWD)/scripts/gen.sh $(PWD)/bin/gen $(PWD)/data
mpi: 
	$(MPICC) $(CFLAGS) $(BATCHER) -o bin/batcher
bg: 
	$(BGCC) $(BGFLAGS) $(BATCHER) -o bin/batcher

clean:
	rm bin/*
	rm data/*


