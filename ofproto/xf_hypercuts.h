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
 * Hypercuts Packet Classification.
 */

#ifndef XF_HYPERCUTS_H
#define XF_HYPERCUTS_H

#define DEBUG 0 
#define LOOKUP_DEBUG 0

#include "xf_stdinc.h"
#include "xf_dheap.h"
#include "xf_list.h"

#ifdef OVS
#include "classifier.h"
#include "ofp-util.h"
#include "ofproto.h"
#include "ofpbuf.h"
#endif

#define MAXNODES    1000000     /* max nodes in list */
#define MAXRULES    500000      /* max rules */
#define MAXBUCKETS  1000        /* max buckets */
#define MAXCUTS     64          /* max cuts */
#define MAXDIMS     5           /* max dimensions */
#define MIRROR_PORT 3

#define RULESIZE    4.5
#define NODESIZE    4
#define RULEPTSIZE  0.5

#define LOOKUP_MATCH_NONE   -1
#define LOOKUP_MATCH_EQUAL  0
#define LOOKUP_MATCH_HIGH   1
#define LOOKUP_MATCH_LOW    2

struct xf_range {
    unsigned low;               /* lower limit of field value for a node */
    unsigned high;              /* higher limit of field value for a node */
};

struct xf_rule {
    struct xf_range field[MAXDIMS];
    int priority;
    unsigned int in_port;
    char action[5][20];
    uint16_t interface[5];
    int n_actions; 
    int derived_from;
    int num_derived_rules;
    int* derived_rules;
    struct rule *rule_ptr;
    //char vaction[10];
#ifdef OVS
    struct rule* oftable_rule;
    struct ofpact* ofpacts;
    unsigned int ofpacts_len;
#endif
};

struct xf_lookup_match {
    int node_id;
    int rule_id;
    int fid;    // File ID - only used for traces
    int status; // Lookup Status - only used for traces
};

struct xf_node {
    int is_leaf;                    /* is a leaf node if 1 */
    int node_rules;                 /* number of rules in this node */
    int* rule_id;                   /* rule ids in this node */

    int empty_list;                 /* list is empty if 1, 0 if not */
    int list_length;                /* length of list */
    int* rule_list;                 /* moved up rules */

    struct xf_range field[MAXDIMS]; /*node boundry */
    int dim[MAXDIMS];               /* dimensions to cut */
    int n_cuts[MAXDIMS];            /* number of cuts */
    int* child;                     /* child pointers */
};

struct xf_tree {
    /* tree stats */
    int     tree_pass;              /* max tree depth */
    int     num_nodes;              /* current number of nodes in tree */
    int     num_stored_rules;       /* stored rules */
    int     num_redun_rules;        /* removed rules during preprocessing */
    int     num_internal_rules;     /* internally stored rules */
    int     num_loaded_rules;       /* number of rules loaded */
    int     num_rules;              /* current number of rules */
    int     build_status;          
    /* lookup stats */
    unsigned long num_packets;      /* total number of packets thrown */
    unsigned long matched_packets;  /* classified packets */
    float   cost;                   /* number of memeory access */
    int     num_accesses;           /* memory accessess */
    float   lookup_time;            /* Lookup Time */
    float   worstcost;              /* worst case cost */
    int     worstaccesses;          /* worst case memory accesses */
    float   worsttime;              /* worst case lookup time */
    float   bestcost;               /* best case cost */
    int     bestaccesses;           /* best case memory accesses */
    float   besttime;               /* best case lookup time */

    /* tree parameters */
    int     bucket_size;            /* leaf threashold */
    float   spfac;                  /* space explosion factor */
    int     redun;                  /* redundancy optimization */
    int     push;                   /* rule pushing optimization */
    int     push_threshold;         /* rule pushing threshhold */

    /* files */
    FILE* fpr;                      /* fp for ruleset file */
    FILE* fpt;                      /* fp for test trace file */

    struct xf_rule* rule;           /* all rules from the ruleset */
    struct xf_node* node_set;       /* base pointer to array of nodes */
    struct ofproto* ofproto;        /* pointer to OpenvSwitch ofproto */

    /*rules splitting - are these needed? */
    int     num_split_rules;
    struct xf_rule* spilt_rules;
};

struct xf_tree vtt_handle;

void    xf_add_rule(struct ofproto*, struct xf_rule*);

void    parseargs(struct xf_tree*, int, char**);                /* parses arguments of hypercut */
//int     xf_load_rules(struct xf_tree*);                         /* loads the rule set */
void    printbounds(struct xf_tree*);                           /* prints max and min of rules */

int set_default_args(struct xf_tree*);
#ifdef OVS
void    xf_tree_init(struct xf_tree*, struct ofproto*);
#else
void    xf_tree_init(struct xf_tree*);
#endif

void    print_unique_components(struct xf_tree*, int*);
void    xf_choose_np_dim(struct xf_tree*, struct xf_node*);     /* choose unique dimentions */
void    xf_remove_redundancy(struct xf_tree*, struct xf_node*); /* remove redundancy optimization */
void    xf_pushing_rule(struct xf_tree*, struct xf_node*);      /* rule pushing optimization */
void    xf_tree_build(struct xf_tree*);                         /* create trie */

void    print_tree_stat(struct xf_tree*);                       /* prints the stats of tree */
void    print_node_stat(struct xf_tree*, int, int);             /* prints the stats of node */

struct xf_rule* xf_tree_lookup(struct xf_tree*, unsigned*, struct xf_lookup_match*); /* packet lookup */
void    tracesLookup(struct xf_tree*);                          /* perfrom lookup from traces */
void    print_lookup_stats(struct xf_tree*);                    /* Print the stats for tree lookup */

void    xf_partition_rules(struct xf_tree*);                    /* Rule partitioning */

int     trieNode(struct xf_tree*, struct xf_rule add_rule, int* nodeID);
int     insertNode(struct xf_tree*);
int    xf_update_rules(struct xf_tree*, struct xf_rule* add_rule);

/*  int     xf_node_rules(struct xf_node*,int,uint32_t*); */
void    xf_tree_destroy(struct xf_tree*);                       /* destroy the trie */

#endif  /* XF_HYPERCUT_H */
