/*
 * Copyright (c) 2013, xFlow Research Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Header file for dynamic heap data structure.
 * Maintains a subset of items in {1,...,m}, where item has a key.
 */

#ifndef XF_DHEAP_H
#define XF_DHEAP_H

#include "xf_stdinc.h"

/*
 * typedef for better understanding of dheap
 */
typedef unsigned long keytyp;
typedef int item;

struct dheap {
    int     N;                  /* max number of items in heap */
    int     n;                  /* number of items in heap */
    int     d;                  /* base of heap , base 2 is binary heap */
    item*   h;                  /* {h[1],...,h[n]} is set of items */
    int*    pos;                /* pos[i] gives position of i in h */
    keytyp* kvec;               /* kvec[i] is key of item i */
};

struct  dheap*  dheap_init(int, int);
void    dheap_exit(struct dheap*);


void    dheap_insert(struct dheap*, item, keytyp);      /* insert item with specified key */
void    dheap_remove(struct dheap*, item);              /* remove item from heap */
item    dheap_deletemin(struct dheap*);                 /* remove and return smallest item */
void    dheap_changekey(struct dheap*, item, keytyp);   /* change the key of an item */
void    dheap_print(struct dheap*);                     /* print the heap */

item    dheap_minchild(struct dheap*, item);            /* return smallest child of item */
void    dheap_siftup(struct dheap*, item, int);         /* move item up to restore heap order */
void    dheap_siftdown(struct dheap*, item, int);       /* move item down to restore heap order */


inline item    dheap_findmin(struct dheap*);            /* return the item with smallest key */
inline keytyp  dheap_key(struct dheap*, item);          /* return the key of item */
inline bit     dheap_member(struct dheap*, item);       /* return true if item in heap */
inline bit     dheap_empty(struct dheap*);              /* return true if heap is empty */

#endif  /* XF_DHEAP_H */
