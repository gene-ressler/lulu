/*
 * merger.h
 *
 *  Created on: Feb 24, 2014
 *      Author: generessler
 */

#ifndef MERGER_H_
#define MERGER_H_

#include "marker.h"

void merge_markers(MARKER_INFO *info, MARKER *markers, int markers_size);
int merge_markers_fast(MARKER_INFO *info, MARKER *markers, int markers_size);
int merge_test(int test_markers_size);

#endif /* MERGER_H_ */
