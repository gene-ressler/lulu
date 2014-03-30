/*
 * priority_queue.h
 *
 *  Created on: Feb 23, 2014
 *      Author: generessler
 */

#ifndef PRIORITY_QUEUE_H_
#define PRIORITY_QUEUE_H_

typedef double PRIORITY_QUEUE_VALUE;

typedef struct priority_queue_s {
	int max_size;					// max number of items in heap
	int size;						// current number of items in heap
	int *heap;						// heap of indices to values
	int *locs;						// map of value indices to heap locations
	PRIORITY_QUEUE_VALUE *values;	// values referred to by heap
} PRIORITY_QUEUE;

// Initialize a newly allocated priority queue structure.
void pq_init(PRIORITY_QUEUE *q);

#define PRIORITY_QUEUE_DECL(Q) PRIORITY_QUEUE Q[1]; pq_init(Q)

// Clear a previously initialized and possibly set up priority queue, returning
// it to the initialized state but with all resourced freed.
void pq_clear(PRIORITY_QUEUE *q);

// Build the queue with given pre-allocated and filled array of values.
void pq_set_up(PRIORITY_QUEUE *q, PRIORITY_QUEUE_VALUE *values, int size);

// Build the queue with given pre-allocated and filled array of values
// and given heap indices.  Note the indices are owned by the heap
// after set up and will be freed with the heap.
void pq_set_up_heap(PRIORITY_QUEUE *q,
		int *heap, int size,
		PRIORITY_QUEUE_VALUE *values, int max_size);

// Return the index of the minimum value on the queue.
int pq_peek_min(PRIORITY_QUEUE *q);

// Remove and return the index of the minimum value on the queue.
int pq_get_min(PRIORITY_QUEUE *q);

// Update the queue given that the value at index i has changed.
void pq_update(PRIORITY_QUEUE *q, int i);

// Add a new value with index i into the queue.
void pq_add(PRIORITY_QUEUE *q, int i);

// Delete index i from the heap, making the corresponding key an orphan.
void pq_delete(PRIORITY_QUEUE *q, int i);

// Return an array containing indices currently in the heap.
#define pq_index_set(Q)			((Q)->heap)

// Return an array containing indices currently in the heap.
#define pq_index(Q, I)			((Q)->heap[I])

// Return the size of the index array contained above.
#define pq_index_set_size(Q)	((Q)->size)

// Return non-zero iff the queue is empty.
#define pq_empty_p(Q) ((Q)->size <= 0)

// Run the heap module through some unit tests.
int pq_test(int size);

#endif /* PRIORITY_QUEUE_H_ */
