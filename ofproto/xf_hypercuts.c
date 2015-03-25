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

#include "xf_hypercuts.h"
#ifdef OVS
#include "xf_stdinc.h"
#include <errno.h>
#include <netinet/in.h>
#include "byte-order.h"
#include "dynamic-string.h"
#include "flow.h"
#include "hash.h"
#include "odp-util.h"
#include "ofp-util.h"
#include "packets.h"
#include "xf_list.h"
#include "socket-util.h"
#include "ofproto-provider.h"
#include "ofpbuf.h"
#include "ofp-actions.h"
#endif

int set_default_args(struct xf_tree* tree) {
    /* Global Variables Values */
    tree->bucket_size = 64;                    /* leaf threashold */
    tree->spfac = 1.0;                         /* space explosion factor */
    tree->redun = 0;                           /* redundancy optimization */
    tree->push = 0;                            /* rule pushing optimization */
    tree->push_threshold = 100;                /* rule pushing threshhold */
    return 0;
}


#ifdef OVS
void xf_add_rule(struct ofproto* ofproto, struct xf_rule* rule) {
    uint64_t ofpacts_stub[32];
    struct ofpbuf buf;
    struct ofputil_flow_mod fm;
    struct ofpact_output *output[rule->n_actions];    //array of n_actions
    enum ofputil_protocol protocol;
    size_t i;
    int j;
    int typeFlag = 1;
    ofpbuf_use_stub(&buf, ofpacts_stub, sizeof(ofpacts_stub));
    match_init_catchall(&fm.match);
    fm.priority = rule->priority;
    fm.buffer_id = UINT32_MAX;
    fm.match.flow.dl_type = htons(0x0800); //htons(ETH_TYPE_IP); 
    fm.match.wc.masks.dl_type = 0xFFFF;

    fm.match.flow.in_port = rule->in_port;
    fm.match.wc.masks.in_port = UINT32_MAX;
    fm.table_id = 0;
    struct ofpact* temp;
    for(j =0 ; j < rule->n_actions; j++) {
        if (strcmp(rule->action[j], "output") == 0) {
            output[j] = ofpact_put_OUTPUT(&buf);
            output[j]->port = rule->interface[j];
            output[j]->max_len = 1;
        } else if (strcmp(rule->action[j], "flood") == 0) {
            output[j] = ofpact_put_OUTPUT(&buf);
            output[j]->port = OFPP_FLOOD;
            output[j]->max_len = 1;
            fm.ofpacts = xmemdup(buf.data, buf.size);
            fm.ofpacts_len = buf.size;
            typeFlag = 0;
            break;
        } else if (strcmp(rule->action[j], "drop") == 0) {
            output[j] = ofpact_put_OUTPUT(&buf);
            output[j]->port = OFPP_NONE;
            output[j]->max_len = 0;
            fm.ofpacts = xmemdup(buf.data, buf.size);
            fm.ofpacts_len = 0;
            typeFlag = 0;
            break;
        } else if (strcmp(rule->action[j],"mirror") == 0) {
            output[j] = ofpact_put_OUTPUT(&buf);
            output[j]->port = MIRROR_PORT;
            output[j]->max_len = 1;
        } else {
            output[j] = ofpact_put_OUTPUT(&buf);
            output[j]->port = OFPP_NORMAL;
            output[j]->max_len = 1;
            //fm.ofpacts = xmemdup(buf.data, buf.size);
            //fm.ofpacts_len = buf.size;
        }
    }
    if(typeFlag) {
        fm.ofpacts = xmemdup(buf.data, buf.size);
        fm.ofpacts_len = buf.size;
        fm.ofpacts->type = OFPACT_OUTPUT;
    }

      printf("length in add_rule %d\n",fm.ofpacts_len); 
    // populate match with rule
    // set source IP 
    fm.match.flow.nw_src = htonl(rule->field[0].low & rule->field[0].high);
    fm.match.wc.masks.nw_src = htonl(pow(2,32) - (rule->field[0].high - rule->field[0].low + 1) );
    //printf("FM SRC IP = %u, SRC IP MASK = %u \n", fm.match.flow.nw_src, fm.match.wc.masks.nw_src);

    // set destination IP 
    fm.match.flow.nw_dst = htonl(rule->field[1].low & rule->field[1].high);
    fm.match.wc.masks.nw_dst  = htonl(pow(2,32) - (rule->field[1].high - rule->field[1].low + 1) );
    //printf("FM DST IP = %u, DST IP MASK = %u \n", fm.match.flow.nw_dst, fm.match.wc.masks.nw_dst);

    // set source port
    if(rule->field[2].low == rule->field[2].high) {
         fm.match.flow.tp_src = htons(rule->field[2].low & rule->field[2].high);
         fm.match.wc.masks.tp_src  = 0xFFFF; //htons(pow(2,16)- (rule->field[2].high - rule->field[2].low + 1) );
    }
    //printf("FM SRC PORT = %u, SRC PORT MASK = %u \n", fm.match.flow.tp_src, fm.match.wc.masks.tp_src);

    // set destination port 
    if(rule->field[3].low == rule->field[3].high) {
    fm.match.flow.tp_dst = htons(rule->field[3].low & rule->field[3].high);
    fm.match.wc.masks.tp_dst = 0xFFFF; //htons(pow(2,16)- (rule->field[3].high - rule->field[3].low + 1) );
    }

    //printf("FM DST PORT = %u, DST PORT MASK = %u \n", fm.match.flow.tp_dst, fm.match.wc.masks.tp_dst);

    // set network protocol  
    if(rule->field[4].low == 0 && rule->field[4].high == 0xFF)
    {
    	fm.match.flow.nw_proto = 0;
        fm.match.wc.masks.nw_proto = 0;
    }
    else
    {
        fm.match.flow.nw_proto = rule->field[4].low; 
        fm.match.wc.masks.nw_proto = UINT8_MAX;
    }
    //printf("FM PROTO = %u, PROTO MASK = %u \n", fm.match.flow.nw_proto, fm.match.wc.masks.nw_proto);

    fm.command = OFPFC_ADD;
    int error = ofproto_flow_mod(ofproto, &fm);
    //xf_update_rule(,
    //printf("ofproto flow mod error = %s\n",strerror(error));

    //xf_ofproto_add_flow(rule, ofproto, &fm.match, rule->priority, fm.ofpacts, fm.ofpacts_len);
    //ofproto_add_flow(tree->ofproto, &fm.match, rule->priority, fm.ofpacts, fm.ofpacts_len);
/*
    printf("added xf_rule at: %u\n", (unsigned)rule);
    printf("added of_rule at: %u\n", (unsigned)rule->oftable_rule);
    printf("Rule added with field 0 low = %u high = %u\n",rule->field[0].low,rule->field[0].high);
    printf("Rule added with field 1 low = %u high = %u\n",rule->field[1].low,rule->field[1].high);
    printf("Rule added with field 2 low = %u high = %u\n",rule->field[2].low,rule->field[2].high);
    printf("Rule added with field 3 low = %u high = %u\n",rule->field[3].low,rule->field[3].high);
*/
}
#endif

/* parses arguments for hypercuts */
void parseargs(struct xf_tree* tree, int argc, char* argv[]) {
    int c;
    bit ok;
    set_default_args(tree);
    tree->fpr = fopen("acl1_10.txt", "r");
    tree->fpt = fopen("acl1_10_trace.txt", "r");

    if (tree->fpr == NULL) {
        printf("Error opening rule file\n");
    }

    if (tree->fpt == NULL) {
        printf("Error opening trace file\n");
    }

    ok = 1;

    while ((c = getopt(argc, argv, "b:s:r:t:hdp:")) != -1) {
        switch (c) {
        case 'b':
            tree->bucket_size = atoi(optarg);
            break;

        case 's':
            tree->spfac = atof(optarg);
            break;

        case 'r':
            if (tree->fpr != NULL) {
                fclose(tree->fpr);
            }

            tree->fpr = fopen(optarg, "r");
            break;

        case 't':
            if (tree->fpt != NULL) {
                fclose(tree->fpt);
            }

            tree->fpt = fopen(optarg, "r");
            break;

        case 'd':
            tree->redun = 1;
            break;

        case 'p':
            tree->push = 1;
            tree->push_threshold = atoi(optarg);
            break;

        case 'h':
            printf("hypercut [-b bucketSize][-s spfac][-d][-p pushthreshold] ");
            printf("[-r ruleset][-t trace]\n");
            exit(1);
            break;

        default:
            ok = 0;
        }
    }

    if (tree->bucket_size <= 0 || tree->bucket_size > MAXBUCKETS) {
        printf("bucket_size should be greater than 0 and less than %d\n",
               MAXBUCKETS);
        ok = 0;
    }

    if (tree->spfac < 0) {
        printf("space factor should be >= 0\n");
        ok = 0;
    }

    if (tree->push_threshold < 0) {
        printf("Push threshold should be >= 0\n");
        ok = 0;
    }

    if (tree->fpr == NULL) {
        printf("can't open ruleset file\n");
        ok = 0;
    }

    if (!ok || optind < argc) {
        fprintf(stderr, "hypercut [-b bucketsize][-s spfac][-d]");
        fprintf(stderr, "[-p push_threshold][-r ruleset][-t trace]\n");
        fprintf(stderr, "Type \"hypercut -h\" for help\n");
        exit(1);
    }

    printf("*************************\n");
    printf("Bucket Size =  %d\n", tree->bucket_size);
    printf("Space Factor = %f\n", tree->spfac);

    if (tree->redun == 1) {
        printf("Remove redundancy optimization on\n");
    } else {
        printf("Remove redundancy optimization off\n");
    }

    if (tree->push == 1) {
        printf("Rule pushing optimization on, threshold %d\n", tree->push_threshold);
    } else {
        printf("Rule pushing optimization off\n");
    }
}

