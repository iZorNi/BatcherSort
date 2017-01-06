#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "point.h"

int main(int argc, char **argv)
{
	if (argc != 4) {
		fprintf(stderr, "Usage: gen <count> <dev> <output file>\n");
		exit(EXIT_FAILURE);
	}
	errno = 0;
	char *endptr;
	char *str = argv[1];
	long count = strtol(argv[1], &endptr, 10);
	if ((errno == ERANGE && (count == LONG_MAX || count == LONG_MIN)) || (errno != 0 && count == 0)) {
		perror("strtol");
		exit(EXIT_FAILURE);
	}
	if (endptr == str) {
		fprintf(stderr, "No digits were found\n");
		exit(EXIT_FAILURE);
	}
	str = argv[2];
	errno = 0;
	float dev = strtof(argv[2], &endptr);
	if ((errno == ERANGE && (dev == HUGE_VALF)) || (errno != 0 && dev == 0)) {
		perror("strtof");
		exit(EXIT_FAILURE);
	}
	if (endptr == str) {
		fprintf(stderr, "No digits were found\n");
		exit(EXIT_FAILURE);
	}

	FILE *output = fopen(argv[3], "wb");
	if (output == NULL) {
		fprintf(stderr, "Can't create file `%s'\n", argv[3]);
		exit(EXIT_FAILURE);
	}

	srand(time(NULL));
	fwrite(&count, sizeof(count), 1, output);
	for (long i = 0; i < count; i++) {
		Point p = getRandomPoint(dev);
		if (fwrite(&p, sizeof(Point), 1, output) != 1) {
			fprintf(stderr, "Error writing file `%s'\n", argv[3]);
			exit(EXIT_FAILURE);
		}
	}
	fclose(output);

	return 0;
}
