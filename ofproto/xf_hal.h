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
 *  Header file for HAL implementation for NOVA1
 */

#ifndef XF_HAL_H
#define XF_HAL_H


#include "xf_stdinc.h"
#include "xf_mtt.h"
#include "xf_hypercuts.h"

struct xf_hal_rule_stats {
    uint32_t packets, bytes;
};

struct xf_hal_range {
    unsigned low;               /* lower limit of field value for a node */
    unsigned high;              /* higher limit of field value for a node */
};

/*
 * Rule Structure
 */
struct xf_hal_rule {
    uint32_t id, rule_id;
    struct xf_hal_range field[MAXDIMS];
    struct xf_hal_rule_stats stats;
};

/*
 * Node structure
 */
struct xf_hal_node {
    struct xf_hal_rule* data;
    struct xf_hal_node* next;
};

/*
 * Global Linked List Head
 */
struct xf_hal_node* xf_hal_handle;

/*
 * HAL functions
 */
int xf_hal_load_file(void);
int xf_hal_init(void);
int xf_hal_insert(int);
int xf_hal_read(int);
int xf_hal_delete(int);
int xf_hal_ispresent(int);
int xf_hal_length(void);
int xf_hal_print(void);
int xf_hal_exit(void);
int xf_hal_dump_file(void);
int xf_hal_update_stats(void);
int xf_hal_sync_stats(int, struct xf_hal_rule_stats*);

int xf_hal_rule_print(struct xf_hal_rule* s_rule);
int xf_hal_rule_update_stats(struct xf_hal_rule* s_rule);

/*
 * Linked List Functions
 */
int xf_hal_list_push(struct xf_hal_node**, struct xf_hal_rule*);        /* pushes a value onto the Linked List */
int xf_hal_list_push_last(struct xf_hal_node**, struct xf_hal_rule*);   /* appends a node */
struct xf_hal_rule* xf_hal_list_pop(struct xf_hal_node**);              /* returns the value of Linked List head */
int xf_hal_list_delete(struct xf_hal_node**, int);                      /* deletes an element */
struct xf_hal_rule* xf_hal_list_elem(struct xf_hal_node**, int);        /* checks for an element */
int xf_hal_list_len(struct xf_hal_node*);                               /* Linked List length */
int xf_hal_list_print(struct xf_hal_node**);                            /* prints all the Linked List data */
int xf_hal_list_clear(struct xf_hal_node**);                            /* clears the Linked List of all elements */
int xf_hal_list_update_stats(struct xf_hal_node**);                     /*  updates the stats of the linked list */


#endif  /* XF_HAL_H */