/* loads rules from ruleset */
/*
int xf_load_rules(struct xf_tree* tree) {
    printf("in load rules\n");
    int i = 0, j = 0, k = 0;
    unsigned tmp;
    unsigned sip1, sip2, sip3, sip4, siplen;
    unsigned dip1, dip2, dip3, dip4, diplen;
    unsigned proto, protomask;
    char* s = (char*)calloc(300, sizeof(char));
    struct xf_rule* rule;
#ifdef OVS
    tree->fpr = fopen("rules", "r");

    if (tree->fpr == NULL) {
        printf("Error opening rule file\n");
        return -1;
    }

#endif

    while (fgets(s, 300, tree->fpr) != NULL) {
        k++;
    }

    rewind(tree->fpr);
    tree->rule = (struct xf_rule*)calloc(k, sizeof(struct xf_rule));
    rule = tree->rule;

    while (1) {
        if (i == k) { 
            printf("i == k\n");
            break;
        }

        rule[i].interface = 0;
        rule[i].in_port = 0;
        j = fscanf(tree->fpr, "@%u.%u.%u.%u/%u\t%u.%u.%u.%u/%u\t%u : %u\t%u : %u\t%x/%x\t%s : %hu\t%u\t\n",
                   &sip1, &sip2, &sip3, &sip4, &siplen, &dip1, &dip2, &dip3, &dip4, &diplen,
                   &rule[i].field[2].low, &rule[i].field[2].high, &rule[i].field[3].low,
                   &rule[i].field[3].high, &proto, &protomask, rule[i].action, &rule[i].interface,rule[i].in_port);
        printf("j = %d\n",j);
        if (j < 17) {
            printf("j < 17\n");
            break;
        }
        printf("read from file\n");
        if (siplen == 0) {
            rule[i].field[0].low = 0;
            rule[i].field[0].high = 0xFFFFFFFF;
        } else if (siplen > 0 && siplen <= 8) {
            tmp = sip1 >> (8-siplen);
	    tmp = tmp << (32 - siplen);
            rule[i].field[0].low = tmp;
            rule[i].field[0].high = rule[i].field[0].low + (1 << (32 - siplen)) - 1;
        } else if (siplen > 8 && siplen <= 16) {
            tmp = sip1 << 24;
            sip2= sip2 >> (16-siplen);
            sip2 = sip2 << (32-siplen);
	    tmp += sip2;
            rule[i].field[0].low = tmp;
            rule[i].field[0].high = rule[i].field[0].low + (1 << (32 - siplen)) - 1;
        } else if (siplen > 16 && siplen <= 24) {
            tmp = sip1 << 24;
            tmp += sip2 << 16;
            sip3 = sip3 >> (24-siplen);
            sip3 = sip3 << (32-siplen);
            tmp += sip3;
            rule[i].field[0].low = tmp;
            rule[i].field[0].high = rule[i].field[0].low + (1 << (32 - siplen)) - 1;
        } else if (siplen > 24 && siplen <= 32) {
            tmp = sip1 << 24;
            tmp += sip2 << 16;
            tmp += sip3 << 8;
            tmp += sip4 >> (32 - siplen);
            rule[i].field[0].low = tmp;
            rule[i].field[0].high = rule[i].field[0].low + (1 << (32 - siplen)) - 1;
        } else {
            printf("Src IP length exceeds 32\n");
            return 0;
        }

        if (diplen == 0) {
            rule[i].field[1].low = 0;
            rule[i].field[1].high = 0xFFFFFFFF;
        } else if (diplen > 0 && diplen <= 8) {
            tmp = dip1 << 24;
            rule[i].field[1].low = tmp;
            rule[i].field[1].high = rule[i].field[1].low + (1 << (32 - diplen)) - 1;
        } else if (diplen > 8 && diplen <= 16) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            rule[i].field[1].low = tmp;
            rule[i].field[1].high = rule[i].field[1].low + (1 << (32 - diplen)) - 1;
        } else if (diplen > 16 && diplen <= 24) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            tmp += dip3 << 8;
            rule[i].field[1].low = tmp;
            rule[i].field[1].high = rule[i].field[1].low + (1 << (32 - diplen)) - 1;
        } else if (diplen > 24 && diplen <= 32) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            tmp += dip3 << 8;
            tmp += dip4;
            rule[i].field[1].low = tmp;
            rule[i].field[1].high = rule[i].field[1].low + (1 << (32 - diplen)) - 1;
        } else {
            printf("Dest IP length exceeds 32\n");
            return 0;
        }

        if (protomask == 0xFF) {
            rule[i].field[4].low = proto;
            rule[i].field[4].high = proto;
        } else if (protomask == 0) {
            rule[i].field[4].low = 0;
            rule[i].field[4].high = 0xFF;
        } else {
            printf("Protocol mask error\n");
            return 0;
        }

        //xf_add_rule(tree, &rule[i]);
	printf("rule %d added\n",i);

        rule[i].priority = i;     
        rule[i].num_derived_rules = 0;
        rule[i].derived_from = 0;
        rule[i].derived_rules = NULL;
        i++;
    }

    if (k != i) {
        printf("The number of rules in file %d != number of rules loaded %d \n",
               k, i);
        exit(0);
    } else {
        printf("The number of rules = %d\n", k);
        tree->num_loaded_rules = k;
    }

#if DEBUG >= 1
    printbounds(tree);
#endif
    // tree->rule = rule; 
    free(s);
    return i;  // number of rules loaded in rule variable
}
*/

/* prints the max and min element from ruleset to find range of fields */
void printbounds(struct xf_tree* tree) {
    int i = 0, k = 0;
    struct xf_range f[MAXDIMS];
    int wc[MAXDIMS], em[MAXDIMS];
    int r = 0;
    int total_wc = 0, total_em = 0, total_r = 0;
    struct xf_rule* rule = tree->rule;

    for (k = 0; k < MAXDIMS; k++) {
        f[k].high = 0;

        if (k == 4) {
            f[k].low = 0xFF;
        } else if (k < 2) {
            f[k].low = 0xFFFFFFFF;
        } else {
            f[k].low = 0xFFFF;
        }

        wc[k] = 0;
        em[k] = 0;
    }

    for (i = 0; i < tree->num_loaded_rules; i++) {
        for (k = 0; k < MAXDIMS; k++) {
            if (rule[i].field[k].low < f[k].low) {
                f[k].low = rule[i].field[k].low;
            }

            if (rule[i].field[k].high > f[k].high) {
                f[k].high = rule[i].field[k].high;
            }

            if (rule[i].field[k].low == rule[i].field[k].high) {
                em[k]++;
                /* printf("%u ",rule[i].field[k].low); */
            } else if (rule[i].field[k].low == 0) {
                if (k == 4 && rule[i].field[k].high == 0xFF) {
                    wc[k]++;
                    /* printf("* "); */
                } else if (k < 2 && rule[i].field[k].high == 0xFFFFFFFF) {
                    wc[k]++;
                    /* printf("* "); */
                } else if (rule[i].field[k].high == 0xFFFF) {
                    wc[k]++;
                    /* printf("* "); */
                } else {
                    /* printf("%u : %u ",rule[i].field[k].low,rule[i].field[k].high); */
                }
            } else {
                /* printf("%u : %u ",rule[i].field[k].low,rule[i].field[k].high); */
            }

            /* all */
            printf("%u : %u ",htonl(rule[i].field[k].low),htonl(rule[i].field[k].high));
        }

        printf("\n");
    }

    printf("*************************\n");

    for (k = 0; k < MAXDIMS; k++) {
        total_wc += wc[k];
        total_em += em[k];
        r = tree->num_loaded_rules - (em[k] + wc[k]);
        total_r += r;
        printf("Field[%i] WC[%d->%0.2f%%] EM[%d->%0.2f%%] RN[%d->%0.2f%%] Range[%u:%u]\n",
               k, wc[k], (((float)(wc[k]) / tree->num_loaded_rules) * 100),
               em[k], (((float)(em[k]) / tree->num_loaded_rules) * 100),
               r, (((float)(r) / tree->num_loaded_rules) * 100),
               f[k].low, f[k].high);
    }

    printf("Field[*] WC[%d/%d->%0.2f%%] EM[%d/%d->%0.2f%%] EM[%d/%d->%0.2f%%]\n",
           total_wc, tree->num_loaded_rules * MAXDIMS,
           (((float)(total_wc) / (tree->num_loaded_rules * MAXDIMS)) * 100),
           total_em, tree->num_loaded_rules * MAXDIMS,
           (((float)(total_em) / (tree->num_loaded_rules * MAXDIMS)) * 100),
           total_r, tree->num_loaded_rules * MAXDIMS,
           (((float)(total_r) / (tree->num_loaded_rules * MAXDIMS)) * 100));
}

