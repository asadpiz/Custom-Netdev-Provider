/*
 * xf_acl.c - Nova v1.
 * Copyright (c) 2013, xFlow Research Inc.
 *
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential.
 *
 * Written by Qasim Maqbool <qasim.maqbool@xflowresearch.com>
 *
 */

#include "xf_acl.h"
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

#include "fm_sdk.h"
#include "api/fm_api_acl.h"

// #define FM_MAIN_SWITCH 0

/**************************************************
* Global Variables
**************************************************/

// fm_int sw = 0;
fm_eventPktRecv* pktEvent = 0;
fm_buffer* recvPkt = 0;
int intinit = 0;
int is_queue_init = 0;  /* Flag to save the state of queues */

fm_status err;
fm_int sw_acl = 0;  // FM_MAIN_SWITCH;
fm_int acl = 0;

unsigned header[MAXDIMS];
fm_timestamp wait = { 5 , 0 };
fm_semaphore seqSem, recvSem;

/**************************************************
* Application's event handler
**************************************************/

void
eventHandler(fm_int event, fm_int sw, void* ptr) {
    fm_eventPort* portEvent = (fm_eventPort*) ptr;
    FM_NOT_USED(sw);
    char* data = (char*)malloc(sizeof(char) * 100);
    struct iphdr* ip;
    struct tcphdr* tcp;
    struct ether_header* ether;

    switch (event) {
    case FM_EVENT_SWITCH_INSERTED:
        printf("Switch #%d inserted!\n", sw);

        if (sw == 0) {  // FM_MAIN_SWITCH) {
            fmSignalSemaphore(&seqSem);
        }

        break;

    case FM_EVENT_PORT:
        printf("port event: port %d is %s\n",
               portEvent->port,
               (portEvent->linkStatus ? "up" : "down"));
        break;

    case FM_EVENT_PKT_RECV:
        pktEvent = (fm_eventPktRecv*) ptr;
        recvPkt = (fm_buffer*)pktEvent->pkt;
        printf("Packet Recieved at %d\n", pktEvent->srcPort);
        printf("Packet Length = %lu \n", (size_t)recvPkt->len);
        analyse_data((char*)recvPkt->data);
        fmSignalSemaphore(&recvSem);
        /*        if (!sq_push(pktEvent->srcPort, &soft_q, (void*)recvPkt->data,
                             (size_t)recvPkt->len)) {
                    printf("XFLOW_DEBUG: ERROR: error while pushing the data ");
                    printf("in software queues. \n");
                }
        */
        break;
    }
}

