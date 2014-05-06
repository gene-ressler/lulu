/*
 * qt.h
 *
 *  Created on: Mar 17, 2014
 *      Author: generessler
 */

#ifndef QT_H_
#define QT_H_

#include "namespace.h"
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
    MARKER_COORD x, y;
    MARKER_DISTANCE w, h;
    int max_depth;
    MARKER_INFO *info;
    NODE root[1];
} QUADTREE;

#define QUADTREE_DECL(Name) QUADTREE Name[1]; qt_init(Name)

#define qt_init(T)  NAME(qt_init)(T)
void qt_init(QUADTREE *qt);

#define qt_setup(T, MaxDepth, X, Y, W, H, Info) NAME(qt_setup)(T, MaxDepth, X, Y, W, H, Info)
void qt_setup(QUADTREE *qt, int max_depth,
        MARKER_COORD x, MARKER_COORD y, MARKER_DISTANCE w, MARKER_DISTANCE h,
        MARKER_INFO *info);

#define qt_clear(T) NAME(qt_clear)(T)
void qt_clear(QUADTREE *qt);

#define qt_insert(T, Marker)    NAME(qt_insert)(T, Marker)
void qt_insert(QUADTREE *qt, MARKER *marker);

#define qt_delete(T, Marker)    NAME(qt_delete)(T, Marker)
void qt_delete(QUADTREE *qt, MARKER *marker);

#define qt_nearest(T, Marker)   NAME(qt_nearest)(T, Marker)
MARKER *qt_nearest(QUADTREE *qt, MARKER *marker);

#define qt_nearest_wrt(Markers, T, A)   NAME(qt_nearest_wrt)(Markers, T, A)
int qt_nearest_wrt(MARKER *markers, QUADTREE *qt, int a);

#endif /* QT_H_ */
