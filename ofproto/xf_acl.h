/*
 * xf_acl.h - Nova v1.
 * Copyright (c) 2013, xFlow Research Inc.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential.
 *
 * Written by Qasim Maqbool <qasim.maqbool@xflowresearch.com>,
 *
 */

#ifndef XF_ACL_H
#define XF_ACL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/ethernet.h>
#include <math.h>

#include "fm_sdk.h"
#include "api/fm_api_acl.h"
#include "xf_hypercuts.h"

extern unsigned header[MAXDIMS];
extern void* pkt_data;
extern fm_timestamp wait;
extern fm_semaphore recvSem;
extern fm_timestamp wait;
extern fm_int sw_acl;
extern int pkt_size;

static grpCount = 0;         /* mirror group count */

struct xf_acl_rule {
    fm_int priority;         // rule_id from hypercuts
    fm_aclCondition cond;    // 5-tuple conditions for now
    fm_aclValue value;       // 5-tuple values for now
    fm_aclActionExt action;  // action to be applied on matching packets
    fm_aclParamExt param;       // parameters for rule conditions
};

struct xf_acl_rule* xf_acl_rule_init(void);
int xf_acl_rule_destroy(struct xf_acl_rule*);

int xf_acl_init(void);
int xf_set_rule_condition(struct xf_acl_rule*);
int xf_set_rule_values(struct xf_acl_rule*, struct xf_rule*);
int xf_set_rule_action(struct xf_acl_rule*, struct xf_rule*);
int xf_add_acl_rule(int*, struct xf_acl_rule*);

int xf_get_acl_rule(fm_int*, fm_aclCondition*,
                    fm_aclValue*, fm_aclActionExt*, fm_aclParamExt*);

int xf_delete_acl_port(fm_int* port);
int xf_delete_acl_rule(fm_int* rule);
int xf_delete_acl(void);
int xf_apply_acl(void);
int xf_add_port_set(struct xf_acl_rule* acl_rule, struct xf_rule* rule);

int xf_get_acl_stats(fm_int*, fm_aclCounters*);

void eventHandler(fm_int, fm_int, void*);

int xf_switch_init(void);

int analyse_data(char*);
int xf_insert_rule(struct xf_acl_rule *, struct xf_rule *, int );

/*

TODO: probably dont need these, but ask Sohaib for clarity
int xf_hal_ispresent(int);
int xf_hal_sync_stats(int, struct xf_hal_rule_stats*);
int xf_hal_rule_update_stats(struct xf_hal_rule* s_rule);

void cleanup(const char* src, int err);

*/

#endif  /* XF_ACL_H */
