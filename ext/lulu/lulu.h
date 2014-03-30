/*
 * lulu.h
 *
 *  Created on: Feb 24, 2014
 *      Author: generessler
 */

#ifndef LULU_H_
#define LULU_H_

#include "marker.h"

typedef struct marker_list_s {
    MARKER_INFO info[1];
    MARKER *markers;
    int size, max_size;
} MARKER_LIST;

#define MARKER_LIST_DECL(Name)  MARKER_LIST Name[1]; init_marker_list(Name)
#define ml_set_marker_list_info(L, Kind, Scale)  mr_info_set((L)->info, (Kind), (Scale))

extern char EXT_VERSION[];

void init_marker_list(MARKER_LIST *list);
MARKER_LIST *new_marker_list(void);
void free_marker_list(MARKER_LIST *list);
void clear_marker_list(MARKER_LIST *list);
void add_marker(MARKER_LIST *list, COORD x, COORD y, SIZE size);

#endif /* LULU_H_ */