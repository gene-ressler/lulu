/*
 * test.h
 *
 *  Created on: Mar 21, 2014
 *      Author: generessler
 */

#ifndef TEST_H_
#define TEST_H_

#ifdef UNIT_TESTS

#include "marker.h"

int emit_markers(const char *name, MARKER *markers, int n_markers);
void emit_rectangle(FILE *f, double x, double y, double w, double h);
void emit_segment(FILE *f, MARKER *a, MARKER *b);
int emit_marker_array(FILE *f, MARKER *markers, int n_markers);
int emit_marker_ptr_array(FILE *f, MARKER **markers, int n_markers);
double rand_double(void);
void set_random_markers(MARKER_INFO *info, MARKER *markers, int n_markers);
void qt_clear(QUADTREE *qt);
int qt_test(int size);
int pq_test(int size);
int merge_test(int test_markers_size);

#endif

#endif /* TEST_H_ */