/*
 * Initializes netdev Intel system and Initializes the Seacliff Switch
*/
int xf_switch_init() {
    if (!intinit) {
        intinit = 1;
        fm_uint32 compile_flags, apply_flags;
        fm_switchInfo   swInfo;
        fm_int          cpi, rule;
        fm_int          port;
        fm_aclCondition cond;
        fm_aclValue     value;
        fm_aclAction    action;
        struct in_addr src;
        struct in_addr dst;
        char            status[100];
        /* First Rule */
        fm_int firstRule;
        fm_aclCondition firstCond;
        fm_aclValue* firstValue = malloc(sizeof(fm_aclValue));
        fm_aclAction firstAction;
        fm_aclParam firstParam;
        fmOSInitialize();
        fmCreateSemaphore("seq", FM_SEM_BINARY, &seqSem, 0);
        fmCreateSemaphore("recv", FM_SEM_BINARY, &recvSem, 0);

        if ((err = fmInitialize(eventHandler)) != FM_OK) {
            cleanup("fmInitialize", err);
        }

        fmWaitSemaphore(&seqSem, &wait);

        if ((err = fmSetSwitchState(sw_acl, TRUE)) != FM_OK) {
            cleanup("fmSetSwitchState", err);
        }

        fmCreateVlan(sw_acl, 1);
        fmGetSwitchInfo(sw_acl, &swInfo);
        printf("Total Number of Ports = %d\n", swInfo.numCardPorts);

        for (cpi = 1 ; cpi < swInfo.numCardPorts ; cpi++) {
            if ((err = fmMapCardinalPort(sw_acl, cpi, &port, NULL)) != FM_OK) {
                cleanup("fmMapCardinalPort", err);
            }

            if ((err = fmSetPortState(sw_acl, port, FM_PORT_STATE_UP, 0)) != FM_OK) {
                cleanup("fmSetPortState", err);
            }

            if ((err = fmAddVlanPort(sw_acl, 1, port, FALSE)) != FM_OK) {
                cleanup("fmAddVlanPort", err);
            }

            if ((err = fmSetVlanPortState(sw_acl,
                                          1,
                                          port,
                                          FM_STP_STATE_FORWARDING)) != FM_OK) {
                cleanup("fmSetVlanPortState", err);
            }
        }

        acl = 0;

        if ((err = fmCreateACL(sw_acl, acl)) != FM_OK) {
            cleanup("fmCreateACL", err);
        }

        rule = 12;
        cond = FM_ACL_MATCH_SRC_IP | FM_ACL_MATCH_DST_IP | FM_ACL_MATCH_PROTOCOL;
        inet_aton("0.0.0.0", &src);
        value.srcIp.addr[0] = src.s_addr;
        value.srcIp.isIPv6 = 0;
        value.srcIpMask.addr[0] = 0;
        value.srcIpMask.isIPv6 = 0;
        inet_aton("0.0.0.0", &dst);
        value.srcIp.addr[0] = dst.s_addr;
        value.srcIp.isIPv6 = 0;
        value.srcIpMask.addr[0] = 0;
        value.srcIpMask.isIPv6 = 0;
        value.protocol = 0x01;
        value.protocolMask = 8;
        action = FM_ACL_ACTION_TRAP;

        if ((err = fmAddACLRule(sw_acl, acl, rule, cond, &value, \
                                action, 0)) != FM_OK) {
            cleanup("fmAddACLRule", err);
        }

        for (cpi = 1 ; cpi < swInfo.numCardPorts ; cpi++) {
            port = cpi;

            if ((err = fmAddACLPort(sw_acl, acl, port, FM_ACL_TYPE_INGRESS)) != FM_OK) {
                cleanup("fmAddACLPort", err);
            }
        }

        compile_flags = FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE;

        if ((err = fmCompileACL(sw_acl, status, 100, 0)) != FM_OK) {
            cleanup("fmCompileACL", err);
        }

        apply_flags = FM_ACL_APPLY_FLAG_NON_DISRUPTIVE;

        if ((err = fmApplyACL(sw_acl, 0)) != FM_OK) {
            cleanup("fmApplyACL", err);
        }

        fmGetACLRuleFirst(0, 0, &firstRule, &firstCond, &firstValue, &firstAction, &firstParam);
        printf("after initialization, first rule = %d\n", (int)firstRule);
    }

    return 0;
}

/**
 *@breif       Packet Crawl
 *
 *@param data  Crawl packet
 *
 *@return       0 for success, other means fail
 */
int analyse_data(char* data) {
    struct iphdr* ip;
    struct tcphdr* tcp;
    struct ether_header* ether;
    /*  If the data from the data link had to crawl,
     *  then there is the Ethernet frame header */
    ether = (struct ether_header*)data;
    printf("Ether Type :%d\n", ether->ether_type);
    /* IP Header*/
    ip = (struct iphdr*)(data + sizeof(struct ether_header));
    /* ip = (struct iphdr*)data; */
    header[4] = ip->protocol;
    printf("Protocol::%d\n", ip->protocol);
    header[0] = ntohl(ip->saddr);
    printf("Source IP::%s\n", inet_ntoa(*((struct in_addr*)&ip->saddr)));
    header[1] = ntohl(ip->daddr);
    printf("Dest IP::%s\n", inet_ntoa(*((struct in_addr*)&ip->daddr)));
    /* TCP HEADER*/
    tcp = (struct tcphdr*)(data + sizeof(*ip));
    header[2] = ntohs(tcp->source);
    printf("Source Port::%d\n", ntohs(tcp->source));
    header[3] = ntohs(tcp->dest);
    printf("Dest Port::%d\n", ntohs(tcp->dest));
    printf("\n");
    return 0;
}

void cleanup(const char* src, int err) {
    printf("ERROR: %s: %s\n", src, fmErrorMsg(err));
    exit(1);
}   /* end cleanup */

