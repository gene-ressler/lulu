/*
 * merger.c
 *
 *  Created on: Feb 24, 2014
 *      Author: generessler
 */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>
#include <time.h>
#include "merger.h"
#include "utility.h"
#include "pq.h"
#include "qt.h"
#include "test.h"

/**
 * Repeatedly merge the closest pair of the given markers until they don't overlap.
 *
 * The centroid rule is used for merging.  I.e. each marker's area represents
 * a population of points. Merging two markers removes the originals and creates a
 * new marker with area the sum of the originals and center at the average position
 * of the two merged populations.
 *
 * The markers array must include extra buffer space.  If there are n markers, the
 * array must have 2n-1 spaces, with only the first n initialized.
 *
 * The number of markers after merging is returned. Zero or more of these will be
 * marked deleted_p and should be ignored.
 *
 * This algorithm is O(n k log n), where k is the maximum number of simultaneously
 * overlapping markers in the original, unmerged set.
 */
int merge_markers_fast(MARKER_INFO *info, MARKER *markers, int n_markers) {
	int augmented_length = 2 * n_markers - 1;
	NewArrayDecl(int, n_nghbr, augmented_length);
	NewArrayDecl(DISTANCE, mindist, augmented_length);
	NewArrayDecl(int, inv_nghbr_head, augmented_length);
	NewArrayDecl(int, inv_nghbr_next, augmented_length);
	NewArrayDecl(int, tmp, augmented_length); // Too big
	// Do not free the following array!  It's owned by the priority queue.
	NewArrayDecl(int, heap, augmented_length); // Too big

	// Specialized quadtree supports finding closest marker to any given one.
	QUADTREE_DECL(qt);

	// Priority queue keyed on distances of overlapping pairs of markers.
	PRIORITY_QUEUE_DECL(pq);

	/// Extent of markers in the domain.
	MARKER_EXTENT ext[1];

	// Get a bounding box for the whole collection of markers.
	get_marker_array_extent(markers, n_markers, ext);

	// Set up the quadtree with the bounding box. Choose tree depth heuristically.
	int max_depth = high_bit_position(n_markers) / 4 + 3;
	qt_setup(qt, max_depth, ext->x, ext->y, ext->w, ext->h, info);

	// Insert all the markers in the quadtree.
	for (int i = 0; i < n_markers; i++)
		qt_insert(qt, markers + i);

	// Set all the inverse nearest neighbor links to null.
	for (int i = 0; i < augmented_length; i++)
		inv_nghbr_head[i] = inv_nghbr_next[i] = -1;

	// Initialize the heap by adding an index for each overlapping pair. The
	// The heap holds indices into the array of min-distance keys. An index for
	// pair a->bis added iff markers with indices a and b overlap and b < a.
	int heap_size = 0;
	for (int a = 0; a < n_markers; a++) {
		int b = qt_nearest_wrt(markers, qt, a);
		if (0 <= b && b < a) {
			n_nghbr[a] = b;
			mindist[a] = mr_distance(info, markers + a, markers + b);
			heap[heap_size++] = a;

			// Here we are building a linked list of markers that have b as nearest.
			inv_nghbr_next[a] = inv_nghbr_head[b];
			inv_nghbr_head[b] = a;
		}
	}

	// Now install the raw heap array into the priority queue. After this it's owned by the queue.
	pq_set_up_heap(pq, heap, heap_size, mindist, augmented_length); // Too big

	while (!pq_empty_p(pq)) {

		// Get nearest pair from priority queue.
		int a = pq_get_min(pq);
		int b = n_nghbr[a];

		// Delete both of the nearest pair from all data structures.
		pq_delete(pq, b);
		qt_delete(qt, markers + a);
		qt_delete(qt, markers + b);
		mr_set_deleted(markers + a);
		mr_set_deleted(markers + b);

		// Capture the inv lists of both a and b in tmp.
		int tmp_size = 0;
		for (int p = inv_nghbr_head[a]; p >= 0; p = inv_nghbr_next[p])
			if (!markers[p].deleted_p)
				tmp[tmp_size++] = p;
		for (int p = inv_nghbr_head[b]; p >= 0; p = inv_nghbr_next[p])
			if (!markers[p].deleted_p)
				tmp[tmp_size++] = p;

		// Create a new merged marker. Adding it after all others means
		// nothing already in the heap could have it as nearest.
		int aa = n_markers++;
		mr_merge(info, markers, aa, a, b);

		// Add to quadtree.
		qt_insert(qt, markers + aa);

		// Find nearest overlapping neighbor of the merged marker, if any.
		int bb = qt_nearest_wrt(markers, qt, aa);
		if (0 <= bb) {
			n_nghbr[aa] = bb;
			mindist[aa] = mr_distance(info, markers + aa, markers + bb);
			pq_add(pq, aa);
			inv_nghbr_next[aa] = inv_nghbr_head[bb];
			inv_nghbr_head[bb] = aa;
		}

		// Reset the nearest neighbors of the inverse neighbors of the deletions.
		for (int i = 0; i < tmp_size; i++) {
			int aa = tmp[i];
			int bb = qt_nearest_wrt(markers, qt, aa);
			if (0 <= bb && bb < aa) {
				n_nghbr[aa] = bb;
				mindist[aa] = mr_distance(info, markers + aa, markers + bb);
				pq_update(pq, aa);
				inv_nghbr_next[aa] = inv_nghbr_head[bb];
				inv_nghbr_head[bb] = aa;
			} else {
				pq_delete(pq, aa);
			}
		}
	}
	qt_clear(qt);
	pq_clear(pq);
	Free(n_nghbr);
	Free(mindist);
	Free(inv_nghbr_head);
	Free(inv_nghbr_next);
	Free(tmp);
	return n_markers;
}

#ifndef EXCLUDE_UNIT_TEST

#include <sys/time.h>

/**
 * Generate many random markers and then merge them. Write
 * results as a Javascript array to be included included in a test page.
 */
int merge_test(int size) {
	struct timeval start[1], stop[1], diff[1];

	MARKER_INFO_DECL(info);
	mr_info_set(info, CIRCLE, 2);

	NewArrayDecl(MARKER, markers, 2 * size - 1);
	mr_init(markers, 2 * size - 1);
	set_random_markers(info, markers, size);

	int m = emit_markers("before", markers, size);
	fprintf(stderr, "before merge: %d\nmerging...\n", m);

	gettimeofday(start, NULL);

	size = merge_markers_fast(info, markers, size);

	gettimeofday(stop, NULL);

	int n = emit_markers("after", markers, size);
	fprintf(stderr, "after merge: %d\n", n);

	timersub(stop, start, diff);
	fprintf(stderr, "%.3f seconds", diff->tv_sec + 1.0e-6 * diff->tv_usec);

	return EXIT_SUCCESS;
}

#endif
