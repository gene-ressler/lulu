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
	// fprintf(stderr, "max_depth = %d\n", max_depth);
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
		mr_merge(info, markers + aa, markers + a, markers + b);

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

/**
 * Return the arg min and min distance marker from the one at position x,
 * looking upward, i.e. in [x+1..n).
 */
static void get_mindist(MARKER_INFO *info, MARKER *markers, int x, int n, int *argmin_rtn,
		DISTANCE *min_rtn) {
	int argmin = x + 1;
	DISTANCE min = mr_distance(info, markers + x, markers + argmin);
	for (int y = x + 2; y < n; y++) {
		double d = mr_distance(info, markers + x, markers + y);
		if (d < min) {
			argmin = y;
			min = d;
		}
	}
	*argmin_rtn = argmin;
	*min_rtn = min;
}

/**
 * Merge the markers in the array until they don't overlap any more.
 * Slots no longer needed are marked deleted.
 *
 * This algorithm is O(n^3) worst case and O(n^2) for typical data.
 *
 * It is slightly adapted from the "generic" algorithm in
 *
 * Daniel MŸllner,
 * "Modern hierarchical, agglomerative clustering algorithms"
 * http://arxiv.org/abs/1109.2378
 */
void merge_markers(MARKER_INFO *info, MARKER *markers, int n_markers) {
	// 5-8
	int *n_nghbr;
	NewArray(n_nghbr, n_markers - 1);
	DISTANCE *mindist;
	NewArray(mindist, n_markers - 1);
	for (int x = 0; x < n_markers - 1; x++)
		get_mindist(info, markers, x, n_markers, n_nghbr + x, mindist + x);

	// 2, 9
	PRIORITY_QUEUE pq[1];
	pq_init(pq);
	pq_set_up(pq, mindist, n_markers - 1);

	// 10
	for (int i = 1; i < n_markers; i++) {

		// 11
		int a = pq_peek_min(pq);

		// 12
		int b = n_nghbr[a];

		// 13
		DISTANCE delta = mindist[a];

		// 14
		while (delta != mr_distance(info, markers + a, markers + b)) {

			// 15-16
			get_mindist(info, markers, a, n_markers, n_nghbr + a, mindist + a);
			pq_update(pq, a);

			// 17
			a = pq_peek_min(pq);

			// 18
			b = n_nghbr[a];

			// 19
			delta = mindist[a];
		} // 20

		if (delta > 0)
			break;

		// 21, 24
		a = pq_get_min(pq);

		// 23, 25-27
		mr_merge(info, markers + b, markers + b, markers + a);
		mr_set_deleted(markers + a);

		// 28, 33
		for (int j = 0; j < pq_index_set_size(pq); j++) {
			int x = pq_index(pq, j);

			// 29-30
			if (x < a && n_nghbr[x] == a)
				n_nghbr[x] = b;

			if (x < b) {

				// 34
				double d = mr_distance(info, markers + x, markers + b);
				if (d < mindist[x]) {

					// 35
					n_nghbr[x] = b;

					// 36
					mindist[x] = d;
					pq_update(pq, x);
				}
			}
		}

		// 39
		get_mindist(info, markers, b, n_markers, n_nghbr + b, mindist + b);
		pq_update(pq, b);
	}
	pq_clear(pq);
	Free(n_nghbr);
	Free(mindist);
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
	// merge_markers(info, markers, size);

	gettimeofday(stop, NULL);

	int n = emit_markers("after", markers, size);
	fprintf(stderr, "after merge: %d\n", n);

	timersub(stop, start, diff);
	fprintf(stderr, "%.3f seconds", diff->tv_sec + 1.0e-6 * diff->tv_usec);

	return EXIT_SUCCESS;
}

#endif
