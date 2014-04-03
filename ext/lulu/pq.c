/*
 * priority_queue.c
 *
 * A priority queue with a min heap. This accepts an array
 * of values and maintains a min heap of indices into the values array.
 * There is also a reverse map that takes a value index to the heap
 * element that refers to it. This allows values to be adjusted.
 *
 *  Created on: Feb 23, 2014
 *      Author: generessler
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "utility.h"
#include "pq.h"

// Move index at heap location j upward until its parent's value is no bigger.
static void pq_sift_up(PRIORITY_QUEUE *q, int j) {
    int i = q->heap[j];
    PRIORITY_QUEUE_VALUE val = q->values[i];
    while (j > 0) {
        int j_pnt = (j - 1) / 2;
        if (q->values[q->heap[j_pnt]] <= val)
            break;
        int k = q->heap[j] = q->heap[j_pnt];
        q->locs[k] = j;
        j = j_pnt;
    }
    int k = q->heap[j] = i;
    q->locs[k] = j;
}

// Move index at heap location j downward until its children's values are no smaller.
// Pay some attention to efficiency because this is bottleneck code.
static void pq_sift_down(PRIORITY_QUEUE *q, int j) {
    int i = q->heap[j];
    PRIORITY_QUEUE_VALUE val = q->values[i];
    for (;;) {
        int j_rgt = 2 * j + 2;
        if (j_rgt < q->size) {
            // two children
            int j_lft = j_rgt - 1;
            PRIORITY_QUEUE_VALUE val_lft = q->values[q->heap[j_lft]];
            PRIORITY_QUEUE_VALUE val_rgt = q->values[q->heap[j_rgt]];
            if (val_lft < val_rgt) {
                if (val <= val_lft)
                    break;
                int k = q->heap[j] = q->heap[j_lft];
                q->locs[k] = j;
                j = j_lft;
            } else {
                if (val <= val_rgt)
                    break;
                int k = q->heap[j] = q->heap[j_rgt];
                q->locs[k] = j;
                j = j_rgt;
            }
        } else if (j_rgt == q->size) {
            // left child only
            int j_lft = j_rgt - 1;
            if (val <= q->values[q->heap[j_lft]])
                break;
            int k = q->heap[j] = q->heap[j_lft];
            q->locs[k] = j;
            j = j_lft;
            break; // this node has no children
        } else {
            break; // no children at all
        }
    }
    int k = q->heap[j] = i;
    q->locs[k] = j;
}

// Initialize a newly allocated priority queue structure.
void pq_init(PRIORITY_QUEUE *q) {
    q->max_size = q->size = 0;
    q->heap = NULL;
    q->locs = NULL;
    q->values = NULL;
}

// Clear a previously initialized and possibly set up priority queue, returning
// it to the initialized state but with all resourced freed.  Note the values
// are owned by the user and are not freed here.
void pq_clear(PRIORITY_QUEUE *q) {
    Free(q->heap);
    Free(q->locs);
    pq_init(q);
}

// Build the queue with given pre-allocated and filled array of values.
void pq_set_up(PRIORITY_QUEUE *q, PRIORITY_QUEUE_VALUE *values, int size) {
    q->max_size = q->size = size;
    q->values = values;
    NewArray(q->heap, size);
    NewArray(q->locs, size);
    for (int i = 0; i < size; i++)
        q->heap[i] = q->locs[i] = i;
    // heapify
    for (int j = size / 2 - 1; j >= 0; j--)
        pq_sift_down(q, j);
}

// Build the queue with given pre-allocated and filled array of values
// and given heap indices.  Note the indices are owned by the heap
// after set up and will be freed with the heap.
void pq_set_up_heap(PRIORITY_QUEUE *q, int *heap, int size,
        PRIORITY_QUEUE_VALUE *values, int max_size) {
    q->max_size = max_size;
    q->values = values;
    q->size = size;
    q->heap = heap;
    NewArray(q->locs, max_size);
    for (int i = 0; i < max_size; i++)
        q->locs[i] = -1;
    for (int j = 0; j < size; j++)
        q->locs[heap[j]] = j;
    for (int j = size / 2 - 1; j >= 0; j--)
        pq_sift_down(q, j);
}

// Return the index of the minimum value on the queue.
int pq_peek_min(PRIORITY_QUEUE *q) {
    return q->size <= 0 ? -1 : q->heap[0];
}

// Remove and return the index of the minimum value on the queue.
int pq_get_min(PRIORITY_QUEUE *q) {
    if (q->size <= 0)
        return -1;
    int i = q->heap[0];
    q->locs[i] = -1;
    if (--q->size > 0) {
        q->heap[0] = q->heap[q->size];
        pq_sift_down(q, 0);
    }
    return i;
}

// Add a new value with index i into the queue.
void pq_add(PRIORITY_QUEUE *q, int i) {
    if (q->size >= q->max_size)
        return;
    int j = q->size++;
    q->heap[j] = i;
    pq_sift_up(q, j);
}

// Restore the heap after the value at index i is changed.
void pq_update(PRIORITY_QUEUE *q, int i) {
    int j = q->locs[i];
    if (j >= 0) {
        pq_sift_down(q, j);
        pq_sift_up(q, j);
    }
}

// Delete index i from the heap, making the corresponding key an orphan.
void pq_delete(PRIORITY_QUEUE *q, int i) {
    int j = q->locs[i];
    if (0 <= j) {
        q->locs[i] = -1;
        if (j < --q->size) {
            q->heap[j] = q->heap[q->size];
            pq_sift_down(q, j);
            pq_sift_up(q, j);
        }
    }
}
