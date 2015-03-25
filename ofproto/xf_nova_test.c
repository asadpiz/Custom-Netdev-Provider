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
 * Test file for NOVA1.
 */

#include "xf_stdinc.h"
#include "xf_hypercuts.h"
#include "xf_dheap.h"
#include "xf_list.h"
#include "xf_mtt.h"
#include "xf_acl.h"

#define TCAM 0

#define SLEEP sleep(1)
/*
 * Global variables
 */
int RUN = 0;
//int sw =0;
/* Function prototypes */
void* evictor(void* t);
/*
 * Eviction function for Evictor Thread
 */
void* evictor(void* t) {
    while (RUN) {
        SLEEP;
        printf("**********Evitor Thread**********\n");
        xf_mtt_sync_stats();
    }

    pthread_exit(NULL);
    return t;
}

/**
 *@breif             Main test function for NOVA1
 *
 *@param  argc       Number of arguments
 *@param  argv       Arguments from command line
 *
 *@return            0 for success, other means fail.
 */
int main(int argc, char* argv[]) {
    /* unsigned header[MAXDIMS]; */
    struct xf_lookup_match match;
    struct xf_tree* tree = &vtt_handle;
    struct xf_rule* rule;
    pthread_t evictor_thread;
    int  et, ej;
    /* initializations */
#ifndef OVS
    parseargs(tree, argc, argv);
#endif
    xf_tree_build(tree);
    /* xf_partition_rules(tree); */
    /* insertNode(tree); */
    xf_mtt_init();
    match.fid = INT_MAX;
    match.rule_id = -1;
    match.node_id = -1;
    xf_switch_init();
    printf("FM init done\n");
    /* Threads */
    et = pthread_create(&evictor_thread, NULL, evictor, NULL);

    if (et) {
        printf("ERROR; return code from pthread_create(evictor) is %d\n", et);
        exit(-1);
    }

    /* Threads code end */

    while (1) {
        fmWaitSemaphore(&recvSem, FM_WAIT_FOREVER);
        printf("******************%lu***********************\n",
               tree->num_packets + 1);
        printf("EVENT 1: packet %lu arrive at NOVA1\n", tree->num_packets + 1);
#if DEBUG >= 1
        printf("Packet %lu [%u %u %u %u %u]\n", tree->num_packets + 1,
               htonl(header[0]), htonl(header[1]), header[2], header[3], header[4]);
#endif
        xf_tree_lookup(tree, header, &match);

        if (match.status == LOOKUP_MATCH_EQUAL || match.status == LOOKUP_MATCH_HIGH || match.status == LOOKUP_MATCH_LOW) {
            printf("EVENT 3 : packet %lu match rule %d in node %d\n",
                   tree->num_packets, match.rule_id + 1, match.node_id);

            if (xf_mtt_ispresent(match.node_id)) {
                printf("EVENT 3(i) : Node %d already present in TCAM, apply action from rule %d\n",
                       match.node_id, match.rule_id + 1);
                /* Testing */
                /*  xf_mtt_delete(match.node_id); */
            } else {
                printf("EVENT 3(ii) : Node %d is not present in TCAM\n", match.node_id);
                #if TCAM == 0
                    rule = &tree->rule[match.rule_id];
                    fm_buffer*       buf;
                    fm_int           port;
                    int*             portList = malloc(80 * sizeof(int));
                    fm_status        status;
                    buf = fmAllocateBuffer(sw_acl);
                        
                    if (!buf) {
                    printf("Fail to allocate buffer\n");
                    return 1;
                    }

                    memcpy(buf->data, pkt_data, pkt_size);
                    buf->len = pkt_size; //sizeof(pkt_data);
		    printf("Interface %d \n",rule->interface);
                    if ((status = fmMapCardinalPort(sw_acl, rule->interface, &port, NULL))
                    != FM_OK) {
                    printf("Error: fmMapCardinalPort %s", fmErrorMsg(status));
                    return 1;                    
                    }
                    
                    portList[0] = port;
		    //portList[1] = 3;
                    //portList[2] = 2;
                    if(status = fmSendPacketDirected(sw_acl,portList,1,buf) != FM_OK) {
                    printf("Error: fmSendPacketDirected %s", fmErrorMsg(status));
                    }
		    else
			printf("Packet Sent to %d\n",port);

                #else
                if (xf_mtt_insert(tree, match.node_id)) {
                    printf("EVENT 3ab : Node %d inserted in MTT and TCAM\n", match.node_id);
                } else {
                    printf("EVENT 3ab : Error writing Node in MTT\n");
                }
                #endif
            }
        } else {
            printf("EVENT 2: packet %lu match NO rule, Send it to controller\n", tree->num_packets);
        }

        
       
                    xf_mtt_print();
    }

    print_lookup_stats(tree);
    RUN = 0;
    /* Destroy HAL,MTT and Tree */
    xf_mtt_exit();
    xf_tree_destroy(tree);
    /* Exit Threads */
    ej = pthread_join(evictor_thread, NULL);

    if (ej != 0) {
        printf("[ERROR] Return code from pthread_join() is %d\n", ej);
        return EXIT_FAILURE;
    } else {
        /* printf("Evictor Thread joined\n"); */
    }

    pthread_exit(&evictor_thread);
    return 0;
}
