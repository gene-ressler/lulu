/*
 * marker.c
 *
 *  Created on: Feb 24, 2014
 *      Author: generessler
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "marker.h"
#include "utility.h"

#define SQRT_1_PI 0.564189583547756286948079451560772585844050629328998856844085

void mr_info_init(MARKER_INFO *info) {
	info->c = SQRT_1_PI;
}

void mr_info_set(MARKER_INFO *info, MARKER_KIND kind, DISTANCE scale) {
	info->c = scale * (kind == SQUARE ? 0.5 : SQRT_1_PI);
}

void mr_init(MARKER *markers, int n_markers) {
	for (int i = 0; i < n_markers; i++) {
		MARKER *marker = markers + i;
		marker->deleted_p = 0;
		marker->size = 0;
		marker->r = 0;
		marker->x = marker->y = marker->x_sum = marker->y_sum = 0;
	}
}

void mr_set(MARKER_INFO *info, MARKER *marker, COORD x, COORD y, SIZE size) {
	mr_init(marker, 1);
	marker->size = size;
	marker->r = size_to_radius(info, size);
	marker->x = x;
	marker->y = y;
	marker->x_sum = x * size;
	marker->y_sum = y * size;
}

void mr_merge(MARKER_INFO *info, MARKER *merged, MARKER *a, MARKER *b) {
	merged->deleted_p = 0;
	merged->size = a->size + b->size;
	merged->r = size_to_radius(info, merged->size);
	merged->x_sum = a->x_sum + b->x_sum;
	merged->y_sum = a->y_sum + b->y_sum;
	merged->x = merged->x_sum / merged->size;
	merged->y = merged->y_sum / merged->size;
}

DISTANCE size_to_radius(MARKER_INFO *info, SIZE size) {
	return info->c * sqrt(size);
}

DISTANCE mr_distance(MARKER_INFO *info, MARKER *a, MARKER *b) {
	if (info->kind == SQUARE) {
		double r_sum = mr_r(a) + mr_r(b);
		double dx = fabs(mr_x(b) - mr_x(a)) - r_sum;
		double dy = fabs(mr_y(b) - mr_y(a)) - r_sum;
		return dx < 0 && dy < 0 ? fmax(dx, dy) : sqrt(dx * dx + dy * dy);
	}
	double dx = mr_x(b) - mr_x(a);
	double dy = mr_y(b) - mr_y(a);
	return sqrt(dx * dx + dy * dy) - mr_r(a) - mr_r(b);
}

void get_marker_array_extent(MARKER *a, int n_markers, MARKER_EXTENT *ext) {
	if (n_markers > 0) {
		double ew = mr_w(a);
		double ee = mr_e(a);
		double es = mr_s(a);
		double en = mr_n(a);
		for (int i = 1; i < n_markers; i++) {
			double w = mr_w(a + i);
			double e = mr_e(a + i);
			double s = mr_s(a + i);
			double n = mr_n(a + i);
			if (w < ew)
				ew = w;
			if (e > ee)
				ee = e;
			if (s < es)
				es = s;
			if (n > en)
				en = n;
		}
		ext->x = ew;
		ext->y = es;
		ext->w = ee - ew;
		ext->h = en - es;
	}
}