void print_unique_components(struct xf_tree* tree, int ncomponent[]) {
    int total_component = 0, k;
    printf("*************************\n");

    for (k = 0; k < MAXDIMS; k++) {
        printf("Field[%d] Unique Components[%d->%0.2f%%]\n", k, ncomponent[k],
               (((float)ncomponent[k] / tree->num_rules) * 100));
        total_component += ncomponent[k];
    }

    printf("Field[*] Unique Components [%d/%d->%0.2f%%]\n",
           total_component, tree->num_loaded_rules * MAXDIMS,
           ((float)total_component / (tree->num_rules * MAXDIMS)) * 100);
    printf("*************************\n");
}

void xf_choose_np_dim(struct xf_tree* tree, struct xf_node* v) {
    int nc[MAXDIMS];                /* number of cuts in each dim */
    int maxnc, minnc;               /* maximum and minimum number of cuts */
    int NC;                         /* number of child of node */
    int* nr[MAXDIMS];               /* number of rules in each child */
    int ncomponent[MAXDIMS];        /* unique component in each dimension */
    int done;
    float avgcomponent;
    int i, j, k;
    int lo, hi, r;
    int temp;
    keytyp tmpkey;
    struct dheap* H1 = dheap_init(MAXRULES, 2);
    struct dheap* H2 = dheap_init(MAXRULES, 2);
    int   Nr;
    int flag = 1;
    struct xf_rule* rule = tree->rule;

    for (k = 0; k < MAXDIMS; k++) {
        nr[k] = NULL;    /* sohaib nr[k] is int* */
    }

    /* count the unique components on each dimension Result=ncomponent[k] */
    for (k = 0; k < MAXDIMS; k++) 
    {  /* for all dims */
        ncomponent[k] = 0;

        for (i = 0; i < v->node_rules; i++) {
            if (rule[v->rule_id[i]].field[k].low < v->field[k].low) {  /* rule boundry  with node boundry */
                dheap_insert(H1, v->rule_id[i], v->field[k].low);      /* insert greater one in H1 */
            } else {
                dheap_insert(H1, v->rule_id[i], rule[v->rule_id[i]].field[k].low);
            }
        }

        while (dheap_findmin(H1) != Null) {  /* H1 not empty */
            temp = dheap_findmin(H1);
            tmpkey = dheap_key(H1, temp);

            while (tmpkey == dheap_key(H1, dheap_findmin(H1))) {  /* add in H2 while value is equal */
                j = dheap_deletemin(H1);

                if (rule[j].field[k].high > v->field[k].high) {   /* insert lower one in H2 */
                    dheap_insert(H2, j, v->field[k].high);
                } else {
                    dheap_insert(H2, j, rule[j].field[k].high);
                }
            }

            while (dheap_findmin(H2) != Null) {  /* while H2 is not empty */
                ncomponent[k]++;
                temp = dheap_findmin(H2);
                tmpkey = dheap_key(H2, temp);

                while (dheap_findmin(H2) != Null && dheap_key(H2, temp) == tmpkey) {
                    dheap_deletemin(H2);
                    temp = dheap_findmin(H2);
                }
            }
        }
    }

#if DEBUG >= 1

    if (!tree->tree_pass) {
        print_unique_components(tree, ncomponent);
    }

#endif

    /* choose the set of dimensions to cut result=v->dim[k] */
    if (ncomponent[0] == 1 && ncomponent[1] == 1 && ncomponent[2] == 1 &&
            ncomponent[3] == 1 && ncomponent[4] == 1) {
        for (k = 0; k < MAXDIMS; k++) {
            v->dim[k] = 0;
            v->n_cuts[k] = 1;
        }

        return;
    }

    avgcomponent = 0.0;

    for (k = 0; k < MAXDIMS; k++) {
        avgcomponent += ncomponent[k];
    }

    avgcomponent = avgcomponent / MAXDIMS;

    for (k = 0; k < MAXDIMS; k++) {
        if (ncomponent[k] > avgcomponent &&
                ((v->field[k].high - v->field[k].low) > 1)) {  /* boundry should be greater than 1 */
            v->dim[k] = 1;
            nc[k] = 2;
        } else {
            v->dim[k] = 0;
            nc[k] = 1;
        }
    }

    /* choose the number of cuts */
    for (k = 0; k < MAXDIMS; k++) {
        if (v->dim[k] == 1) {
            done = 0;

            while (!done) {
                nr[k] = (int*)realloc(nr[k], nc[k] * sizeof(int));  /* nc[k]=2 */

                for (i = 0; i < nc[k]; i++) {
                    nr[k][i] = 0;    /* nr[dim][nc] */
                }

                for (j = 0; j < v->node_rules; j++) {  /* for all rules */
                    r = (v->field[k].high - v->field[k].low) / nc[k];
                    lo = v->field[k].low;
                    hi = lo + r;

                    for (i = 0; i < nc[k]; i++) {  /* for all cuts */
                        if (rule[v->rule_id[j]].field[k].low <= hi &&
                                rule[v->rule_id[j]].field[k].high >= lo) {
                            nr[k][i]++;  /* rules in k dim and i-th cut */
                        }

                        lo = hi + 1;
                        hi = lo + r;
                    }
                }

                Nr = nc[k];

                for (i = 0; i < nc[k]; i++) {
                    Nr += nr[k][i];   /* Nr is cuts in this dim and rules in alls cuts of dim */
                }

                /* if true then nc in this dim * 2 else done */
                if (Nr < tree->spfac * v->node_rules &&
                        v->field[k].high - v->field[k].low > nc[k] &&
                        nc[k] <= MAXCUTS / 2) {
                    nc[k] = nc[k] * 2;
                } else {
                    done = 1;
                }
            }
        }
    }

    NC = 1;

    for (k = 0; k < MAXDIMS; k++) {
        if (v->dim[k] == 1) {
            NC = NC * nc[k];
        }
    }

/*    
    printf("<<NC = %d (%d:%d:%d:%d:%d) @ layer %d DIM(%d:%d:%d:%d:%d)\n",
            NC, nc[0], nc[1], nc[2], nc[3], nc[4], tree->tree_pass,
            v->dim[0],v->dim[1], v->dim[2], v->dim[3], v->dim[4]);
*/
    while (NC > tree->spfac * sqrt(v->node_rules) || NC > MAXCUTS) 
    {      
        //printf("spfac = %d\n", tree->spfac);
        //printf("v->node_rules = %d\n", v->node_rules);
        //printf("this: %d\n", tree->spfac * sqrt(v->node_rules));
        //printf("NC = %d\n", NC);
        
        maxnc = 0;
        minnc = MAXCUTS + 1;

        for (k = 0; k < MAXDIMS; k++) {
            if (v->dim[k] == 1) {  /* && nc[k] != 2){ */
                if (maxnc < nc[k]) {
                    maxnc = nc[k];
                }

                if (minnc > nc[k]) {
                    minnc = nc[k];
                }
            }
        }

        for (k = MAXDIMS - 1; k >= 0; k--) {  /* 4 to 0 dims */
            if (v->dim[k] == 1) {
                if (flag == 1 && minnc == nc[k]) {  /* if flag 1 and nc is min */
                    nc[k] = nc[k] / 2;

                    if (nc[k] == 1) {
                        v->dim[k] = 0;
                    }

                    break;
                } else if (flag == 0 && maxnc == nc[k]) {  /* if flag 0 and nc is max */
                    nc[k] = nc[k] / 2;

                    if (nc[k] == 1) {
                        v->dim[k] = 0;
                    }

                    break;
                }
            }
        }

        NC = 1;

        for (k = 0; k < MAXDIMS; k++) {
            if (v->dim[k] == 1) {
                NC = NC * nc[k];
            }
        }

        /* if(flag == 1) flag = 0; */
        /* else flag = 1; */
    }


    for (k = 0; k < MAXDIMS; k++) {
        if (v->dim[k] == 1) {
            v->n_cuts[k] = nc[k];
        } else {
            v->n_cuts[k] = 1;
        }
    }
/*
    printf(">>NC = %d (%d:%d:%d:%d:%d) @ layer %d with %d rules DIM (%d:%d:%d:%d:%d)\n",
            NC,v->n_cuts[0], v->n_cuts[1], v->n_cuts[2], v->n_cuts[3],v->n_cuts[4],
            tree->tree_pass,v->node_rules,v->dim[0],v->dim[1],v->dim[2],v->dim[3],v->dim[4]); 
*/
    v->child = (int*)malloc( NC * sizeof(int));

    for (k = 0; k < MAXDIMS; k++) 
        free(nr[k]);

    dheap_exit(H1);  /* free Memory */
    dheap_exit(H2);
}

