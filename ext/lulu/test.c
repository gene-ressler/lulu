/*
 * test.c
 *
 *  Created on: Mar 21, 2014
 *      Author: generessler
 */

#ifdef UNIT_TESTS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include "test.h"
#include "utility.h"

double rand_double(void)
{
    unsigned i = rand();
    i = (i << 8) ^ (unsigned)rand();
    i = (i << 8) ^ (unsigned)rand();
    i = (i << 8) ^ (unsigned)rand();
    return i / (256.0 * 256.0 * 256.0 * 256.0);
}

void emit_rectangle(FILE *f, double x, double y, double w, double h) {
    fprintf(f, "  { kind: 'r', color: 'lightGray', p0: { x: %.2f, y: %.2f }, p1: { x: %.2f, y: %.2f } },\n", x, y, x + w, y + h);
}

void emit_segment(FILE *f, MARKER *a, MARKER *b) {
    fprintf(f, "  { kind: 's', color: 'red', p0: { x: %.2f, y: %.2f }, p1: { x: %.2f, y: %.2f } },\n",
            mr_x(a), mr_y(a), mr_x(b), mr_y(b));
}

int emit_marker_ptr_array(FILE *f, MARKER **markers, int n_markers) {
    int n_emitted = 0;
    for (int i = 0; i < n_markers; ++i) {
        MARKER *m = markers[i];
        if (!m->deleted_p) {
            fprintf(f, "  { kind: 'm', x: %.2f, y: %.2f, r: %.2f }, // %d\n", mr_x(m), mr_y(m), mr_r(m), i);
            n_emitted++;
        }
    }
    return n_emitted;
}

int emit_marker_array(FILE *f, MARKER *markers, int n_markers) {
    int n_emitted = 0;
    for (int i = 0; i < n_markers; ++i) {
        MARKER *m = markers + i;
        if (!m->deleted_p) {
            fprintf(f, "  { x: %.2f, y: %.2f, r: %.2f }, // %d\n", mr_x(m), mr_y(m), mr_r(m), i);
            n_emitted++;
        }
    }
    return n_emitted;
}

int emit_markers(const char *name, MARKER *markers, int n_markers) {
    char buf[1024];
    sprintf(buf, "test/%s.js", name);
    FILE *f = fopen(buf, "w");
    if (!f)
        return -1;
    fprintf(f, "var %s = [\n", name);
    int n_emitted = emit_marker_array(f, markers, n_markers);
    fprintf(f, "];");
    fclose(f);
    return n_emitted;
}

void set_random_markers(MARKER_INFO *info, MARKER *markers, int n_markers) {
    unsigned t = (unsigned)time(0);
    fprintf(stderr, "seed=%u\n", t);
    srand(t);
    int size = 8;
    double x_size = 1024;
    double y_size = 760;
    for (int i = 0; i < n_markers; i++)
        mr_set(info, markers + i, x_size * rand_double(), y_size * rand_double(), 1 + random() % (size - 1));
}

int trace_p = 1;

void trace(const char *fmt, ...)
{
    if (trace_p) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
}

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


/* Randomly permute the given distance array. */
void permute(PRIORITY_QUEUE_VALUE *a, int size) {
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        PRIORITY_QUEUE_VALUE tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

// Run a unit test on a priority queue of random data with given size.
int pq_test(int size) {
    PRIORITY_QUEUE q[1];
    pq_init(q);

    fprintf(stderr, "test: heap_ops (n = %d):\n", size);

    // Build linear pointer array of random keys.
    PRIORITY_QUEUE_VALUE *dist;
    NewArray(dist, size);
    for (int i = 0; i < size; i++)
        dist[i] = i;
    permute(dist, size);
    fprintf(stderr, "  built random data\n");

    pq_set_up(q, dist, size);
    fprintf(stderr, "  finished set up\n");

    // Pull min repeatedly and verify vals are what they should be.
    PRIORITY_QUEUE_VALUE should_be = 0;
    for (int n = 0; n < size; n++) {
        int i = pq_get_min(q);
        if (dist[i] != should_be++) {
            fprintf(stderr, "fail 1: %i -> %g\n", i, dist[i]);
            return 42;
        }
    }
    fprintf(stderr, "  finished checking heapify\n");

    // Now rebuild the heap by inserting one key at a time.
    for (int n = 0; n < size; n++)
        pq_add(q, n);
    fprintf(stderr, "  finished insertions\n");

    // And verify values again.
    should_be = 0;
    for (int n = 0; n < size; n++) {
        int i = pq_get_min(q);
        if (dist[i] != should_be++) {
            fprintf(stderr, "  fail 2: %i -> %g\n", i, dist[i]);
            return 4242;
        }
    }
    fprintf(stderr, "  finished checking insert\n");

    // Now rebuild the heap by inserting one key at a time in reverse.
    for (int n = size - 1; n >= 0; n--)
        pq_add(q, n);
    fprintf(stderr, "  finished reverse insertions\n");

    // Now delete one by one.
    for (int n = 0; n < size; n++)
        pq_delete(q, n);
    fprintf(stderr, "  finished reverse deletions\n");

    pq_clear(q);
    return 0;
}

static void draw(FILE *f, NODE *node, double x, double y, double w, double h) {
    emit_rectangle(f, x, y, w, h);
    emit_marker_ptr_array(f, node->markers, node->marker_count);
    if (internal_p(node)) {
        for (int q = 0; q < 4; q++) {
            QUADRANT_DECL(q, qx, qy, qw, qh, x, y, w, h);
            draw(f, node->children + q, qx, qy, qw, qh);
        }
    }
}

static void draw_nearest(FILE *f, MARKER *markers, MARKER **nearest_markers, int n_markers) {
    for (int i = 0; i < n_markers; i++) {
        if (nearest_markers[i])
            emit_segment(f, markers + i, nearest_markers[i]);
    }
}

int qt_draw(QUADTREE *qt, MARKER *markers, MARKER **nearest_markers, int n_markers, const char *name) {
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
