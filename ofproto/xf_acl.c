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
//fm_eventPktRecv* pktEvent = 0;
//fm_buffer* recvPkt = 0;
//int intinit = 0;
//int is_queue_init = 0;  /* Flag to save the state of queues */

fm_status err;
fm_int sw_acl = 0;  // FM_MAIN_SWITCH;
fm_int acl = 0;
unsigned header[MAXDIMS];
void *pkt_data;
int pkt_size;
//fm_timestamp wait = { 5 , 0 };
fm_semaphore seqSem;

/**************************************************
* Application's event handler
**************************************************/
/*
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
        pkt_data = (void*)recvPkt->data;
	    pkt_size = recvPkt->len;
        analyse_data((char*)recvPkt->data);
        fmSignalSemaphore(&recvSem);
	fmFreeBuffer(sw_acl,recvPkt);
        if (!sq_push(pktEvent->srcPort, &soft_q, (void*)recvPkt->data,
           (size_t)recvPkt->len))
        {
            printf("XFLOW_DEBUG: ERROR: error while pushing the data ");
            printf("in software queues. \n");
        }
        
        break;
    }
}
*/

/* Initializes netdev Intel system and Initializes the Seacliff Switch */

/*
int xf_switch_init() {
    if (!intinit) {
        intinit = 1;
        fm_uint32 compile_flags, apply_flags, scenario, port_value;
        fm_switchInfo   swInfo;
        fm_int          cpi, rule;
        fm_int          port;
        fm_aclCondition cond;
        fm_aclValue     value;
        fm_aclActionExt    action;
	fm_aclParamExt	   *param;
        struct in_addr src;
        struct in_addr dst;
        char            status[100];
        fm_int firstRule;
        fm_aclCondition firstCond;
        fm_aclValue* firstValue = malloc(sizeof(fm_aclValue));
        fm_aclActionExt firstAction;
        fm_aclParamExt firstParam;
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
        printf("Total Number of Ports = %d \n", swInfo.numCardPorts);
        
	port_value = FM_PORT_PARSER_STOP_AFTER_L4;
 
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
	    
            if ( (err = fmSetPortAttribute(sw_acl, port, FM_PORT_PARSER, &port_value) ) != FM_OK ) {
            		cleanup("fmSetPortAttribute", err);
            }	

        }

        acl = 0;
        scenario = FM_ACL_SCENARIO_ANY_FRAME_TYPE | FM_ACL_SCENARIO_ANY_ROUTING_TYPE;
        if ((err = fmCreateACLExt(sw_acl, acl, scenario, 0)) != FM_OK) {
            cleanup("fmCreateACLExt", err);
        }

        rule = 6144;
        cond = FM_ACL_MATCH_SRC_IP; 
        // | FM_ACL_MATCH_DST_IP | FM_ACL_MATCH_PROTOCOL; 
        inet_aton("0.0.0.0", &src);
        value.srcIp.addr[0] = src.s_addr;
        value.srcIp.isIPv6 = 0;
        value.srcIpMask.addr[0] = 0;
        value.srcIpMask.isIPv6 = 0;
        //inet_aton("0.0.0.0", &dst);
        //value.srcIp.addr[0] = dst.s_addr;
        //value.srcIp.isIPv6 = 0;
        //value.srcIpMask.addr[0] = 0;
        //value.srcIpMask.isIPv6 = 0;
        //value.protocol = 0x06;
        //value.protocolMask = 8;
        action = FM_ACL_ACTIONEXT_TRAP | FM_ACL_ACTIONEXT_COUNT;

        if ((err = fmAddACLRuleExt(sw_acl, acl, rule, cond, &value, \
                                action, 0)) != FM_OK) {
            cleanup("fmAddACLRuleExt", err);
        }

        for (cpi = 1 ; cpi < swInfo.numCardPorts ; cpi++) {
            port = cpi;

            if ((err = fmAddACLPort(sw_acl, acl, port, FM_ACL_TYPE_INGRESS)) != FM_OK) {
                cleanup("fmAddACLPort", err);
            }
        }

        compile_flags = FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE;

        if ((err = fmCompileACLExt(sw_acl, status, 100, 0, NULL)) != FM_OK) {
            cleanup("fmCompileACL", err);
        }

        apply_flags = FM_ACL_APPLY_FLAG_NON_DISRUPTIVE;

        if ((err = fmApplyACL(sw_acl, 0)) != FM_OK) {
            cleanup("fmApplyACL", err);
        }

        fmGetACLRuleFirstExt(0, 0, &firstRule, &firstCond, &firstValue, &firstAction, &firstParam);
        printf("after initialization, first rule = %d\n", (int)firstRule);
    }

    return 0;
}
*/
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