void xf_remove_redundancy(struct xf_tree* tree, struct xf_node* v) {
    int cover;
    int tmp, tmp2;
    int i, k, j;
    struct xf_rule* rule = tree->rule;

    if (v->node_rules == 1) {
        return;
    }

    tmp = v->node_rules - 1;
    tmp2 = v->node_rules - 2;

    while (tmp >= 1) {
        for (i = tmp2; i >= 0; i--) {
#if DEBUG >= 1
            printf("Comparing Rule ID %d and %d\n", v->rule_id[tmp], v->rule_id[i]);
#endif
            for (k = 0; k < MAXDIMS; k++) {
                cover = 1;
#if DEBUG >= 2                
                printf("%u %u \n %u %u \n %u %u \n %u %u\n",
                       rule[v->rule_id[i]].field[k].low, v->field[k].low,
                       rule[v->rule_id[tmp]].field[k].low, v->field[k].low,
                       rule[v->rule_id[i]].field[k].high, v->field[k].high,
                       rule[v->rule_id[tmp]].field[k].high, v->field[k].high);

#endif                

                if (
                    max_unsigned(rule[v->rule_id[i]].field[k].low, v->field[k].low) >
                    max_unsigned(rule[v->rule_id[tmp]].field[k].low, v->field[k].low)
                    ||
                    min_unsigned(rule[v->rule_id[i]].field[k].high, v->field[k].high) <
                    min_unsigned(rule[v->rule_id[tmp]].field[k].high, v->field[k].high)) {
                    /* printf("break at dim %d\n", k); */
                    cover = 0;
                    break;
                }
            }

            if (strcmp(rule[v->rule_id[i]].action, rule[v->rule_id[tmp]].action) != 0 ||
                    rule[v->rule_id[i]].interface != rule[v->rule_id[tmp]].interface) {
                cover = 0;
                break;
            }

            if (cover == 1) {
#if DEBUG >= 1
                printf("rule %d is covered by %d, becomes %d rules @ layer %d\n",
                       v->rule_id[tmp], v->rule_id[i], v->node_rules - 1, tree->tree_pass);
#endif

                for (j = tmp; j < v->node_rules - 1; j++) {
                    v->rule_id[j] = v->rule_id[j + 1];
                }

                v->node_rules--;
                tree->num_redun_rules++;
                break;
            }
        }

        tmp--;
        tmp2--;
    }
}

void xf_pushing_rule(struct xf_tree* tree, struct xf_node* v) {
    //printf("rule pushing 1\n");
    int i, j, k;
    int idx = 0;
    int cover = 0;
    struct xf_rule* rule = tree->rule;
#if DEBUG >= 1
    int tmp = v-> node_rules;
#endif

    for (i = 0; i < v->node_rules; i++) {
        if (cover == 1) {
            i--;
        }

        cover = 1;

        for (k = 0; k < MAXDIMS; k++) {
            if (v->dim[k] == 1) {
                if (rule[v->rule_id[i]].field[k].low > v->field[k].low ||
                        rule[v->rule_id[i]].field[k].high < v->field[k].high) {
                    cover = 0;
                    break;
                }
            }
        }

        if (cover == 1) {
            v->empty_list = 0;
            v->rule_list = (int*)realloc(v->rule_list, (idx + 1) * sizeof(int));
            v->rule_list[idx] = v->rule_id[i];

            for (j = i + 1; j < v->node_rules; j++) {
                v->rule_id[j - 1] = v->rule_id[j];
            }

            v->node_rules--;
            idx++;
            v->list_length = idx;
            tree->num_stored_rules++;          /* stored rules */
            tree->num_internal_rules++;        /* internally stored rules */
            /* only hold 1!!! */
            /* break;  */
        }
    }

#if DEBUG >= 1

    if (idx != 0) {
        printf("hold %d rules @ layer %d, %d => %d rules\n",
               idx, tree->tree_pass, tmp, v->node_rules);
    }

#endif
}

void xf_tree_init(struct xf_tree* tree, struct ofproto* ofp) {
    struct xf_node* node_set;
    int root = 1;               /* index of root node */
    int j = 0;
    /* tree stats */
    tree->ofproto = ofp;

    if (ofp == NULL) {
        printf("OFP is NULL.\n");
    }

    set_default_args(tree);
    tree->build_status = 0;
    tree->num_rules = 0; //xf_load_rules(tree);
    tree->tree_pass = 0;               /* current level of tree */
    tree->num_nodes = 1;
    tree->node_set = NULL;
    tree->num_redun_rules = 0;
    tree->num_stored_rules = 0;
    tree->cost = 0;
    tree->num_internal_rules = 0;
    tree->num_accesses = 0;
    tree->lookup_time = 0;
    tree->worstcost = 0.0;
    tree->worstaccesses = 0;
    tree->worsttime = 0.0;
    tree->bestcost = BIGINT;
    tree->bestaccesses = BIGINT;
    tree->besttime = BIGINT;
    tree->num_packets = 0;
    tree->matched_packets = 0;
    tree->node_set = (struct xf_node*)realloc(tree->node_set, (tree->num_nodes + 1) * sizeof(struct xf_node));
    node_set = tree->node_set;
    node_set[root].is_leaf = 0;
    node_set[root].node_rules = tree->num_rules;
    node_set[root].empty_list = 1;
    node_set[root].rule_list = NULL;

    for (j = 0; j < MAXDIMS; j++) {
        node_set[root].field[j].low = 0;

        if (j < 2) {
            node_set[root].field[j].high = 0xFFFFFFFF;
        } else if (j == 4) {
            node_set[root].field[j].high = 0xFF;
        } else {
            node_set[root].field[j].high = 0xFFFF;
        }

        node_set[root].dim[j] = 0;
        node_set[root].n_cuts[j] = 1;
    }

    /*
    node_set[root].rule_id = (int*)calloc(tree->num_rules, sizeof(int));

    for (j = 0; j < tree->num_rules; j++) {
        node_set[root].rule_id[j] = j;
    }

    node_set[root].child = (int*)malloc(sizeof(int));
    node_set[root].child[0] = Null; 
    */
    /*  Reverse priority
        for(j=tree->num_loaded_rules;j>=0;j--)
        {
            tree->rule[j].priority=tree->num_loaded_rules-j;
        }
    */
}

