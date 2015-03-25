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
 * Header file for MTT for NOVA1 prototype
 */

#ifndef XF_MTT_H
#define XF_MTT_H

#include "xf_stdinc.h"
#include "ofproto-provider.h"
#include "xf_hypercuts.h"
/* #include "xf_hal.h" */
#include "xf_acl.h"

#define TIME_DIFF(t1, t2) \
        ((t2.tv_sec - t1.tv_sec)*1000.0*1000.0) + (t2.tv_usec - t1.tv_usec)

#define MTT_SIZE 3072    // maximum number of 5-tuple rules in TCAM

#define TIMEOUT 5000000

/* List of slots in the MTT. 0 - empty, 1 - filled */
int xf_mtt_slots[MTT_SIZE];

/* Variables to provide MTT sync in a separate thread, with mutex locks */
int ij, it;
pthread_t insert_thread;
pthread_mutex_t mtt_lock;

/* Thread to sync TCAM and MTT */
void* xf_sync_tcam(void* t);

/* Global Linked List Head */
struct xf_mtt_list_node* xf_mtt_handle;
int xf_mtt_num_rules;

struct xf_mtt_node_stats {
    uint32_t packets, bytes;  /* stats */
    struct timeval last_updated;
};

/* Simple MTT node Structure */
struct xf_mtt_node {
    uint32_t vtt_node_id;    /* accessed using tree handle */
    uint32_t* tcam_handle;   /* accessed using TCAM handle */
    uint32_t nrules;
    struct xf_mtt_node_stats stats;
    struct xf_tree *tree;
    bool flag_insert;
    bool flag_remove;
};

/* Linked List Node structure format */
struct xf_mtt_list_node {
    struct xf_mtt_node* data;
    struct xf_mtt_list_node* next;
};

/* MTT functions */
int xf_mtt_init(void);
int find_empty_slot(void);
int xf_mtt_insert(struct xf_tree*, int);
int xf_mtt_read(int);
int xf_mtt_delete(int);
int xf_mtt_ispresent(int);
int xf_mtt_length(void);
int xf_mtt_rules_count(struct xf_mtt_list_node*);
int xf_mtt_print(void);
int xf_mtt_exit(void);
int xf_mtt_force_eviction(int);
int xf_mtt_sync_stats(struct xf_mtt_list_node *);

/* Link List Functions */
int xf_mtt_list_push(struct xf_mtt_list_node**, struct xf_mtt_node*);       /* pushes a value onto Linked List */
int xf_mtt_list_push_last(struct xf_mtt_list_node**, struct xf_mtt_node*);  /* appends a node */
struct xf_mtt_node* xf_mtt_list_pop(struct xf_mtt_list_node**);             /* removes the head from the Linked List */
int xf_mtt_list_delete(struct xf_mtt_list_node**, int);                     /* deletes an element */
struct xf_mtt_node* xf_mtt_list_elem(struct xf_mtt_list_node**, int);       /* checks for an element */
int xf_mtt_list_len(struct xf_mtt_list_node*);                              /* Linked List length */
int xf_mtt_list_print(struct xf_mtt_list_node**);                           /* prints all the Linked List data */
int xf_mtt_list_clear(struct xf_mtt_list_node**);                           /* clears the Linked List of all elements */
int xf_mtt_list_evict_node(struct xf_mtt_list_node**);

int xf_node_print(struct xf_mtt_node*);

#endif  /* XF_MTT_H */
