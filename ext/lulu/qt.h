/*
 * qt.h
 *
 *  Created on: Mar 17, 2014
 *      Author: generessler
 */

#ifndef QT_H_
#define QT_H_

#include "marker.h"

typedef struct node_s {
	struct node_s *children;
	MARKER **markers;
	int marker_count, markers_size;
} NODE;

#define leaf_p(Node) ((Node)->children == NULL)
#define internal_p(Node) ((Node)->children != NULL)

// These are chosen so the 1's bit encodes the
// quadrant x location and the 2's bit encodes y
//
//  ---------------
// |       |       |
// |  1 0  |  1 1  |
// |       |       |
//  ---------------
// |       |       |
// |  0 0  |  0 1  |
// |       |       |
//  ---------------
#define SW 0
#define SE 1
#define NW 2
#define NE 3

typedef struct quadtree_s {
	double x, y, w, h;
	int max_depth;
	MARKER_INFO *info;
	NODE root[1];
} QUADTREE;

#define QUADTREE_DECL(Name) QUADTREE Name[1]; qt_init(Name)

void qt_init(QUADTREE *qt);
void qt_setup(QUADTREE *qt, int max_depth, double x, double y, double w, double h, MARKER_INFO *info);
void qt_insert(QUADTREE *qt, MARKER *marker);
void qt_delete(QUADTREE *qt, MARKER *marker);
MARKER *qt_nearest(QUADTREE *qt, MARKER *marker);
int qt_nearest_wrt(MARKER *markers, QUADTREE *qt, int a);
void qt_clear(QUADTREE *qt);
int qt_test(int size);

#endif /* QT_H_ */
