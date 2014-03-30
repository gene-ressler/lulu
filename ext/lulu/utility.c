/*
 * utility.c
 *
 *  Created on: Feb 23, 2014
 *      Author: generessler
 */

#include <stdio.h>
#include <stdlib.h>
#include "utility.h"

void *safe_malloc(size_t size, const char *file, int line) {
	void *p = malloc(size);
	if (!p) {
		fprintf(stderr, "%s:%d: out of memory\n", file, line);
		exit(1);
	}
	return p;
}

void *safe_realloc(void *p, size_t size, const char *file, int line) {
	p = realloc(p, size);
	if (!p) {
		fprintf(stderr, "%s:%d: out of memory\n", file, line);
		exit(1);
	}
	return p;
}

/**
 * Return the 0-based position of highest bit or -1 of zero.
 */
int high_bit_position(unsigned n) {
	int p = -1;
	while (n) {
		p++;
		n >>= 1;
	}
	return p;
}

double rand_double(void)
{
	unsigned i = random();
	i = (i << 8) ^ random();
	i = (i << 8) ^ random();
	i = (i << 8) ^ random();
	return i / (256.0 * 256.0 * 256.0 * 256.0);
}
