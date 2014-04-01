/*
 * test.h
 *
 *  Created on: Mar 21, 2014
 *      Author: generessler
 */

#ifndef TEST_H_
#define TEST_H_

#include "marker.h"

int emit_markers(const char *name, MARKER *markers, int n_markers);
void emit_rectangle(FILE *f, double x, double y, double w, double h);
void emit_segment(FILE *f, MARKER *a, MARKER *b);
int emit_marker_array(FILE *f, MARKER *markers, int n_markers);
int emit_marker_ptr_array(FILE *f, MARKER **markers, int n_markers);
void set_random_markers(MARKER_INFO *info, MARKER *markers, int n_markers);

#endif /* TEST_H_ */