void xf_tree_build(struct xf_tree* tree) {
    printf("in tree build\n");
    struct xf_list* Q = xf_list_init(MAXNODES);
    int last;
    int nr;
    int empty;
    int u, v;
    int i[MAXDIMS];
    int r[MAXDIMS], lo[MAXDIMS], hi[MAXDIMS];
    int idx, cover;
    int j, k, s, t, root = 1;
    struct xf_rule* rule;
    struct xf_node* node_set;
#ifndef OVS
    //xf_tree_init(tree);
#endif
    rule = tree->rule;
    node_set = tree->node_set;
    xf_list_append(Q, root);
    last = root;

    while (xf_list_get(Q, 1) != Null) {
        v = xf_list_get(Q, 1);
        xf_list_trunc(Q, 1);

        if (tree->redun == 1) {
            xf_remove_redundancy(tree, &node_set[v]);
        }

        if (node_set[v].node_rules > tree->bucket_size) {
            xf_choose_np_dim(tree, &node_set[v]);

            if (tree->push == 1 && tree->tree_pass <= tree->push_threshold) {
                xf_pushing_rule(tree, &node_set[v]);
            }
        }

//        printf("tree_pass %d  Node[%d] Rules : %d\n",
//               tree->tree_pass, v, node_set[v].node_rules);
#if DEBUG >= 2
        print_node_stat(tree, v, 1);
#endif

        if (node_set[v].node_rules <= tree->bucket_size) {
            node_set[v].is_leaf = 1;
            tree->num_stored_rules += node_set[v].node_rules;

            printf("Leaf Node[%d] with %d Rules!\n", v , node_set[v].node_rules);
#if DEBUG >= 2
            print_node_stat(tree, v, 1);
#endif

        } else {
            idx = 0;
            r[0] = (node_set[v].field[0].high - node_set[v].field[0].low) /
                   node_set[v].n_cuts[0];
            lo[0] = node_set[v].field[0].low;
            hi[0] = lo[0] + r[0];

            /* iterate all feilds to number of cuts */
            for (i[0] = 0; i[0] < node_set[v].n_cuts[0]; i[0]++) {
                r[1] = (node_set[v].field[1].high - node_set[v].field[1].low) /
                       node_set[v].n_cuts[1];
                lo[1] = node_set[v].field[1].low;
                hi[1] = lo[1] + r[1];

                for (i[1] = 0; i[1] < node_set[v].n_cuts[1]; i[1]++) {
                    r[2] = (node_set[v].field[2].high - node_set[v].field[2].low) /
                           node_set[v].n_cuts[2];
                    lo[2] = node_set[v].field[2].low;
                    hi[2] = lo[2] + r[2];

                    for (i[2] = 0; i[2] < node_set[v].n_cuts[2]; i[2]++) {
                        r[3] = (node_set[v].field[3].high - node_set[v].field[3].low) /
                               node_set[v].n_cuts[3];
                        lo[3] = node_set[v].field[3].low;
                        hi[3] = lo[3] + r[3];

                        for (i[3] = 0; i[3] < node_set[v].n_cuts[3]; i[3]++) {
                            r[4] = (node_set[v].field[4].high - node_set[v].field[4].low) /
                                   node_set[v].n_cuts[4];
                            lo[4] = node_set[v].field[4].low;
                            hi[4] = lo[4] + r[4];

                            for (i[4] = 0; i[4] < node_set[v].n_cuts[4]; i[4]++) {
                                empty = 1;  /* initially node is empty */
                                nr = 0;

                                for (j = 0; j < node_set[v].node_rules; j++) {
                                    cover = 1;   /* that rule lie in the range (same as cover in lookup) */

                                    for (k = 0; k < MAXDIMS; k++) {
                                        if (rule[node_set[v].rule_id[j]].field[k].low > hi[k] ||
                                                rule[node_set[v].rule_id[j]].field[k].high < lo[k]) {
                                            cover = 0;  /* that rule don't lie in the boundry for any of dim */
                                            break;
                                        }
                                    }

                                    if (cover == 1) {
                                        empty = 0;   /* node is not empty. */
                                        nr++;
                                    }
                                }

                                if (!empty) {
                                    tree->num_nodes++;
                                    u = tree->num_nodes;
                                    node_set[v].child[idx] = u;  /* realloced in np_dim */
                                    tree->node_set = (struct xf_node*)realloc(tree->node_set, (tree->num_nodes + 1) * sizeof(struct xf_node));
                                    node_set = tree->node_set;
                                    node_set[u].child = (int*)malloc(sizeof(int));   /* init child 0 */
                                    node_set[u].child[0] = Null;
                                    node_set[u].node_rules = nr;
                                    node_set[u].empty_list = 1;
                                    node_set[u].rule_list = NULL;

                                    if (nr <= tree->bucket_size) {
                                        node_set[u].is_leaf = 1;
                                        tree->num_stored_rules += nr;
#if DEBUG >= 1
                                        printf("Leaf\tNode[%d]=>Node[%d].Child[%d] Rules : %d\n",
                                               u, v, idx, nr);
#endif
                                    } else {
                                        node_set[u].is_leaf = 0;
                                        xf_list_append(Q, u);
#if DEBUG >= 1
                                        printf("NonLeaf Node[%d]=>Node[%d].Child[%d] Rules : %d\n",
                                               u, v, idx, nr);
#endif
                                    }

                                    for (t = 0; t < MAXDIMS; t++) {
                                        if (node_set[v].dim[t] == 1) {  /* if dim to cut then update range else don't */
                                            node_set[u].field[t].low = lo[t];
                                            node_set[u].field[t].high = hi[t];
                                        } else {
                                            node_set[u].field[t].low = node_set[v].field[t].low;
                                            node_set[u].field[t].high = node_set[v].field[t].high;
                                        }
                                    }

                                    s = 0;
                                    node_set[u].rule_id = (int*)calloc(node_set[v].node_rules, sizeof(int));

                                    for (j = 0; j < node_set[v].node_rules; j++) {
                                        cover = 1;

                                        for (k = 0; k < MAXDIMS; k++) {
                                            if (node_set[v].dim[k] == 1) {
                                                if (rule[node_set[v].rule_id[j]].field[k].low > hi[k] ||
                                                        rule[node_set[v].rule_id[j]].field[k].high < lo[k]) {
                                                    cover = 0;
                                                    break;
                                                }
                                            }
                                        }

                                        if (cover == 1) {
                                            node_set[u].rule_id[s] = node_set[v].rule_id[j];
                                            s++;
                                        }
                                    }

#if DEBUG >= 2
                                    print_node_stat(tree, u, 0);  /* ncuts not calculated */
#endif
                                } else {
                                    node_set[v].child[idx] = Null;
#if DEBUG >= 1
                                    printf("Null\tNode[0]=>Node[%d].Child[%d] Rules : 0\n",
                                           v, idx);
#endif
#if DEBUG >= 3
                                    printf("**lo,hi=[%u:%u,%u:%u,%u:%u,%u:%u,%u:%u]\n",
                                           lo[0], hi[0], lo[1], hi[1], lo[2], hi[2],
                                           lo[3], hi[3], lo[4], hi[4]);
#endif
                                }

                                idx++;
                                lo[4] = hi[4] + 1;
                                hi[4] = lo[4] + r[4];
                            }

                            lo[3] = hi[3] + 1;
                            hi[3] = lo[3] + r[3];
                        }

                        lo[2] = hi[2] + 1;
                        hi[2] = lo[2] + r[2];
                    }

                    lo[1] = hi[1] + 1;
                    hi[1] = lo[1] + r[1];
                }

                lo[0] = hi[0] + 1;
                hi[0] = lo[0] + r[0];
            }

            if (v == last) {
                tree->tree_pass++;
                last = xf_list_tail(Q);
                /* printf("Level=%d\n",tree->tree_pass); */
            }
        }  /* if tree->node_rules > bucket size */
    }  /* end while(xf_list_get(Q,1) != Null) */

    xf_list_exit(Q);
    /*printf("tree build done\n"); */
    printf("tree num_nodes = %d\n", tree->num_nodes);
#if DEBUG >= 4
    print_tree_stat(tree);
#endif
#if DEBUG >= 1
    printf("*************************\n");
    printf("Number of nodes = %d\n", tree->num_nodes);
    printf("Max trie depth = %d\n", tree->tree_pass);
    printf("Remove redun = %d\n", tree->num_redun_rules);
    printf("Stored rules = %d\n", tree->num_stored_rules);
    printf("Internally stored rules = %d\n", tree->num_internal_rules);
    printf("Bytes/filter = %.2f\n", (tree->num_nodes * NODESIZE + tree->num_rules * RULESIZE + tree->num_stored_rules * RULEPTSIZE)
                                      * 4 / tree->num_rules);
    printf("*************************\n");
#endif
    tree->build_status = 1;

}


void print_node_stat(struct xf_tree* tree, int node_id, int IsPrint) {
    printf("printing node %d states\n",node_id);
    int i = node_id, j;
    struct xf_node* node_set;
    node_set = tree->node_set;

    if (tree == NULL) {
        printf("Tree is NULL\n");
    }

    if (tree->node_set == NULL) {
        printf("NodeSet is NULL\n");
    }

    if (IsPrint) {
        printf(" ");
    }

#if DEBUG >= 3
    printf("**lo,hi=[%u:%u,%u:%u,%u:%u,%u:%u,%u:%u]\n",
           node_set[i].field[0].low, node_set[i].field[0].high,
           node_set[i].field[1].low, node_set[i].field[1].high,
           node_set[i].field[2].low, node_set[i].field[2].high,
           node_set[i].field[3].low, node_set[i].field[3].high,
           node_set[i].field[4].low, node_set[i].field[4].high);

    if (!node_set[i].is_leaf && IsPrint)
        printf("**dim[%d,%d,%d,%d,%d] n_cuts[%d,%d,%d,%d,%d]\n",
               node_set[i].dim[0], node_set[i].dim[1], node_set[i].dim[2],
               node_set[i].dim[3], node_set[i].dim[4],
               node_set[i].n_cuts[0], node_set[i].n_cuts[1], node_set[i].n_cuts[2],
               node_set[i].n_cuts[3], node_set[i].n_cuts[4]);

        printf("**rule_ids(%d) : ", node_set[i].node_rules);
#endif
    for (j = 0 ; j < node_set[i].node_rules; j++) {
        printf("%d ", node_set[i].rule_id[j]);
    }

    printf("\n");

    if (!node_set[i].empty_list) {
        printf("**ListIDs(%d) : ", node_set[i].list_length);

        for (j = 0; j < node_set[i].list_length; j++) {
            printf("%d ", node_set[i].rule_list[j]);
        }

        printf("\n");
    }
}

void print_tree_stat(struct xf_tree* tree) {
    struct xf_node* node_set;
    int i;
    node_set = tree->node_set;
    printf("*************************\n");
    printf("Tree Nodes = %d\n", tree->num_nodes);

    for (i = 1; i <= tree->num_nodes; i++) {
        if (node_set[i].is_leaf) {
            printf("Leaf ");
        } else {
            printf("Non-Leaf ");
        }

        printf("Node[%d]\n", i);
#if DEBUG >= 2
        print_node_stat(tree, i, 1);
#endif
    }
}

void xf_partition_rules(struct xf_tree* tree) {
    struct xf_rule* rule = tree->rule;
    struct xf_node* node_set = tree->node_set;
    int i, j, k;
    int cover = 0;
    int parent_id, child_id;

    for (j = 1; j <= tree->num_nodes; j++) {         /* all nodes */
        if (node_set[j].is_leaf == 1) {  /* iterate over only leaf nodes */
            cover = 0;

            for (i = 0; i < node_set[j].node_rules; i++) {  /* all rules in node */
                /* check if there is any rule that can be pushed up */
                cover = 1;

                for (k = 0; k < MAXDIMS; k++) {
                    if (rule[node_set[j].rule_id[i]].field[k].low > node_set[j].field[k].low ||
                            rule[node_set[j].rule_id[i]].field[k].high < node_set[j].field[k].high) {
                        cover = 0;
                        break;
                    }
                }

                if (cover == 1) {
                    parent_id = node_set[j].rule_id[i];
                    child_id = tree->num_rules;
                    tree->rule = (struct xf_rule*)realloc(tree->rule, (tree->num_rules + 1) * sizeof(struct xf_rule));
                    rule = tree->rule;

                    for (k = 0; k < MAXDIMS; k++) {
                        rule[child_id].field[k].low = node_set[j].field[k].low;
                        rule[child_id].field[k].high = node_set[j].field[k].high;
                    }

                    rule[child_id].derived_from = parent_id;
                    rule[child_id].priority = rule[parent_id].priority;
                    rule[child_id].in_port = rule[parent_id].in_port;             /*Ingress port*/
                    for(k=0;k<rule[parent_id].n_actions;k++) {
                        snprintf(rule[child_id].action[k], sizeof(rule[child_id].action[k]), "%s", rule[parent_id].action[k]);
                        rule[child_id].interface[k] = rule[parent_id].interface[k];
                    }
                    rule[child_id].num_derived_rules = 0;
                    rule[parent_id].derived_rules = (int*)realloc(rule[parent_id].derived_rules, (rule[parent_id].num_derived_rules + 1) * sizeof(int));
                    rule[parent_id].derived_rules[rule[parent_id].num_derived_rules] = child_id;
                    rule[parent_id].num_derived_rules++;
                    /* replace parent rule id with child rule id */
                    node_set[j].rule_id[i] = tree->num_rules;
                    tree->num_rules++;
                }
            }
        }
    }

#if DEBUG >= 3

    for (i = 0; i < tree->num_rules; i++) {
        printf("id %d -> ", i);

        for (k = 0; k < MAXDIMS; k++) {
            printf("%u : %u ", rule[i].field[k].low, rule[i].field[k].high);  /* all */
        }

        printf("priority %d ", rule[i].priority);

        if (rule[i].derived_from) {
            printf("D_from %d ", rule[i].derived_from);
        }

        if (rule[i].num_derived_rules) {
            printf("D_rules ");

            for (k = 0; k < rule[i].num_derived_rules; k++) {
                printf("%d ", rule[i].derived_rules[k]);
            }
        }

        printf("\n");
    }

    print_tree_stat(tree);
#endif
}