/*
void cleanup(const char* src, int err) {
    printf("ERROR: %s: %s\n", src, fmErrorMsg(err));
    exit(1);
}   
*/

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
    fm_uint32 scenario;
    scenario = FM_ACL_SCENARIO_ANY_FRAME_TYPE | FM_ACL_SCENARIO_ANY_ROUTING_TYPE;
    if ((err = fmCreateACLExt(sw_acl, acl, scenario, 0)) != FM_OK) {
        printf("ERROR: fmCreateACL: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}

int xf_set_rule_condition(struct xf_acl_rule* acl_rule) {
    acl_rule->cond = FM_ACL_MATCH_SRC_IP | FM_ACL_MATCH_DST_IP | FM_ACL_MATCH_PROTOCOL | FM_ACL_MATCH_L4_SRC_PORT_WITH_MASK | FM_ACL_MATCH_L4_DST_PORT_WITH_MASK | FM_ACL_MATCH_INGRESS_PORT_SET;
    return 0;
}

int xf_set_rule_values(struct xf_acl_rule* acl_rule, struct xf_rule* rule) {
    /* set source IP */
    /* check if htonl is necessary */
    acl_rule->value.srcIp.addr[0] = htonl(rule->field[0].low & rule->field[0].high);
    acl_rule->value.srcIpMask.addr[0] = htonl(pow(2,32) - pow(2, ceil(log(rule->field[0].high - rule->field[0].low + 1) / log(2))));
    acl_rule->value.srcIp.isIPv6 = 0;
    acl_rule->value.srcIpMask.isIPv6 = 0;
    /* set destination IP */
    acl_rule->value.dstIp.addr[0] = htonl(rule->field[1].low & rule->field[1].high);
    acl_rule->value.dstIpMask.addr[0] = htonl(pow(2,32) - pow(2,ceil(log(rule->field[1].high - rule->field[1].low + 1) / log(2))));
    acl_rule->value.dstIp.isIPv6 = 0;
    acl_rule->value.dstIpMask.isIPv6 = 0;
    /* set source port */
     acl_rule->value.L4SrcStart = rule->field[2].low & rule->field[2].high;
     acl_rule->value.L4SrcMask = 16- ceil(log(rule->field[1].high
                                            - rule->field[1].low + 1) / log(2)) - 1;
    /* set destination port */
     acl_rule->value.L4DstStart = rule->field[3].low & rule->field[3].high;
     acl_rule->value.L4DstMask = 16- ceil(log(rule->field[1].high
                                            - rule->field[1].low + 1) / log(2)) - 1;
    /* set network protocol */
    acl_rule->value.protocol = rule->field[4].low;

    if (rule->field[4].low == 0) {
        acl_rule->value.protocolMask = 0;
    } else {
        acl_rule->value.protocolMask = 8;
    }

    return 0;
}

int xf_set_rule_action(struct xf_acl_rule* acl_rule, struct xf_rule* rule) {
    int fl = 1;      /*flag for flood and drop type*/
#if DEBUG >= 1
    printf("Number of actions in rule = %d\n",rule->n_actions);
    printf("Rule action = %s\n",rule->action[0]);
#endif
    if (strcmp(rule->action[0], "output") == 0) {
        acl_rule->action = FM_ACL_ACTIONEXT_REDIRECT | FM_ACL_ACTIONEXT_COUNT;
        acl_rule->param.logicalPort = rule->interface[0];
    } else if (strcmp(rule->action[0], "flood") == 0) {
        acl_rule->action = FM_ACL_ACTIONEXT_PERMIT | FM_ACL_ACTIONEXT_COUNT;
        fl = 0;
    } else if (strcmp(rule->action[0], "drop") == 0) {
        acl_rule->action = FM_ACL_ACTIONEXT_DENY | FM_ACL_ACTIONEXT_COUNT;
        fl = 0;
    } else {
        acl_rule->action = FM_ACL_ACTIONEXT_PERMIT | FM_ACL_ACTIONEXT_COUNT;
    }
    if(rule->n_actions > 1 && fl) {
        acl_rule->action = acl_rule->action | FM_ACL_ACTIONEXT_MIRROR_GRP;
        acl_rule->param.mirrorGrp = 1;
    } 

    return 0;
}

int xf_add_acl_rule(int* priority, struct xf_acl_rule* rule) {
    rule->priority = *priority;
    //rule->param = 0;
    if ((err = fmAddACLRuleExt(sw_acl, acl, rule->priority, rule->cond, &(rule->value), rule->action, &rule->param)) != FM_OK) {
        printf("ERROR: fmAddACLRuleExt: %s\n", fmErrorMsg(err));
        return -1;
    }

    //printf("Rule %d added to ACL 0\n",*priority);
    //fflush(stdout);
    return 0;
}

int xf_add_acl_to_port(fm_int* port, fm_aclType* type) {
    if ((err = fmAddACLPort(sw_acl, acl, *port, *type)) != FM_OK) {
        printf("ERROR: fmAddACLPort: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}


int xf_add_port_set(struct xf_acl_rule* acl_rule, struct xf_rule* rule) {
   
    fm_int port_set; 
    if ((err = fmCreateACLPortSet(sw_acl, &port_set)) != FM_OK) {
        printf("ERROR: fmCreateACLPortSet : %s\n",fmErrorMsg(err));
        return -1;
    }

    if(err = fmAddACLPortSetPort(sw_acl, port_set, rule->in_port) != FM_OK) {
        printf("ERROR: fmAddACLPortSetPort : %s\n",fmErrorMsg(err));
        return -1;
    } 
    acl_rule->value.portSet = port_set;
    return 0;
}

int xf_apply_acl() {
    fm_text statusText = (char*)malloc(200 * sizeof(char));
    fm_int statusTextLength = 200;  // for now, 200 chars should suffice
    fm_uint32 flags = FM_ACL_COMPILE_FLAG_NON_DISRUPTIVE;
    if ((err = fmCompileACLExt(sw_acl, statusText, statusTextLength, flags, NULL)) != FM_OK) {
        printf("ERROR: fmCompileACLExt: %s\n", fmErrorMsg(err));
        return -1;
    }
    flags = FM_ACL_APPLY_FLAG_NON_DISRUPTIVE;
    if ((err = fmApplyACL(sw_acl, flags)) != FM_OK) {
        printf("ERROR: fmApplyACL: %s\n", fmErrorMsg(err));
        return -1;
    } else {
#if DEBUG >= 1
         printf("ACL Applied \n");
         fflush(stdout);
#endif
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
    // printf("getting ACL stats for rule %d\n", *rule);
    if ((err = fmGetACLCountExt(sw_acl, acl, *rule, counters)) != FM_OK) {
        printf("ERROR: fmGetACLCount: %s\n", fmErrorMsg(err));
        return -1;
    }

    return 0;
}

int xf_insert_rule(struct xf_acl_rule *acl_rule, struct xf_rule* rule, int priority)
{
    int retval;
    //printf("insert for rule %d\n", priority);
    //char *op = "set cond";
    //timer_start(&t1);
    xf_set_rule_condition(acl_rule);
    //timer_end(op, &t1);
    //timer_start(&t2);
    xf_set_rule_values(acl_rule, rule);
    //op = "set values";
    //timer_end(op, &t2);
    //timer_start(&t3);
    xf_set_rule_action(acl_rule, rule);
    //op = "set action";
    //timer_end(op, &t3);
    //timer_start(&t4);
    if ((retval = xf_add_acl_rule(&priority, acl_rule)) != 0)
        return -1;
    //op = "add rule";
    //timer_end(op, &t4);

    return 0;
}

