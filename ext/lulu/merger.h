/*
 * merger.h
 *
 *  Created on: Feb 24, 2014
 *      Author: generessler
 */

#ifndef MERGER_H_
#define MERGER_H_

#include "namespace.h"
#include "marker.h"

#define merge_markers_fast(Info, Markers, MarkersSize)  NAME(merge_markers_fast)(Info, Markers, MarkersSize)
int merge_markers_fast(MARKER_INFO *info, MARKER *markers, int markers_size);

#endif /* MERGER_H_ */