int insertNode(struct xf_tree* tree) {
    FILE* fpn = fopen("acl1_10_new.txt", "r");
    struct xf_rule* new_rule;
    int num_new_rules = 0;
    int i = 0 , j = 0;
    int tmp;
    unsigned sip1, sip2, sip3, sip4, siplen;
    unsigned dip1, dip2, dip3, dip4, diplen;
    unsigned proto, protomask;
    char* s = (char*)calloc(300, sizeof(char));

    if (fpn == NULL) {
        printf("Can NOt open RUle file\n");
        exit(0);
    }

    while (fgets(s, 300, fpn) != NULL) {
        num_new_rules++;
    }

    rewind(fpn);
    new_rule = (struct xf_rule*)calloc(num_new_rules, sizeof(struct xf_rule));

    while (1) {
        if (i == num_new_rules) { /* all rules loaded */
            break;
        }

        new_rule[i].interface[0] = 0;
        j = fscanf(fpn, "@%u.%u.%u.%u/%u\t%u.%u.%u.%u/%u\t%u : %u\t%u : %u\t%x/%x\t%s : %hu\t%u\t\n",
                   &sip1, &sip2, &sip3, &sip4, &siplen, &dip1, &dip2, &dip3, &dip4, &diplen,
                   &new_rule[i].field[2].low, &new_rule[i].field[2].high, &new_rule[i].field[3].low,
                   &new_rule[i].field[3].high, &proto, &protomask, new_rule[i].in_port, new_rule[i].action, &new_rule[i].interface);  /*ingress port*/

        if (j < 16) {
            break;
        }

        if (siplen == 0) {
            new_rule[i].field[0].low = 0;
            new_rule[i].field[0].high = 0xFFFFFFFF;
        } else if (siplen > 0 && siplen <= 8) {
            tmp = sip1 << 24;
            new_rule[i].field[0].low = tmp;
            new_rule[i].field[0].high = new_rule[i].field[0].low + (1 << (32 - siplen)) - 1;
        } else if (siplen > 8 && siplen <= 16) {
            tmp = sip1 << 24;
            tmp += sip2 << 16;
            new_rule[i].field[0].low = tmp;
            new_rule[i].field[0].high = new_rule[i].field[0].low + (1 << (32 - siplen)) - 1;
        } else if (siplen > 16 && siplen <= 24) {
            tmp = sip1 << 24;
            tmp += sip2 << 16;
            tmp += sip3 << 8;
            new_rule[i].field[0].low = tmp;
            new_rule[i].field[0].high = new_rule[i].field[0].low + (1 << (32 - siplen)) - 1;
        } else if (siplen > 24 && siplen <= 32) {
            tmp = sip1 << 24;
            tmp += sip2 << 16;
            tmp += sip3 << 8;
            tmp += sip4;
            new_rule[i].field[0].low = tmp;
            new_rule[i].field[0].high = new_rule[i].field[0].low + (1 << (32 - siplen)) - 1;
        } else {
            printf("Src IP length exceeds 32\n");
            return 0;
        }

        if (diplen == 0) {
            new_rule[i].field[1].low = 0;
            new_rule[i].field[1].high = 0xFFFFFFFF;
        } else if (diplen > 0 && diplen <= 8) {
            tmp = dip1 << 24;
            new_rule[i].field[1].low = tmp;
            new_rule[i].field[1].high = new_rule[i].field[1].low + (1 << (32 - diplen)) - 1;
        } else if (diplen > 8 && diplen <= 16) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            new_rule[i].field[1].low = tmp;
            new_rule[i].field[1].high = new_rule[i].field[1].low + (1 << (32 - diplen)) - 1;
        } else if (diplen > 16 && diplen <= 24) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            tmp += dip3 << 8;
            new_rule[i].field[1].low = tmp;
            new_rule[i].field[1].high = new_rule[i].field[1].low + (1 << (32 - diplen)) - 1;
        } else if (diplen > 24 && diplen <= 32) {
            tmp = dip1 << 24;
            tmp += dip2 << 16;
            tmp += dip3 << 8;
            tmp += dip4;
            new_rule[i].field[1].low = tmp;
            new_rule[i].field[1].high = new_rule[i].field[1].low + (1 << (32 - diplen)) - 1;
        } else {
            printf("Dest IP length exceeds 32\n");
            return 0;
        }

        if (protomask == 0xFF) {
            new_rule[i].field[4].low = proto;
            new_rule[i].field[4].high = proto;
        } else if (protomask == 0) {
            new_rule[i].field[4].low = 0;
            new_rule[i].field[4].high = 0xFF;
        } else {
            printf("Protocol mask error\n");
            return 0;
        }

        i++;  /* number of rules loaded in new_rule variable */
    }

    if (num_new_rules != i) {
        printf("The number of rules in file %d != number of rules loaded %d \n",
               num_new_rules, i);
        exit(0);
    } else {
        printf("The number of rules for dynamic update= %d\n", num_new_rules);
    }

    for (i = 0; i < num_new_rules; i++) {
        xf_update_rules(tree, &new_rule[i]);
    }

    fclose(fpn);
    free(new_rule);
    free(s);
    return 0;
}

int xf_update_rules(struct xf_tree* tree, struct xf_rule* add_rule) {
    struct xf_rule* rule = tree->rule;
    struct xf_node* node_set = tree->node_set;
/*
    if(tree->build_status != 1) {
        printf("returning null due to tree not build\n");
 	return 0;
    }
*/
    int i, cnode, k, retval = 0;
    int cover = 0;
    int isAdded = 0;
    for (cnode = 1; cnode <= tree->num_nodes; cnode++) {         /* all nodes */
        if (node_set[cnode].is_leaf == 1 || cnode == 1) {   /* iterate over only leaf nodes */
            cover = 1;

            for (k = 0; k < MAXDIMS; k++) {
                //printf("checking for field %d\n", k);
                if (add_rule->field[k].low > node_set[cnode].field[k].high ||
                        add_rule->field[k].high < node_set[cnode].field[k].low) {
                    cover = 0;
                    printf("rule does not fit at node %d due to field %d\n", cnode, k);
                    printf("boundaries of node= %d : %d and of rule= %d : %d\n",node_set[cnode].field[k].low,node_set[cnode].field[k].high,add_rule->field[k].low,add_rule->field[k].high);
                    break;
                }
            }
            if (cover == 1) {
                // if (node_set[cnode].node_rules < tree->bucket_size) {
                    if (!isAdded) 
                    {
                        tree->rule = (struct xf_rule*)realloc(tree->rule, (tree->num_rules + 1) * sizeof(struct xf_rule));
                        //printf("realloc() done\n");
                        rule = tree->rule;

                        for (i = 0; i < MAXDIMS; i++) {
                            rule[tree->num_rules].field[i].low = add_rule->field[i].low;
                            rule[tree->num_rules].field[i].high = add_rule->field[i].high;
                        }
                        //printf("rule fields assigned\n");
                        rule[tree->num_rules].rule_ptr = rule_from_xf_rule(add_rule);
                        rule[tree->num_rules].priority = tree->num_rules;
                        rule[tree->num_rules].derived_rules = NULL;
                        rule[tree->num_rules].num_derived_rules = 0;
                        rule[tree->num_rules].derived_from = 0;
                        rule[tree->num_rules].in_port = add_rule->in_port;      /*ingress port*/
                        for (i = 0; i < add_rule->n_actions; i++) {
                            strcpy(rule[tree->num_rules].action[i], add_rule->action[i]);
                            rule[tree->num_rules].interface[i] = add_rule->interface[i];
                        }
                        rule[tree->num_rules].n_actions = add_rule->n_actions;
                        tree->num_rules++;
                        isAdded = 1;
                    }

                    printf("node %d has %d rules\n", cnode, node_set[cnode].node_rules);
                    if (node_set[cnode].node_rules == 0)
                        node_set[cnode].rule_id = (int*)malloc(sizeof(int));
                    else
                        node_set[cnode].rule_id = (int*)realloc(node_set[cnode].rule_id, 
                                                                (node_set[cnode].node_rules + 1) * sizeof(int));
                    //printf("realloc() 2 done\n");
                    node_set[cnode].rule_id[node_set[cnode].node_rules] = tree->num_rules - 1;
                    node_set[cnode].node_rules++;
                    tree->num_stored_rules++;
                    //printf("rule added at node %d\n", retval);
                    retval = cnode;
//                } else {
//                    printf("Do something for bucket\n");
//                }
            }
        }
    }
    //print_tree_stat(tree);
/*
    printf("UPDATE DONE xf_rule @ %u\n",(unsigned int)add_rule);
    printf("UPDATE DONE RULE IN TREE @ %u\n", (unsigned int)&rule[tree->num_rules - 1]);
    printf("UPDATE DONE rule @ %u\n",(unsigned int)rule_from_xf_rule(add_rule));
*/
    return retval;
}


