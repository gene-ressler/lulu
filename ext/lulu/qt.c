/*
 * qt.c
 *
 *  Created on: Mar 17, 2014
 *      Author: generessler
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "qt.h"
#include "utility.h"
#include "test.h"

// Declare the given quadrant of a given bounding box.
#define QUADRANT_DECL(Q, QX, QY, QW, QH, X, Y, W, H) \
  MARKER_DISTANCE QW = W * 0.5; \
  MARKER_DISTANCE QH = H * 0.5; \
  MARKER_COORD QX = (Q & 1) ? X + QW : X; \
  MARKER_COORD QY = (Q & 2) ? Y + QH : Y

// Initialize a node to an empty leaf.
static void init_leaf(NODE *node) {
    node->children = NULL;
    node->markers = NULL;
    node->marker_count = node->markers_size = 0;
}

// Clear contents of a leaf, returning it to the init_leaf state.
static void clear_leaf(NODE *node) {
    Free(node->markers);
    init_leaf(node);
}

// Make a leaf into an internal node with four empty leaves.
static void subdivide(NODE *node) {
    if (leaf_p(node)) {
        NewArray(node->children, 4);
        for (int i = 0; i < 4; i++)
            init_leaf(node->children + i);
    }
}

static void clear_node(NODE *node);

// Recursively clear an internal node by removing all
// subtrees, returning this node to the init_leaf state.
static void clear_internal(NODE *node) {
    for (int i = 0; i < 4; i++)
        clear_node(node->children + i);
    Free(node->children);
    clear_leaf(node);
}

// Clear any node, returning it to the init_leaf state.
// Recursively removes all subtrees.
static void clear_node(NODE *node) {
    if (internal_p(node))
        clear_internal(node);
    else
        clear_leaf(node);
}

// Add a marker to the given node's marker list.
static void add_marker(NODE *node, MARKER *marker) {
    if (node->marker_count == node->markers_size) {
        node->markers_size = 2 + 2 * node->markers_size;
        RenewArray(node->markers, node->markers_size);
    }
    node->markers[node->marker_count++] = marker;
}

// Find a marker in the given node's marker list.
static int find_marker(NODE *node, MARKER *marker) {
    for (int i = 0; i < node->marker_count; i++)
        if (node->markers[i] == marker)
            return i;
    return -1;
}

// Delete a marker from the given node's marker list.
static int delete_marker(NODE *node, MARKER *marker) {
    int i = find_marker(node, marker);
    if (i != -1 && --node->marker_count != 0)
        node->markers[i] = node->markers[node->marker_count];
    return i;
}

// Return non-zero iff the given bounding box lies inside the marker including its boundary.
static int bounds_inside_marker(MARKER_COORD x, MARKER_COORD y, MARKER_DISTANCE w, MARKER_DISTANCE h, MARKER *marker) {
    MARKER_COORD mx = mr_x(marker);
    MARKER_COORD my = mr_y(marker);
    MARKER_DISTANCE mr = mr_r(marker);
    return mx - mr <= x && x + w <= mx + mr && my - mr <= y && y + h <= my + mr;
}

// Return an integer code with bits showing which quadrants of the given
// bounding box are overlapped by the given marker.
static int touch_code(MARKER_COORD x, MARKER_COORD y, MARKER_DISTANCE w, MARKER_DISTANCE h, MARKER *marker) {
    MARKER_COORD xm = x + 0.5 * w;
    MARKER_COORD ym = y + 0.5 * h;
    int code = bit(SW) | bit(SE) | bit(NW) | bit(NE);
    if (mr_e(marker) < xm) code &= ~(bit(NE) | bit(SE));
    if (mr_w(marker) > xm) code &= ~(bit(NW) | bit(SW));
    if (mr_n(marker) < ym) code &= ~(bit(NW) | bit(NE));
    if (mr_s(marker) > ym) code &= ~(bit(SW) | bit(SE));
    return code;
}

// Insert the given marker into the quadtree with given root and corresponding bounding box,
// subdividing no more than the given number of levels.
static void insert(NODE *node, int levels, MARKER_COORD x, MARKER_COORD y, MARKER_DISTANCE w, MARKER_DISTANCE h, MARKER *marker) {
    if (bounds_inside_marker(x, y, w, h, marker) || levels == 0)
        add_marker(node, marker);
    else {
        if (leaf_p(node))
            subdivide(node);
        int code = touch_code(x, y, w, h, marker);
        for (int q = 0; q < 4; q++)
            if (code & bit(q)) {
                QUADRANT_DECL(q, qx, qy, qw, qh, x, y, w, h);
                insert(node->children + q, levels - 1, qx, qy, qw, qh, marker);
            }
    }
}

// A helper function that returns true iff the given array of 4 child quadtrees are all empty leaves.
static int empty_leaves_p(NODE *children) {
    for (int i = 0; i < 4; i++)
        if (internal_p(children + i) || children[i].marker_count > 0)
            return 0;
    return 1;
}

// Delete the given marker from the quadtree with given root and corresponding bounding box,
// trimming any remaining empty leaves.
static void delete(NODE *node, int levels,
        MARKER_COORD x, MARKER_COORD y, MARKER_DISTANCE w, MARKER_DISTANCE h,
        MARKER *marker) {
    if (bounds_inside_marker(x, y, w, h, marker) || levels == 0)
        delete_marker(node, marker);
    else if (internal_p(node)){
        int code = touch_code(x, y, w, h, marker);
        for (int q = 0; q < 4; q++)
            if (code & bit(q)) {
                QUADRANT_DECL(q, qx, qy, qw, qh, x, y, w, h);
                delete(node->children + q, levels - 1, qx, qy, qw, qh, marker);
            }
        if (empty_leaves_p(node->children))
            Free(node->children);
    }
}

// Local struct to hold information about the nearest marker seen so far in a search.
struct nearest_info {
    MARKER_INFO *info;
    MARKER *target, *nearest;
    MARKER_DISTANCE distance;
};

// Use the marker list of the given node to update nearest information with
// respect to the given marker.
static void update_nearest(NODE *node, struct nearest_info *nearest_info) {
    for (int i = 0; i < node->marker_count; i++) {
        // This < assumes the markers are in an array. It sustains the invariant.
        if (node->markers[i] < nearest_info->target) {
            MARKER_DISTANCE d = mr_distance(nearest_info->info, nearest_info->target, node->markers[i]);
            if (d < nearest_info->distance) {
                nearest_info->distance = d;
                nearest_info->nearest = node->markers[i];
            }
        }
    }
}

// Search out the nearest marker that overlaps the given one.  This just visits every
// quad that overlaps the given marker and remembers the closest marker it sees. The
// circle distance function renders quite impossible the ruling out of quads as in
// nearest point neighbor search.
static void search_for_nearest(NODE *node,
        MARKER_COORD x, MARKER_COORD y, MARKER_DISTANCE w, MARKER_DISTANCE h,
        struct nearest_info *nearest_info) {
    update_nearest(node, nearest_info);
    if (internal_p(node)) {
        // Search the children that include some part of the marker.
        int code = touch_code(x, y, w, h, nearest_info->target);
        for (int q = 0; q < 4; q++)
            if (code & bit(q)) {
                QUADRANT_DECL(q, qx, qy, qw, qh, x, y, w, h);
                search_for_nearest(node->children + q, qx, qy, qw, qh, nearest_info);
            }
    }
}

static MARKER *nearest(MARKER_INFO *info, NODE *node,
        MARKER_COORD x, MARKER_COORD y, MARKER_DISTANCE w, MARKER_DISTANCE h,
        MARKER *marker) {
    struct nearest_info nearest_info[1] = {{ info, marker, NULL, 0 }};
    search_for_nearest(node, x, y, w, h, nearest_info);
    return nearest_info->nearest;
}

void qt_init(QUADTREE *qt) {
    init_leaf(qt->root);
    qt->x = qt->y = qt->w = qt->h = 0;
}

void qt_setup(QUADTREE *qt, int max_depth,
        MARKER_COORD x, MARKER_COORD y, MARKER_DISTANCE w, MARKER_DISTANCE h,
        MARKER_INFO *info) {
    qt->x = x;
    qt->y = y;
    qt->w = w;
    qt->h = h;
    qt->max_depth = max_depth;
    qt->info = info;
}

void qt_clear(QUADTREE *qt) {
    clear_node(qt->root);
    qt_init(qt);
}

void qt_insert(QUADTREE *qt, MARKER *marker) {
    MARKER_COORD x = mr_x(marker);
    MARKER_COORD y = mr_y(marker);
    MARKER_DISTANCE r = mr_r(marker);
    if (x + r >= qt->x && x - r <= qt->x + qt->w && y + r >= qt->y && y - r <= qt->y + qt->h)
        insert(qt->root, qt->max_depth, qt->x, qt->y, qt->w, qt->h, marker);
}

void qt_delete(QUADTREE *qt, MARKER *marker) {
    delete(qt->root, qt->max_depth, qt->x, qt->y, qt->w, qt->h, marker);
}

MARKER *qt_nearest(QUADTREE *qt, MARKER *marker) {
    return nearest(qt->info, qt->root, qt->x, qt->y, qt->w, qt->h, marker);
}

int qt_nearest_wrt(MARKER *markers, QUADTREE *qt, int a) {
    MARKER *nearest = qt_nearest(qt, markers + a);
    return nearest ? (int)(nearest - markers) : -1;
}
