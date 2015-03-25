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
 * Header file for Implementation of List for Hypercuts
 */

#ifndef XF_LIST_H
#define XF_LIST_H

#include "xf_stdinc.h"

struct xf_list {
    int    N;                /* list defined on ints in {1,...,N} */
    int    first;            /* beginning of list */
    int    last;             /* last element of the list */
    int*   next;             /* next[i] is successor of i in list */
};

struct xf_list* xf_list_init(int N);                /* intilize the list */
void    xf_list_exit(struct xf_list*);              /* destroy the list */

void    xf_list_push(struct xf_list*, int);         /* push item onto front */
void    xf_list_append(struct xf_list*, int);       /* append item */
int     xf_list_get(struct xf_list*, int);          /* access item */
void    xf_list_trunc(struct xf_list*, int);        /* remove initial items */
int     xf_list_suc(struct xf_list*, int);          /* return successor */
void    xf_list_assign(struct xf_list*, struct xf_list*);  /* list assignment */
void    xf_list_clear(struct xf_list*);             /* remove everything */
void    xf_list_reset(struct xf_list*, int);        /* re-initialize to n */
void    xf_list_print(struct xf_list*);             /* print the items on list */

inline bit  xf_list_mbr(struct xf_list*, int);      /* return if member of list */
inline int  xf_list_tail(struct xf_list*);          /* return last item on list */

#endif  /* XF_LIST_H */
