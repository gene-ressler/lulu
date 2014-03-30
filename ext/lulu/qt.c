/*
 * qt.c
 *
 *  Created on: Mar 17, 2014
 *      Author: generessler
 */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>
#include "qt.h"
#include "utility.h"
#include "test.h"

// Change a bounding box to the given quadrant of itself.
#define TO_QUADRANT(Q, X, Y, W, H) do { \
  W *= 0.5; \
  H *= 0.5; \
  if (Q & 1) X += W; \
  if (Q & 2) Y += H; \
} while (0)

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
static int bounds_inside_marker(double x, double y, double w, double h, MARKER *marker) {
	double mx = mr_x(marker);
	double my = mr_y(marker);
	double mr = mr_r(marker);
	return mx - mr <= x && x + w <= mx + mr && my - mr <= y && y + h <= my + mr ;
}

// Return an integer code with bits showing which quadrants of the given
// bounding box are overlapped by the given marker.
static int touch_code(double x, double y, double w, double h, MARKER *marker) {
	double xm = x + 0.5 * w;
	double ym = y + 0.5 * h;
	int code = bit(SW) | bit(SE) | bit(NW) | bit(NE);
	if (mr_e(marker) < xm) code &= ~(bit(NE) | bit(SE));
	if (mr_w(marker) > xm) code &= ~(bit(NW) | bit(SW));
	if (mr_n(marker) < ym) code &= ~(bit(NW) | bit(NE));
	if (mr_s(marker) > ym) code &= ~(bit(SW) | bit(SE));
	return code;
}

// Insert the given marker into the quadtree with given root and corresponding bounding box,
// subdividing no more than the given number of levels.
static void insert(NODE *node, int levels, double x, double y, double w, double h, MARKER *marker) {
	if (bounds_inside_marker(x, y, w, h, marker) || levels == 0)
		add_marker(node, marker);
	else {
		if (leaf_p(node))
			subdivide(node);
		int code = touch_code(x, y, w, h, marker);
		for (int q = 0; q < 4; q++)
			if (code & bit(q)) {
				double xx = x, yy = y, ww = w, hh = h;
				TO_QUADRANT(q, xx, yy, ww, hh);
				insert(node->children + q, levels - 1, xx, yy, ww, hh, marker);
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
static void delete(NODE *node, int levels, double x, double y, double w, double h, MARKER *marker) {
	if (bounds_inside_marker(x, y, w, h, marker) || levels == 0)
		delete_marker(node, marker);
	else if (internal_p(node)){
		int code = touch_code(x, y, w, h, marker);
		for (int q = 0; q < 4; q++)
			if (code & bit(q)) {
				double xx = x, yy = y, ww = w, hh = h;
				TO_QUADRANT(q, xx, yy, ww, hh);
				delete(node->children + q, levels - 1, xx, yy, ww, hh, marker);
			}
		if (empty_leaves_p(node->children))
			Free(node->children);
	}
}

// Local struct to hold information about the nearest marker seen so far in a search.
struct nearest {
	double distance;
	MARKER *marker;
};

// Use the marker list of the given node to update nearest information with
// respect to the given marker.
static void update_nearest(MARKER_INFO *info, NODE *node, MARKER *marker, struct nearest *nearest) {
	for (int i = 0; i < node->marker_count; i++) {
		if (node->markers[i] != marker) {
			double d = mr_distance(info, marker, node->markers[i]);
			if (d < 0 && d < nearest->distance) {
				nearest->distance = d;
				nearest->marker = node->markers[i];
			}
		}
	}
}

// Search out the nearest marker that overlaps the given one.  This just visits every
// quad that overlaps the given marker and remembers the closest marker it sees. The
// circle distance function renders quite impossible the ruling out of quads as in
// nearest point neighbor search.
static void search_for_nearest(MARKER_INFO *info, NODE *node, double x, double y, double w, double h, MARKER *marker, struct nearest *nearest) {
	update_nearest(info, node, marker, nearest);
	if (internal_p(node)) {
		// Search the children that include some part of the marker.
		int code = touch_code(x, y, w, h, marker);
		for (int q = 0; q < 4; q++)
			if (code & bit(q)) {
				double xx = x, yy = y, ww = w, hh = h;
				TO_QUADRANT(q, xx, yy, ww, hh);
				search_for_nearest(info, node->children + q, xx, yy, ww, hh, marker, nearest);
			}
	}
}

static MARKER *nearest(MARKER_INFO *info, NODE *node, double x, double y, double w, double h, MARKER *marker) {
	struct nearest nearest[1] = {{ DBL_MAX, NULL }};
	search_for_nearest(info, node, x, y, w, h, marker, nearest);
	return nearest->marker;
}

void qt_init(QUADTREE *qt) {
	init_leaf(qt->root);
	qt->x = qt->y = qt->w = qt->h = 0;
}

void qt_setup(QUADTREE *qt, int max_depth, double x, double y, double w, double h, MARKER_INFO *info) {
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
	double x = mr_x(marker);
	double y = mr_y(marker);
	double r = mr_r(marker);
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
	return nearest ? nearest - markers : -1;
}

#ifndef EXCLUDE_UNIT_TEST

static void draw(FILE *f, NODE *node, double x, double y, double w, double h) {
	emit_rectangle(f, x, y, w, h);
	emit_marker_ptr_array(f, node->markers, node->marker_count);
	if (internal_p(node)) {
		for (int q = 0; q < 4; q++) {
			double xx = x, yy = y, ww = w, hh = h;
			TO_QUADRANT(q, xx, yy, ww, hh);
			draw(f, node->children + q, xx, yy, ww, hh);
		}
	}
}

static void draw_nearest(FILE *f, MARKER *markers, MARKER **nearest_markers, int n_markers) {
	for (int i = 0; i < n_markers; i++) {
		if (nearest_markers[i])
			emit_segment(f, markers + i, nearest_markers[i]);
	}
}

int qt_draw(QUADTREE *qt, MARKER *markers, MARKER **nearest_markers, int n_markers, char *name) {
	char buf[1024];
	sprintf(buf, "test/%s.js", name);
	FILE *f = fopen(buf, "w");
	if (!f)
		return -1;
	fprintf(f, "var %s = [\n", name);
	draw(f, qt->root, qt->x, qt->y, qt->w, qt->h);
	draw_nearest(f, markers, nearest_markers, n_markers);
	fprintf(f, "];\n");
	fclose(f);
	return EXIT_SUCCESS;
}

int qt_test(int size) {
	QUADTREE_DECL(qt);
	MARKER_INFO_DECL(info);
	MARKER *markers, **nearest_markers;
	NewArray(markers, size);
	NewArray(nearest_markers, size);
	set_random_markers(info, markers, size);
	qt_setup(qt, 5, 0, 0, 1024, 724, info);
	fprintf(stderr, "inserting %d:\n", size);
	for (int i = 0; i < size; i++) {
		qt_insert(qt, markers + i);
	}
	fprintf(stderr, "inserted %d\n", size);
	for (int i = 0; i < size; i++) {
		nearest_markers[i] = qt_nearest(qt, markers + i);
	}
	fprintf(stderr, "looked up %d\n", size);
	qt_draw(qt, markers, nearest_markers, size, "qt");
	fprintf(stderr, "drew %d\n", size);
	for (int i = 0; i < size; i++) {
		qt_delete(qt, markers + i);
	}
	fprintf(stderr, "after delete all, root is %s\n",
			leaf_p(qt->root) ? "leaf (ok)" : "internal (not ok)");

	return EXIT_SUCCESS;
}

#endif