int fm_init() {
    fm_status err;
    // fm_int firstACL;
    fm_int  vlan, cpi, port;
    // fm_uint32 scenarios, compile_flags, apply_flags;
    // fm_uint64 framecount = 0;
    // fm_aclCondition cond;
    // fm_aclValue value;
    // void *compile_val;
    // fm_aclAction action;
    // fm_aclParam param;
    // fm_aclPortAndType *portAndType;
    // fm_text statusText = (char *)malloc(200 * sizeof(char));
    // fm_int statusTextLength = 200;
    fm_timestamp wait = { 5, 0 };
    // fm_bool *state;
    // struct in_addr src, dst, smask;
    fm_switchInfo swInfo;
    char status[100];
    fm_int mode, state, info;
    // int x;
    vlan = 1;
    // initialize OS so we can create a semaphore
    fmOSInitialize();
    // create the sequencing semaphore to wait until switch is inserted
    fmCreateSemaphore("seq", FM_SEM_BINARY, &seqSem, 0);

    if ((err = fmInitialize(eventHandler)) != FM_OK) {
        cleanup("fmInitialize", err);
    }

    fmWaitSemaphore(&seqSem, FM_WAIT_FOREVER);

    // bring up switch #0
    if ((err = fmSetSwitchState(sw_acl, TRUE)) != FM_OK) {
        cleanup("fmSetSwitchState", err);
    }

    printf("sw_acl = %d\n", sw_acl);

    if ((err = fmCreateVlan(sw_acl, vlan)) != FM_OK) {
        cleanup("fmCreateVlan", err);
    }

    fmGetSwitchInfo(sw_acl, &swInfo);
    printf("Total Number of Ports = %d ", swInfo.numCardPorts);

    for (cpi = 1 ; cpi < swInfo.numCardPorts ; cpi++) {
        if ((err = fmMapCardinalPort(sw_acl, cpi, &port, NULL)) != FM_OK) {
            cleanup("fmMapCardinalPort", err);
        }

        if ((err = fmSetPortState(sw_acl, port, FM_PORT_STATE_UP, 0)) != FM_OK) {
            cleanup("fmSetPortState", err);
        }

        if ((err = fmAddVlanPort(sw_acl, vlan, port, FALSE)) != FM_OK) {
            cleanup("fmAddVlanPort", err);
        }

        if ((err = fmSetVlanPortState(sw_acl, vlan, port,
                                      FM_STP_STATE_FORWARDING)) != FM_OK) {
            cleanup("fmSetVlanPortState", err);
        }
    }

    return 0;
}

struct xf_acl_rule* xf_acl_rule_init() {
    struct xf_acl_rule* rule = (struct xf_acl_rule*)malloc(sizeof(struct xf_acl_rule));
    return rule;
}

int xf_acl_rule_destroy(struct xf_acl_rule* rule) {
    free(rule);
    return 0;
}

