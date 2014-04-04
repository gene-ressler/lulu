/*
 * marker.h
 *
 *  Created on: Feb 24, 2014
 *      Author: generessler
 */

#ifndef MARKER_H_
#define MARKER_H_

typedef double MARKER_COORD;
typedef double MARKER_DISTANCE;
/**
 * Size of population represented by the marker.  For
 * discrete populations, this can be an unsigned int.
 */
typedef double MARKER_SIZE;

typedef struct marker_s {
    MARKER_SIZE size;
    MARKER_DISTANCE r;
    MARKER_COORD x, y, x_sum, y_sum;
    int part_a;
    unsigned deleted_p:1, part_b:31;
} MARKER;

typedef enum marker_kind_e {
    CIRCLE,
    SQUARE,
} MARKER_KIND;

/**
 * Holds parameters of the distance function and merging.
 */
typedef struct marker_type_info_s {
    MARKER_KIND kind;
    // User scale applied to radii of markers during distance computation.
    MARKER_DISTANCE scale;
    // A scale factor that depends on both kind and user scale.
    MARKER_DISTANCE c;
} MARKER_INFO;

#define MARKER_INFO_DECL(I) MARKER_INFO I[1]; mr_info_init(I)

typedef struct marker_extent_s {
    MARKER_COORD x, y, w, h;
} MARKER_EXTENT;

#define mr_deleted_p(M)     ((M)->deleted_p)
#define mr_set_deleted(M)   do { (M)->deleted_p = 1; } while (0)
#define mr_merged(M)        ((M)->part_a >= 0)
#define mr_x(M) ((M)->x)
#define mr_y(M) ((M)->y)
#define mr_r(M) ((M)->r)
#define mr_w(M) ((M)->x - (M)->r)
#define mr_e(M) ((M)->x + (M)->r)
#define mr_s(M) ((M)->y - (M)->r)
#define mr_n(M) ((M)->y + (M)->r)

void mr_init(MARKER *marker, int n_markers);
void mr_reset_parts(MARKER *marker);
void mr_info_init(MARKER_INFO *info);
void mr_info_set(MARKER_INFO *info, MARKER_KIND kind, MARKER_DISTANCE scale);
void mr_set(MARKER_INFO *info, MARKER *marker, MARKER_COORD x, MARKER_COORD y, MARKER_SIZE size);
void mr_merge(MARKER_INFO *info, MARKER *markers, int merged, int a, int b);
MARKER_DISTANCE mr_distance(MARKER_INFO *info, MARKER *a, MARKER *b);
MARKER_DISTANCE size_to_radius(MARKER_INFO *info, MARKER_SIZE size);
void get_marker_array_extent(MARKER *a, int n_markers, MARKER_EXTENT *ext);

#endif /* MARKER_H_ */