/* perform packet classification */
/* First searches non-leaf nodes --- Moved up rules and new child */
/* Then Leaf Nodes               --- Moved up rules and Leaf rules */
/* Then List of Moved Up rules */
struct xf_rule* xf_tree_lookup(struct xf_tree* tree, unsigned int* header, struct xf_lookup_match* match) {
    if(tree->num_rules == 0 || tree->build_status != 1) {
        printf("tree has %d rules\n",tree->num_rules);
        printf("returning null\n");
        match->rule_id = Null;
        match->node_id = Null;
        match->status = LOOKUP_MATCH_NONE;
 	return NULL;
    }
    //printf("tree has %d rules\n",tree->num_rules);
    struct xf_rule* rule = tree->rule;
    struct xf_node* node_set = tree->node_set;
    int idx[MAXDIMS];
    int cvalue[MAXDIMS];
    int cover, cchild;
    /*    int cuts[MAXDIMS]; */
    int cnode = 1;
    int is_matched = 0;
    int nbits[MAXDIMS];
    int i, j, k;
    int* cnodelist = NULL;   /* sohaib setting NULL in int* */
    int ncnode = 0;
    float temp_cost = 0.0;
    int accesses = 0;
    float temp_time = 0.0;
    struct timeval t1, t2;
    int min_priority = INT_MAX;
    match->rule_id = Null;
    match->node_id = Null;
    /* sohaib start timer */
    gettimeofday(&t1, NULL);

    for (i = 0; i < MAXDIMS; i++) {
        if (i == 4) {
            idx[i] = 8;
        } else if (i >= 2) {
            idx[i] = 16;
        } else {
            idx[i] = 32;
        }
    }

   /* 
        struct in_addr src_ip, dst_ip;
        src_ip.s_addr = header[0];
        dst_ip.s_addr = header[1];
        printf("lookup src_ip = %u\n", htonl(header[0]));
        printf("lookup dst_ip = %u\n", htonl(header[1]));
    
   
        printf("doing lookup for: \n");
        printf("%s\n", inet_ntoa(src_ip));
        printf("%s\n", inet_ntoa(dst_ip));
        printf("%u\n", header[2]);
        printf("%u\n", header[3]);
        printf("%u\n", header[4]);
    
    
        printf("tree num_stored_rules = %d\n", tree->num_stored_rules);
        printf("tree num_nodes = %d\n", tree->num_nodes);
        printf("cnode = %d\n", cnode);
   */ 
    /* Iterate over all non-leaf nodes first */
    while (node_set[cnode].is_leaf != 1) {
        //printf("%d is non-leaf\n",cnode);
        tree->cost += NODESIZE;
        tree->num_accesses++;
        temp_cost += NODESIZE;
        accesses++;
        
        /* Check if current node has any moved up rules */
        /* List of non-leaf nodes with moved up rules*/
        if (node_set[cnode].empty_list == 0) {
            cnodelist = (int*)realloc(cnodelist, (ncnode + 1) * sizeof(int));
            cnodelist[ncnode] = cnode;
            ncnode++;
        }
        //printf("not empty list\n");
        /* lookup in moved up rules of non leaf cnode */
        if (node_set[cnode].empty_list == 0) {
            /* Iterate over all moved up rules at current_node */
	    //printf("Iterating over all moved up rules at current_node\n");
            for (i = 0; i < node_set[cnode].list_length; i++) {
                tree->cost += RULEPTSIZE + RULESIZE;
                tree->num_accesses++;
                temp_cost += RULEPTSIZE + RULESIZE;
                accesses++;
                cover = 1;

                for (k = 0; k < MAXDIMS; k++) {
                    /* check if header matches any rule at current node */
                    if (rule[node_set[cnode].rule_list[i]].field[k].low > header[k] ||
                            rule[node_set[cnode].rule_list[i]].field[k].high < header[k]) {
                        cover = 0;
#if LOOKUP_DEBUG >= 2
			printf("NON leaf nodes num %d field %d not matched\n",cnode,k);
#endif
                        break;
                    }
                }
                if(rule[node_set[cnode].rule_list[i]].in_port != header[5]) {        /*ingress port*/
                    cover = 0;
#if LOOKUP_DEBUG >= 2
                    printf("NON leaf nodes num %d field 5 not matched\n",cnode);
#endif
                }
                /* cover = 1 means an internal matching rule was found at the current node  */
                if (cover == 1) {
#if LOOKUP_DEBUG >= 1 
                    printf("(1)Node[%d].Rule[%d] is matched internally\n",
                           cnode, node_set[cnode].rule_list[i]);
#endif

                    /* minimum matching rule */
                    if (min_priority >= rule[node_set[cnode].rule_list[i]].priority) {
                        min_priority = rule[node_set[cnode].rule_list[i]].priority;
                        match->rule_id = node_set[cnode].rule_list[i];
                        match->node_id = cnode;
                    }

                    is_matched = 1;
                    /* dont break here as there can be low priority rules in this node */
                    /* break; */
                }
            }
        }
        for (i = 0; i < MAXDIMS; i++) {
            nbits[i] = 0;
        }

        /* iterate over all dimensions */
        for (i = 0; i < MAXDIMS; i++) {
            /* check if this dimension was cut at current node */
            if (node_set[cnode].dim[i] == 1) {
                /*                cuts[i] = node_set[cnode].n_cuts[i];*/  /* n_cuts in i'th dim of cnode (max 2) */
                /*                while(cuts[i] != 1) {     */  /* while division by 2 is possible */
                /*                    nbits[i]++; */
                /*                    cuts[i] /= 2; */
                /*                } */
                nbits[i] = (int)((log(node_set[cnode].n_cuts[i]) / log(2)));  /* bits to match */
                cvalue[i] = 0;

                /* idx = number of bits in field. 8, 16 or 32
                 * idx[0, 1]=32 idx[2,3]=16,idx[4]=8
                 * this for loop iterates over the number of cuts
                 */
                for (k = idx[i]; k > idx[i] - nbits[i]; k--) {  /* one time.. match.. if nbits=0 loop condition false 16>16 */
                    /* if((header[i] & 1<<(k-1)) != 0) { */
                    if ((header[i] & (int)pow(2, k - 1)) != 0) {  /* if true than child 1 else child 0 */
                        cvalue[i] += (int)pow(2, k - idx[i] + nbits[i] - 1);  /* 2^0=1 .. for one loop k=idx and nbit=1 so ans is 0 */
                    }
                }
            } else {
                cvalue[i] = 0;
            }
        }

#if LOOKUP_DEBUG >= 3
        printf("idx [%d %d %d %d %d]\n", idx[0], idx[1], idx[2],
               idx[3], idx[4]);
        printf("nbits [%d %d %d %d %d]\n", nbits[0], nbits[1], nbits[2],
               nbits[3], nbits[4]);
        printf("cvalu [%d %d %d %d %d]\n", cvalue[0], cvalue[1], cvalue[2],
               cvalue[3], cvalue[4]);
#endif
        cchild = 0;

        /* 0*=1,2,3,4 1*=2,3,4 2*=3,4 3*=4 4=4 */
        for (i = 0; i < MAXDIMS; i++) {  /* cvalue 0 to 4 */
            for (k = i + 1; k < MAXDIMS; k++) {  /* n_cuts dims i+1 < 5 */
                cvalue[i] *= node_set[cnode].n_cuts[k];  /* if n_cuts=1 than this node has child */
            }

            cchild += cvalue[i];
        }

        /* first for loop i< MAXDIMS-1; to i< MAXDIMS
         cchild += cvalue[MAXDIMS-1]; -- cvalue of 4 */
#if LOOKUP_DEBUG >= 3
        printf("n_cuts [%d %d %d %d %d]\n", node_set[cnode].n_cuts[0],
               node_set[cnode].n_cuts[1], node_set[cnode].n_cuts[2],
               node_set[cnode].n_cuts[3], node_set[cnode].n_cuts[4]);
        printf("cvalu [%d %d %d %d %d]\n", cvalue[0], cvalue[1], cvalue[2],
               cvalue[3], cvalue[4]);
#endif
#if LOOKUP_DEBUG >= 2
        printf("Node[%d].child[%d]=%d\n", cnode, cchild, node_set[cnode].child[cchild]);
#endif
        cnode = node_set[cnode].child[cchild];

        if (cnode == Null) {
            break;
        }

        for (i = 0; i < MAXDIMS; i++) {
            idx[i] -= nbits[i];  /* decrement the index for next round by nbits */
        }
    }