/* Initialize ACL for storing rules */
int xf_acl_init() {
    if ((err = fmCreateACL(sw_acl, acl)) != FM_OK) {
        printf("ERROR: fmCreateACL: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}

int xf_set_rule_condition(struct xf_acl_rule* acl_rule) {
    acl_rule->cond = FM_ACL_MATCH_SRC_IP | FM_ACL_MATCH_DST_IP | FM_ACL_MATCH_PROTOCOL;  // | FM_ACL_MATCH_L4_SRC_PORT_MAP | FM_ACL_MATCH_L4_DST_PORT_MAP;
    return 0;
}

int xf_set_rule_values(struct xf_acl_rule* acl_rule, struct xf_rule* rule) {
    struct in_addr srcaddr, dstaddr;
    char* sip;  // =(char*)malloc(sizeof(char)*32);
    char* dip;  // =(char*)malloc(sizeof(char)*32);
    /* set source IP */
    // printf("Rule %u :  %u, and = %u \n",rule->field[0].low,rule->field[0].high,
    //        rule->field[0].low & rule->field[0].high);
    acl_rule->value.srcIp.addr[0] = htonl(rule->field[0].low & rule->field[0].high);
    printf("Src Ip %u\n", acl_rule->value.srcIp.addr[0]);
    acl_rule->value.srcIpMask.addr[0] = 32 - ceil(log(rule->field[0].high - rule->field[0].low + 1) / log(2));
    acl_rule->value.srcIp.isIPv6 = 0;
    acl_rule->value.srcIpMask.isIPv6 = 0;
    /* set destination IP */
    acl_rule->value.dstIp.addr[0] = htonl(rule->field[1].low & rule->field[1].high);
    printf("Dst IP %u \n", acl_rule->value.dstIp.addr[0]);
    acl_rule->value.dstIpMask.addr[0] = 32 - ceil(log(rule->field[1].high - rule->field[1].low + 1) / log(2));
    acl_rule->value.dstIp.isIPv6 = 0;
    acl_rule->value.dstIpMask.isIPv6 = 0;
    /*
        printf("Src IP 2 %u \n", acl_rule->value.srcIp.addr[0]);
        printf("Dst IP 2 %u \n", acl_rule->value.dstIp.addr[0]);
                                 - rule->field[1].low + 1) / log(2);
    */
    /* set network protocol */
    acl_rule->value.protocol = rule->field[4].low;

    if (rule->field[4].low == 0) {
        acl_rule->value.protocolMask = 0;
    } else {
        acl_rule->value.protocolMask = 8;
    }

    srcaddr.s_addr = acl_rule->value.srcIp.addr[0];
    dstaddr.s_addr = acl_rule->value.dstIp.addr[0];
    printf("Src IP 3 %u \n", srcaddr.s_addr);
    printf("Dst IP 3 %u \n", dstaddr.s_addr);
    sip = inet_ntoa(srcaddr);
    printf("Values : Rule %d [%s ", acl_rule->priority, sip);
    dip = inet_ntoa(dstaddr);
    printf("%s 0 0 %d]\n", dip, acl_rule->value.protocol);
    // free(sip);
    // free(dip);
    return 0;
}

int xf_set_rule_action(struct xf_acl_rule* acl_rule, struct xf_rule* rule) {
    if (strcmp(rule->action, "output") == 0) {
        printf("action = output:%u\n", rule->interface);
        acl_rule->action = FM_ACL_ACTION_PERMIT;
    } else if (strcmp(rule->action, "flood") == 0) {
        printf("action = flood\n");
        acl_rule->action = FM_ACL_ACTION_PERMIT;
    } else if (strcmp(rule->action, "drop") == 0) {
        printf("action = drop\n");
        acl_rule->action = FM_ACL_ACTION_DENY;
    } else {
        printf("action = normal\n");
        acl_rule->action = FM_ACL_ACTION_PERMIT;
    }

    return 0;
}

int xf_add_acl_rule(int* priority, struct xf_acl_rule* rule) {
    printf("in xf_add_acl_rule with priority = %d\n", *priority);
    char* sip, *dip;
    fm_int aclrule;
    fm_aclCondition cond;
    fm_aclValue* value = (fm_aclValue*)calloc(sizeof(fm_aclValue), 1);
    fm_aclAction action;
    int x;
    fm_aclParam param;
    struct in_addr saddr, daddr;
    struct in_addr src, dst;
    aclrule = *priority;
    cond = FM_ACL_MATCH_SRC_IP | FM_ACL_MATCH_DST_IP | FM_ACL_MATCH_PROTOCOL;
    inet_aton("141.116.94.130", &src);
    inet_aton("64.91.107.9", &dst);
    value->srcIp.addr[0] = src.s_addr;
    value->srcIp.isIPv6 = 0;
    value->srcIpMask.addr[0] = 32;
    value->srcIpMask.isIPv6 = 0;
    value->dstIp.addr[0] = dst.s_addr;
    value->dstIp.isIPv6 = 0;
    value->dstIpMask.addr[0] = 32;
    value->dstIpMask.isIPv6 = 0;
    value->protocol = 6;
    value->protocolMask = 8;
    param = 0;
    action = FM_ACL_ACTION_PERMIT;
    /*
        if ((err = fmAddACLRule(0, 0, aclrule, cond, &value, action, param)) != FM_OK) {
            printf("ERROR: fmAddACLRule: %s\n", fmErrorMsg(err));
            return -1;
        }
    */
    printf("RULE SIP = %u\n", rule->value.srcIp.addr[0]);
    printf("RULE SIP MASK = %u\n", rule->value.srcIpMask.addr[0]);
    printf("RULE DIP = %u\n", rule->value.dstIp.addr[0]);
    printf("RULE DIP MASK = %u\n", rule->value.dstIpMask.addr[0]);
    printf("RULE PROTOCOL = %u\n", rule->value.protocol);
    printf("RULE PROTOCOL MASK = %u\n", rule->value.protocolMask);
    printf("SIP = %u\n", value->srcIp.addr[0]);
    printf("SIP MASK = %u\n", value->srcIpMask.addr[0]);
    printf("DIP = %u\n", value->dstIp.addr[0]);
    printf("DIP MASK = %u\n", value->dstIpMask.addr[0]);
    printf("PROTOCOL = %u\n", value->protocol);
    printf("PROTOCOL MASK = %u\n", value->protocolMask);
    rule->priority = *priority;
    saddr.s_addr = value->srcIp.addr[0];
    daddr.s_addr = value->dstIp.addr[0];
    // printf("Rule Src IP 2 %u \n", saddr.s_addr);
    // printf("Rule Dst IP 2 %u \n", daddr.s_addr);
    sip = inet_ntoa(saddr);
    dip = inet_ntoa(daddr);
    x = memcmp(&rule->value.srcIp, &value->srcIp, sizeof(fm_ipAddr));
    printf("x = %d\n", x);
    x = memcmp(&rule->value.dstIp, &value->dstIp, sizeof(fm_ipAddr));
    printf("x = %d\n", x);
    x = memcmp(&rule->value.srcIpMask, &value->srcIpMask, sizeof(fm_ipAddr));
    printf("x = %d\n", x);
    x = memcmp(&rule->value.dstIpMask, &value->dstIpMask, sizeof(fm_ipAddr));
    printf("x = %d\n", x);
    x = memcmp(&rule->value.protocol, &value->protocol, sizeof(fm_byte));
    printf("x = %d\n", x);
    x = memcmp(&rule->value.protocolMask, &value->protocolMask, sizeof(fm_byte));
    printf("x = %d\n", x);
    fm_aclValue* new_value = (fm_aclValue*)calloc(sizeof(fm_aclValue), 1);
    // memcpy(new_value, &rule->value, sizeof(fm_aclValue));
    new_value->srcIp.addr[0] = value->srcIp.addr[0];
    new_value->srcIp.isIPv6 = value->srcIp.isIPv6;
    new_value->srcIpMask.addr[0] = value->srcIpMask.addr[0];
    new_value->srcIpMask.isIPv6 = value->srcIpMask.isIPv6;
    new_value->dstIp.addr[0] = value->dstIp.addr[0];
    new_value->dstIp.isIPv6 = value->dstIp.isIPv6;
    new_value->dstIpMask.addr[0] = value->dstIpMask.addr[0];
    new_value->dstIpMask.isIPv6 = value->dstIpMask.isIPv6;
    new_value->protocol = value->protocol;
    new_value->protocolMask = value->protocolMask;
    x = memcmp(value, new_value, sizeof(fm_aclValue));
    printf("new x = %d\n", x);

    if ((err = fmAddACLRule(sw_acl, acl, rule->priority, rule->cond, &rule->value, rule->action, rule->param)) != FM_OK) {
        printf("ERROR: fmAddACLRule: %s\n", fmErrorMsg(err));
        return -1;
    }

    free(new_value);
    printf("Rule %d added to ACL 0\n", *priority);
    free(value);
    return 0;
}

int xf_add_acl_to_port(fm_int* port, fm_aclType* type) {
    if ((err = fmAddACLPort(sw_acl, acl, *port, *type)) != FM_OK) {
        printf("ERROR: fmAddACLPort: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}

int xf_apply_acl() {
    fm_text statusText = (char*)malloc(200 * sizeof(char));
    fm_int statusTextLength = 200;  // for now, 200 chars should suffice

    if ((err = fmCompileACL(sw_acl, statusText, statusTextLength, 0)) != FM_OK) {
        printf("%s\n", statusText);
        printf("ERROR: fmCompileACL: %s\n", fmErrorMsg(err));
        return -1;
    } else {
        printf("ACL Compiled\n");
    }

    if ((err = fmApplyACL(sw_acl, 0)) != FM_OK) {
        printf("ERROR: fmApplyACL: %s\n", fmErrorMsg(err));
        return -1;
    } else {
        printf("ACL Applied \n");
    }

    return 0;
}


int xf_delete_acl() {
    if ((err = fmDeleteACL(sw_acl, acl)) != FM_OK) {
        printf("ERROR: fmDeleteACL: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}

int xf_delete_acl_rule(fm_int* rule) {
    printf("deleting acl rule: %d in acl: %d\n", *rule, acl);

    if ((err = fmDeleteACLRule(sw_acl, acl, *rule)) != FM_OK) {
        printf("ERROR: fmDeleteACLRule: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}

int xf_delete_acl_port(fm_int* port) {
    if ((err = fmDeleteACLPort(sw_acl, acl, *port)) != FM_OK) {
        printf("ERROR: fmDeleteACLPort: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}

int xf_get_acl_rule(fm_int* rule, fm_aclCondition* cond, fm_aclValue* value, fm_aclActionExt* action, fm_aclParamExt* param) {
    if ((err = fmGetACLRule(sw_acl, acl, *rule, cond, value, action,
                            param)) != FM_OK) {
        printf("ERROR: fmGetACLRule: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}

int xf_get_acl_stats(fm_int* rule, fm_aclCounters* counters) {
    if ((err = fmGetACLCountExt(sw_acl, acl, *rule, counters)) != FM_OK) {
        printf("ERROR: fmGetACLCount: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}


