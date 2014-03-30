/*
 * test.c
 *
 *  Created on: Mar 21, 2014
 *      Author: generessler
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "test.h"
#include "utility.h"

void emit_rectangle(FILE *f, double x, double y, double w, double h) {
	fprintf(f, "  { kind: 'r', color: 'lightGray', p0: { x: %.2f, y: %.2f }, p1: { x: %.2f, y: %.2f } },\n", x, y, x + w, y + h);
}

void emit_segment(FILE *f, MARKER *a, MARKER *b) {
	fprintf(f, "  { kind: 's', color: 'red', p0: { x: %.2f, y: %.2f }, p1: { x: %.2f, y: %.2f } },\n",
			mr_x(a), mr_y(a), mr_x(b), mr_y(b));
}

int emit_marker_ptr_array(FILE *f, MARKER **markers, int n_markers) {
	int n_emitted = 0;
	for (int i = 0; i < n_markers; ++i) {
		MARKER *m = markers[i];
		if (!m->deleted_p) {
			fprintf(f, "  { kind: 'm', x: %.2f, y: %.2f, r: %.2f }, // %d\n", mr_x(m), mr_y(m), mr_r(m), i);
			n_emitted++;
		}
	}
	return n_emitted;
}

int emit_marker_array(FILE *f, MARKER *markers, int n_markers) {
	int n_emitted = 0;
	for (int i = 0; i < n_markers; ++i) {
		MARKER *m = markers + i;
		if (!m->deleted_p) {
			fprintf(f, "  { x: %.2f, y: %.2f, r: %.2f }, // %d\n", mr_x(m), mr_y(m), mr_r(m), i);
			n_emitted++;
		}
	}
	return n_emitted;
}

int emit_markers(char *name, MARKER *markers, int n_markers) {
	char buf[1024];
	sprintf(buf, "test/%s.js", name);
	FILE *f = fopen(buf, "w");
	if (!f)
		return -1;
	fprintf(f, "var %s = [\n", name);
	int n_emitted = emit_marker_array(f, markers, n_markers);
	fprintf(f, "];");
	fclose(f);
	return n_emitted;
}

void set_random_markers(MARKER_INFO *info, MARKER *markers, int n_markers) {
	unsigned t = time(0);
	fprintf(stderr, "seed=%u\n", t);
	srandom(t);
	int size = 8;
	double x_size = 1024;
	double y_size = 760;
	for (int i = 0; i < n_markers; i++)
		mr_set(info, markers + i, x_size * rand_double(), y_size * rand_double(), 1 + random() % (size - 1));
}

int trace_p = 1;

void trace(const char *fmt, ...)
{
	if (trace_p) {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}