    /* all non-leaf nodes have been searched - now do lookup on leaf nodes */
    if (cnode != Null) {
        if (node_set[cnode].empty_list == 0) {  /* lookup in moved up rules of cnode */
	   // printf("lookup in moved up rules of cnode\n");
            for (i = 0; i < node_set[cnode].list_length; i++) {
                tree->cost += RULEPTSIZE + RULESIZE;
                tree->num_accesses++;
                temp_cost += RULEPTSIZE + RULESIZE;
                accesses++;
                cover = 1;

                for (k = 0; k < MAXDIMS; k++) {
                    if (rule[node_set[cnode].rule_list[i]].field[k].low > header[k] ||
                            rule[node_set[cnode].rule_list[i]].field[k].high < header[k]) {
                        cover = 0;
#if LOOKUP_DEBUG >= 2
			printf("leaf nodes num %d field %d not matched\n",cnode,k);
#endif
                        break;
                    }
                }
                if(rule[node_set[cnode].rule_list[i]].in_port != header[5]) {
                    cover = 0;
#if LOOKUP_DEBUG >= 2
                    printf("leaf nodes num %d field 5 not matched\n",cnode);
#endif
                }

                if (cover == 1) {
#if LOOKUP_DEBUG >= 1
                    printf("(2.1)Node[%d].Rule[%d] is matched internally\n",
                           cnode, node_set[cnode].rule_list[i]);
#endif

                    if (min_priority >= rule[node_set[cnode].rule_list[i]].priority) {
                        min_priority = rule[node_set[cnode].rule_list[i]].priority;
                        match->rule_id = node_set[cnode].rule_list[i];
                        match->node_id = cnode;
                    }
		    //printf("Matched with rule id = %d\n",match->rule_id);
                    is_matched = 1;
                    /* break; */
                }
            }
        }

        for (i = 0; i < node_set[cnode].node_rules; i++) {   /* lookup in all rules of cnode */
            tree->cost += RULEPTSIZE + RULESIZE;
            tree->num_accesses++;
            temp_cost += RULEPTSIZE + RULESIZE;
            accesses++;
            cover = 1;

            for (k = 0; k < MAXDIMS; k++) {
                if (rule[node_set[cnode].rule_id[i]].field[k].low > header[k] ||
                        rule[node_set[cnode].rule_id[i]].field[k].high < header[k]) {
                    cover = 0;
#if LOOKUP_DEBUG >= 2
		    printf("leaf nodes num %d rule id %d field %d not matched\n",cnode,i,k);
#endif		
                    break;
                }
            }
            if(rule[node_set[cnode].rule_id[i]].in_port != header[5]) {     /*ingress port*/
                    cover = 0;
#if LOOKUP_DEBUG >= 2
                    printf("leaf nodes num %d field 5 not matched\n",cnode);
#endif
            }

            if (cover == 1) {
#if LOOKUP_DEBUG >= 1
                printf("(2.2)Node[%d].Rule[%d] is matched in leaf\n",
                       cnode, node_set[cnode].rule_id[i]);
#endif

                if (min_priority >= rule[node_set[cnode].rule_id[i]].priority) {
                    min_priority = rule[node_set[cnode].rule_id[i]].priority;
                    match->rule_id = node_set[cnode].rule_id[i];
                    match->node_id = cnode;
                }

		//printf("Matched with rule id = %d",match->rule_id);
                is_matched = 1;
                /* break; */
            }
        }
    }

    /* ncnode = number of non-leaf nodes that have non-empty rule lists */
    if (ncnode != 0) {
        for (j = ncnode - 1; j >= 0; j--) {
            for (i = 0; i < node_set[cnodelist[j]].list_length; i++) {
#if LOOKUP_DEBUG >= 1
                printf("node_set[cnodelist[j=%d]=%d].rule_list[i]=%d,min_priority=%d\n",
                       j, cnodelist[j], node_set[cnodelist[j]].rule_list[i], min_priority);
#endif

                if (node_set[cnodelist[j]].rule_list[i] >= min_priority) {  /* rule_id is greater than min_priority break; */
                    tree->cost += RULEPTSIZE;
                    tree->num_accesses++;
                    temp_cost += RULEPTSIZE;
                    accesses++;
                    break;
                } else {
                    tree->cost += RULEPTSIZE + RULESIZE;
                    tree->num_accesses++;
                    temp_cost += RULEPTSIZE + RULESIZE;
                    accesses++;
                }

                cover = 1;

                for (k = 0; k < MAXDIMS; k++) {
                    if (rule[node_set[cnodelist[j]].rule_list[i]].field[k].low > header[k] ||
                            rule[node_set[cnodelist[j]].rule_list[i]].field[k].high < header[k]) {
                        cover = 0;
                        break;
                    }
                }
                if(rule[node_set[cnodelist[j]].rule_list[i]].in_port != header[5]) {
                    cover = 0;
                }

                if (cover == 1) {
#if LOOKUP_DEBUG >= 1
                    printf("(3)Node[%d].Rule[%d] is matched internally\n",
                           cnode, node_set[cnode].rule_list[i]);
#endif

                    /* if(minID > node_set[cnode].rule_list[i]) */
                    /*   minID = node_set[cnode].rule_list[i]; */
                    if (min_priority >= rule[node_set[cnodelist[j]].rule_list[i]].priority) {
                        min_priority = rule[node_set[cnodelist[j]].rule_list[i]].priority;
                        match->rule_id = node_set[cnodelist[j]].rule_list[i];
                        match->node_id = cnodelist[j];
                    }

                    is_matched = 1;
                    /* break; */
                }
            }
        }
    }

    /* stop timer */
    gettimeofday(&t2, NULL);
    /* calculate time difference */
    temp_time = ((t2.tv_sec - t1.tv_sec) * 1000.0 * 1000.0) + (t2.tv_usec - t1.tv_usec);
    tree->lookup_time += temp_time;
#if LOOKUP_DEBUG >= 3
    printf("Words=%.2f,Accesses=%d,Time=%.2f\n", temp_cost, accesses, temp_time);
#endif

    if (tree->worstcost < temp_cost) {
        tree->worstcost = temp_cost;
    }

    if (tree->worstaccesses < accesses) {
        tree->worstaccesses = accesses;
    }

    if (tree->worsttime < temp_time) {
        tree->worsttime = temp_time;
    }

    if (tree->bestcost > temp_cost) {
        tree->bestcost = temp_cost;
    }

    if (tree->bestaccesses > accesses) {
        tree->bestaccesses = accesses;
    }

    if (tree->besttime > temp_time) {
        tree->besttime = temp_time;
    }

    free(cnodelist);
    tree->num_packets++;

    if (is_matched == 1) {
        if (match->rule_id + 1 == match->fid) {
            tree->matched_packets++;
            match->status = LOOKUP_MATCH_EQUAL;
        } else if (match->rule_id + 1 > match->fid) {
            match->status = LOOKUP_MATCH_LOW;
        } else {
            tree->matched_packets++;
            match->status = LOOKUP_MATCH_HIGH;
        }

        return &rule[match->rule_id];
    } else {
        match->rule_id = Null;
        match->node_id = Null;
        match->status = LOOKUP_MATCH_NONE;
        return NULL;
    }
}

void tracesLookup(struct xf_tree* tree) {
    unsigned header[MAXDIMS];
    struct xf_lookup_match match;

    if (tree->fpt != NULL) {
        while (1) {
            if (fscanf(tree->fpt, "%u\t%u\t%u\t%u\t%u\t%d\n",
                       &header[0], &header[1], &header[2], &header[3], &header[4],
                       &(match.fid)) != 6) {
                break;
            }

            /* match.fid++; */
#if LOOKUP_DEBUG >= 1
            printf("Packet %lu (%d) [%u %u %u %u %u]\n", tree->num_packets, match.fid, header[0], header[1],
                   header[2], header[3], header[4]);
#endif
            xf_tree_lookup(tree, header, &match);

            if (match.status == LOOKUP_MATCH_NONE) {
                printf("? packet %lu match NO rule, should be %d\n", tree->num_packets, match.fid);
            } else if (match.status == LOOKUP_MATCH_EQUAL) {
#if LOOKUP_DEBUG >= 1
                printf("**packet %lu match rule %d in node %d\n", tree->num_packets, match.rule_id + 1, match.node_id);
#endif
            } else if (match.status == LOOKUP_MATCH_LOW) {
                printf("? packet %lu match lower priority rule %d, should be %d\n", tree->num_packets, match.rule_id + 1, match.fid);
            } else {
                printf("! packet %lu match higher priority rule %d, should be %d\n", tree->num_packets, match.rule_id + 1, match.fid);
            }
        }

        print_lookup_stats(tree);
    } else {
        printf("No packet trace input\n");
    }
}

void print_lookup_stats(struct xf_tree* tree) {
    printf("*************************\n");
    printf("# of packets misclassified = %lu/%lu\n", (tree->num_packets - tree->matched_packets), tree->num_packets);
    printf("# of word accessed/pkt = %.2f [%.0f - %.0f]\n", tree->cost * 4.0 / tree->num_packets, tree->bestcost * 4.0, tree->worstcost * 4.0);
    printf("# of Memory accesses/pkt = %lu [%d - %d] \n", tree->num_accesses / tree->num_packets, tree->bestaccesses, tree->worstaccesses);
    printf("Lookup Time/pkt (Micro sec)= %.2f [%.0f - %.0f]\n", tree->lookup_time / tree->num_packets, tree->besttime, tree->worsttime);
}

void xf_tree_destroy(struct xf_tree* tree) {
    int i;
    struct xf_node* node_set;
    node_set = tree->node_set;

    for (i = 1; i <= tree->num_nodes; i++) {
        free(node_set[i].child);
        free(node_set[i].rule_id);

        if (!node_set[i].empty_list) {
            free(node_set[i].rule_list);
        }
    }

    for (i = 0; i < tree->num_rules; i++) {
        if (tree->rule[i].num_derived_rules > 0) {
            free(tree->rule[i].derived_rules);
        }
    }

    free(node_set);
    free(tree->rule);  /* free rule array */
    fclose(tree->fpr);
    fclose(tree->fpt);
}
