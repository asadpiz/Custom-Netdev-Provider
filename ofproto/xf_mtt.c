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
 * Implementation of MTT for NOVA1 prototype
 * This contain three type of functions
 * xf_mtt_* are MTT functions.
 * xf_node_* are node related functions.
 * xf_mtt_list_* are linked list related functions
 */

#include "xf_mtt.h"

/* Global variable for ACL functions return value */
int retval;
static int xf_num_tcam_entries = 1;

int find_empty_slot(void) {
    int i;

    for (i = 0; i < MTT_SIZE; i++) {
        if (xf_mtt_slots[i] == 0) {
            return i;
        }
    }

    return 0;
}

/**
 *@brief              Initilize the mtt
 *
 *@return             0 for success, other means fail.
 */
int xf_mtt_init(void) {
    xf_mtt_handle = NULL;  /* pointer to Linked List head */
    xf_mtt_num_rules = 1;
    /* MTT_Slots Initilization */
    memset(xf_mtt_slots, 0, MTT_SIZE);
    it = pthread_create(&insert_thread, NULL, xf_sync_tcam, NULL);
    if (it) {
        printf("ERROR; return code from pthread_create(xf_sync_tcam) is %d\n", it);
        exit(-1);
    }

    return 0;
}

/**
 *@brief              Insert the node in TCAM
 *
 *@param  node        node to insert
 *
 *@return             1 for success, other means fail.
*/
int xf_mtt_insert(struct xf_tree* tree, int vtt_node_id) {
#if DEBUG >= 2
    printf("trying to insert node id %d with %d rules in MTT\n",vtt_node_id,tree->node_set[vtt_node_id].node_rules);
#endif
    struct timeval t1;
    int rule_id, x, temp;
    struct xf_acl_rule acl_rule;
    int i = 0, leaf_rules = 0;
    /* uint32_t *rule_ids=NULL;   */
    struct xf_mtt_node* node = (struct xf_mtt_node*)malloc(sizeof(struct xf_mtt_node));
    struct xf_node* node_set = tree->node_set;
    struct xf_rule* node_rules = tree->rule;
    struct xf_rule* rule;
    struct rule* rule_temp;
    struct xf_rule temp_xf_rule;

    if (xf_mtt_list_elem(&xf_mtt_handle, vtt_node_id)) 
    {
        // printf("vtt_node_id %d already exist in mtt\n", vtt_node_id);
        return 0;
    }
 
    else 
    {
        node->vtt_node_id = vtt_node_id;
        node->nrules = 0;
        node->tcam_handle = NULL;
        
        // printf("trying to insert node id %d with %d rules in MTT\n",vtt_node_id,tree->node_set[vtt_node_id].node_rules);
        /* xf_node_rules(node_set,vtt_node_id,rule_ids);  */

        if (node_set != NULL) 
        {
            if (node_set[vtt_node_id].is_leaf) 
            {
                node->nrules = node_set[vtt_node_id].node_rules;
                // node->node_rules = (struct xf_rule *)malloc(node->nrules * sizeof(struct xf_rule));
                node->tree = tree;
                
                while (xf_mtt_num_rules + node->nrules > MTT_SIZE) 
                {
                    // printf("need to evict as MTT has %d rules\n", xf_mtt_num_rules);

                    pthread_mutex_lock(&mtt_lock); // get MTT lock for setting remove flag
                    if ((retval = xf_mtt_list_evict_node(&xf_mtt_handle)) < 0)
                    {
                        // printf("evict_node returned < 0....breaking mtt_insert \n");
                        pthread_mutex_unlock(&mtt_lock); // release MTT lock
                        return -1;
                    }

                    temp = retval;
                    if ((retval = xf_mtt_delete(temp)) != 0)
                    {
                        // printf("mtt_delete returned an error....breaking mtt_insert \n");
                        pthread_mutex_unlock(&mtt_lock); // release MTT lock
                        return -1;
                    }
                    pthread_mutex_unlock(&mtt_lock); // release MTT lock
                    // printf("node %d deleted, xf_mtt_num_rules = %d\n", temp, xf_mtt_num_rules);
#if DEBUG >= 1
                    printf("xf_mtt_num_rules %d node->nrules %d MTT_SIZE %d\n",xf_mtt_num_rules, node->nrules,MTT_SIZE);
                    printf("Node %d is force Evicted\n", temp);
#endif
                }

                // printf("NOW about to insert in MTT, evictions done\n");
                node->tcam_handle = (uint32_t*)malloc(sizeof(uint32_t) * node->nrules);
/*
                for (i = 0 ; i < node_set[vtt_node_id].node_rules; i++) 
                {
                    node->tcam_handle[i] = find_empty_slot();
                    printf("mtt insert flag set, for slot: %d\n", node->tcam_handle[i]);
                    xf_mtt_slots[node->tcam_handle[i]] = 1;
                }
*/
            }
/*
#ifdef EMPTY_LIST
            if (!node_set[vtt_node_id].empty_list) 
            {
                printf("Node has pushed up rules\n");
                leaf_rules = node->nrules;
                node->nrules += node_set[vtt_node_id].list_length;
                node->tcam_handle = (uint32_t*)realloc(node->tcam_handle, node->nrules * sizeof(uint32_t));
                node->tree = tree;

                for (i = leaf_rules; i < (node_set[vtt_node_id].list_length + leaf_rules); i++) 
                {
                    rule = &(node_rules[node_set[vtt_node_id].rule_id[i]]);
                    node->tcam_handle[i] = find_empty_slot();
                    xf_mtt_slots[node->tcam_handle[i]] = 1;
                }
            }
#endif
*/

            node->stats.bytes = 0;
            node->stats.packets = 0;
            gettimeofday(&node->stats.last_updated, NULL);
        }
        pthread_mutex_lock(&mtt_lock);
        xf_mtt_list_push_last(&xf_mtt_handle, node);
        xf_mtt_num_rules += node->nrules;
        pthread_mutex_unlock(&mtt_lock);
#if DEBUG >= 1
        printf("vtt_node_id %d added in mtt, now mtt_num_rules = %d\n",vtt_node_id, xf_mtt_num_rules); 
#endif
    }
}

/**
 *@brief              Reads a node from TCAM
 *
 *@param  vtt_node_id vtt_node_id for reading a node
 *
 *@return             1 for success, other means fail.
*/
int xf_mtt_read(int vtt_node_id) 
{
    struct xf_mtt_node* node;

    if ((node = xf_mtt_list_elem(&xf_mtt_handle, vtt_node_id)) != NULL) {
        // printf("MTT_Node with vtt_node_id %d Found\n", vtt_node_id);
        xf_node_print(node);
        return 1;
    }
 
    else 
    {
        //printf("Can not read element with vtt_node_id %d from mtt\n", vtt_node_id);
        return 0;
    }
}

/**
 *@brief               Deletes a node from TCAM
 *
 *@param  vtt_node_id: node_id for deleting a node
 *
 *@return              0 for success, other means fail.
*/
int xf_mtt_delete(int vtt_node_id) 
{
    struct xf_mtt_node* node;
    int i;

    if ((node = xf_mtt_list_elem(&xf_mtt_handle, vtt_node_id))) 
    {
        if (node->flag_insert == 1 || node->flag_remove == 1) 
        {
            // printf("this node has a flag set so returning\n", vtt_node_id);
            return -1;
        }

        for (i = 0 ; i < node->nrules; i++) 
        {
            // xf_hal_delete(node->tcam_handle[i]); 
            // xf_delete_acl_rule(&(node->tcam_handle[i]));
            // printf("mtt remove flag set, for slot: %d\n", node->tcam_handle[i]);
            xf_mtt_slots[node->tcam_handle[i]] = 0;
        }
        node->flag_remove = 1;
        node->flag_insert = 0;

        // xf_apply_acl();
        xf_mtt_num_rules -= node->nrules;
    }
    // xf_mtt_print();
    return 0;
}

/**
 *@brief              checks if the node is present in MTT
 *
 *@param  vtt_node_id          vtt_node_id for reading a node
 *
 *@return             0 for not present in MTT, 1 for present in MTT.
*/
int xf_mtt_ispresent(int vtt_node_id) {
    return xf_mtt_list_elem(&xf_mtt_handle, vtt_node_id) != NULL ? 1 : 0;
}

/**
 *@brief              Return current no of nodes in MTT
 *
 *@return             length of MTT
*/
int xf_mtt_length(void) {
    return xf_mtt_list_len(xf_mtt_handle);
}

/**
 *@brief              Prints all nodes of MTT
 *
 *@return             0 for success, other means fail.
*/
int xf_mtt_print(void) {
    printf("==========MTT PRINT==========\n");
    xf_mtt_list_print(&xf_mtt_handle);
    return 0;
}

/**
 *@brief              Exit the mtt and save all the nodes in TCAM
 *
 *@return             0 for success, other means fail.
*/
int xf_mtt_exit(void) {
    xf_mtt_list_clear(&xf_mtt_handle);
    return 0;
}


/**
 *@brief              Prints a node
 *
 *@param  node        node to print
 *
 *@return             0 for success, other means fail.
*/
int xf_node_print(struct xf_mtt_node* node) {
    int i = 0;
    struct timeval last_updated;
    float time_diff = 0.0;

    if (node != NULL) {
        gettimeofday(&last_updated, NULL);
        time_diff = TIME_DIFF(node->stats.last_updated, last_updated);
        printf("Node : %u nrules : %u pkts/bytes %u/%u Updated %.02f ",
               node->vtt_node_id, node->nrules,
               node->stats.packets, node->stats.bytes, time_diff);
        printf("TCAM handles ");

        for (i = 0; i < node->nrules; i++) {
            printf("%u ", node->tcam_handle[i]);
        }
        printf("insert flag = %d\n", node->flag_insert);
        printf("remove flag = %d\n", node->flag_remove);
        printf("\n");
    }

    return 0;
}

/**
 *@brief              Push a node in the linked list
 *
 *@param  mtt_handle   Pointer to linked list
 *@param  data        node to be pushed in linked list
 *
 *@return             0 for success, other means fail.
*/
int xf_mtt_list_push(struct xf_mtt_list_node** mtt_handle,
                     struct xf_mtt_node* data) {
    struct xf_mtt_list_node* node_new =
        (struct xf_mtt_list_node*)malloc(sizeof(struct xf_mtt_list_node));
    node_new->data = data;
    node_new->next = *mtt_handle;
    *mtt_handle = node_new;
    node_new->data->flag_insert = 1;
    node_new->data->flag_remove = 0;
    return 0;
}

/**
 *@brief              Add element on the end of linked list
 *
 *@param  mtt_handle   Pointer to linked list
 *@param  data        node to be inserted in linked list
 *
 *@return             0 for success, other means fail.
*/
int xf_mtt_list_push_last(struct xf_mtt_list_node** mtt_handle,
                          struct xf_mtt_node* data) {
    struct xf_mtt_list_node* node_curr = *mtt_handle;

    if (!node_curr) {
        xf_mtt_list_push(mtt_handle, data);
    } else  {
        /* find the last node */
        while (node_curr -> next) {
            node_curr = node_curr -> next;
        }

        /* build the node after it */
        xf_mtt_list_push(&(node_curr -> next), data);
    }

    return 0;
}

/**
 *@brief              Get an element from the list
 *
 *@param  mtt_handle   Pointer to linked list
 *
 *@return             node from linked list
*/
struct xf_mtt_node* xf_mtt_list_pop(struct xf_mtt_list_node** mtt_handle) {
    struct xf_mtt_list_node* node_togo = *mtt_handle;
    struct xf_mtt_node* data = NULL;

    if (mtt_handle)  {
        data = node_togo -> data;
        *mtt_handle = node_togo -> next;
        free(node_togo->data->tcam_handle);
        free(node_togo->data);
        free(node_togo);
    }

    return data;
}

/**
 *@brief              Deletes a node in the linked list
 *
 *@param  mtt_handle   Pointer to linked list
 *@param  data        node to be Deleted in linked list
 *
 *@return             1 for Deleted, 0 means not deleted.
*/
int xf_mtt_list_delete(struct xf_mtt_list_node** mtt_handle,
                       int vtt_node_id) {
    struct xf_mtt_list_node* node_curr_prev = *mtt_handle;
    struct xf_mtt_list_node* node_curr = node_curr_prev->next;

    if (node_curr_prev -> data->vtt_node_id == vtt_node_id) {  /* compare */
        xf_mtt_list_pop(mtt_handle);  /* pop the Linked List's head */
        return 1;
    }

    while (node_curr) {
        if (node_curr -> data->vtt_node_id == vtt_node_id) {  /* compare */
            node_curr_prev->next = node_curr->next;
            free(node_curr->data->tcam_handle);
            // free(node_curr->data->node_rules);
            free(node_curr->data);
            free(node_curr);
           
            return 1;
        } else {
            node_curr = node_curr -> next;
            node_curr_prev = node_curr_prev -> next;
        }
    }

    return 0;
}

/**
 *@brief              Find if node is present
 *
 *@param  mtt_handle   Pointer to linked list
 *@param  data        node to find from linked list
 *
 *@return             matching node if node found NULL in case of failure.
*/
struct xf_mtt_node* xf_mtt_list_elem(struct xf_mtt_list_node** mtt_handle,
                                     int vtt_node_id) {
    struct xf_mtt_list_node* node_curr = *mtt_handle;

    while (node_curr) {
        if (node_curr->data->vtt_node_id == vtt_node_id) {  /* compare */
            return node_curr->data;
        } else {
            node_curr = node_curr -> next;
        }
    }

    return NULL;
}

/**
 *@brief              Retrun the length of linked list
 *
 *@param  mtt_handle   Pointer to linked list
 *
 *@return             Length of the linked list. Currently stored nodes
*/
int xf_mtt_list_len(struct xf_mtt_list_node* mtt_handle) {
    struct xf_mtt_list_node* curr = mtt_handle;
    int len = 0;

    while (curr) {
        ++len;
        curr = curr -> next;
    }

    return len;
}

/**
 *@brief              Retrun the length of linked list
 *
 *@param  mtt_handle   Pointer to linked list
 *
 *@return             Length of the linked list. Currently stored nodes
*/
int xf_mtt_rules_count(struct xf_mtt_list_node* mtt_handle) {
    struct xf_mtt_list_node* curr = mtt_handle;
    int count = 0;

    while (curr) {
        count += curr->data->nrules;
        curr = curr -> next;
    }

    return count;
}


/**
 *@brief              Print the currently stored nodes
 *
 *@param  mtt_handle   Pointer to linked list
 *
 *@return             0 for success, other means fail.
*/
int xf_mtt_list_print(struct xf_mtt_list_node** mtt_handle) {
    struct xf_mtt_list_node* node_curr = *mtt_handle;

    if (!node_curr) {
        puts("MTT is empty");
    } else  {
        while (node_curr) {
            xf_node_print(node_curr -> data);  /* print node */
            node_curr = node_curr -> next;
        }
    }

    return 0;
}

/**
 *@brief              delete all nodes stored
 *
 *@param  mtt_handle   Pointer to linked list
 *
 *@return             0 for success, other means fail.
*/
int xf_mtt_list_clear(struct xf_mtt_list_node** mtt_handle) {
    while (*mtt_handle) {
        xf_mtt_list_pop(mtt_handle);
    }

    return 0;
}


/**
 *@brief              return vtt_node_id for forced eviction
 *
 *@param  mtt_handle   Pointer to linked list
 *
 *@return             matching node if node found -1 in case of failure.
 */
int xf_mtt_list_evict_node(struct xf_mtt_list_node** mtt_handle) 
{
    struct xf_mtt_list_node* node_curr = *mtt_handle;
    uint32_t min_packets = 0;
    int vtt_node_id = node_curr->data->vtt_node_id;
    int i;
     
    if (!node_curr)
    {
        //printf("breaking from list_evict_node as mtt_handle is NULL\n");
        return -1;
    }
    
    while (node_curr) 
    {
        if (node_curr->data->flag_insert == 0 && node_curr->data->flag_remove == 0) 
        {
            if (node_curr->data->stats.packets < min_packets) 
            {
                min_packets = node_curr->data->stats.packets;
                vtt_node_id = node_curr->data->vtt_node_id;
            }
        }

        // num_nodes++;
        node_curr = node_curr->next;
    }

    // printf("selected node %d to evict\n", vtt_node_id);
    return vtt_node_id;
} 

/**
 *@brief             Syncs stats between MTT and switch ACL 
 *
 *@param             
 *
 *@return            0 for success, other means fail.
*/
int xf_mtt_sync_stats(struct xf_mtt_list_node *node) 
{
    // struct xf_mtt_list_node* node = xf_mtt_handle;
    /* struct xf_hal_rule_stats hal_stats; */
    fm_aclCounters counters;
    struct xf_mtt_node_stats mtt_stats;
    struct timeval last_updated;
    struct xf_mtt_list_node* next_node;
    float time_diff = 0.0;
    int i, x, vtt_node_id;
    int pkts;
    struct timeval t1;
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);
   
    //while (node) {
        // printf("updating stats of node %d\n",node->data->vtt_node_id); 
        next_node = NULL;
        vtt_node_id = 0;
        mtt_stats.packets = 0;
        mtt_stats.bytes = 0;

        for (i = 0 ; i < node->data->nrules; i++) 
        {
            /* xf_hal_sync_stats(node->data->tcam_handle[i], &hal_stats); */
            if ( (x = xf_get_acl_stats(&node->data->tcam_handle[i], &counters) ) < 0)
            {
                // printf("error in getting stats for node %d\n", node->data->tcam_handle[i]);
                break;
            }
            mtt_stats.packets += counters.cntPkts; /* hal_stats.packets; */
            mtt_stats.bytes += counters.cntOctets; /* hal_stats.bytes; */
            /* Random stats */
            //pkts = rand() % 10;
            //mtt_stats.packets += pkts;
            /* packet size 30 bytes to 60 bytes randomly */
            //mtt_stats.bytes += ((pkts * rand() % 30) + 30);
        }

        if (mtt_stats.packets > node->data->stats.packets) 
        {
            // printf("Node : %d stats updated\n", node->data->vtt_node_id); 
            gettimeofday(&node->data->stats.last_updated, NULL);
            node->data->stats.packets = mtt_stats.packets;
            node->data->stats.bytes = mtt_stats.bytes;
        } 
        else
        {
            gettimeofday(&last_updated, NULL);
            time_diff = TIME_DIFF(node->data->stats.last_updated, last_updated);
            /* printf("Node : %d Stats Not updated from %.02f \n",
                   node->data->vtt_node_id, time_diff); */
/*
            if (time_diff >= TIMEOUT) 
            {
                next_node = node->next;
                vtt_node_id = node->data->vtt_node_id;
                // TODO: figure out how to do eviction here
                // xf_mtt_delete(vtt_node_id);
                //printf("EVENT 5 : Node %d is Evicted from MTT and TCAM\n", vtt_node_id);
            }
*/
        }
/*
        if (vtt_node_id == 0) {
            node = node -> next;
        } else {
            node = next_node;
        }
*/
    //}

    return 0;
}



/**
 *@brief             Syncs MTT and switch TCAM
 *
 *@param             
 *
 *@return            0 for success, other means fail.
*/

int xf_mtt_tcam_sync() 
{
    struct xf_mtt_list_node* node = xf_mtt_handle;
    struct xf_acl_rule acl_rule;
    //struct xf_mtt_list_node* next_node;
    int i, rule_id, vtt_node_id, retval;
    struct xf_rule *rule;
    struct xf_tree* tree;
    struct timeval t1, t2;
    float sync_time = 0.0;
    int num_rules_inserted = 0;
    int num_rules_removed = 0;

    fm_int err, arule, nrule;
    fm_aclCondition acond, ncond;
    fm_aclActionExt aaction, naction;
    fm_aclParamExt aparam, nparam;
    fm_aclValue *avalue = (fm_aclValue *)malloc(sizeof *avalue);
    fm_aclValue *nvalue = (fm_aclValue *)malloc(sizeof *nvalue);


    // get MTT lock here

    pthread_mutex_lock(&mtt_lock);

    while (node)
    {
        // printf("sync MTT node %d with TCAM\n",node->data->vtt_node_id); 
        if (node->data->flag_remove == 1)
        {
            //printf("removing node %d from TCAM, num_mtt_rules = %d, TCAM entries = %d\n", 
            //        node->data->vtt_node_id, xf_mtt_num_rules, xf_num_tcam_entries);
            tree = node->data->tree;
            vtt_node_id = node->data->vtt_node_id;
            for (i = 0 ; i < node->data->nrules; i++)
            {
                rule_id = tree->node_set[vtt_node_id].rule_id[i];
                rule = &node->data->tree->rule[rule_id];
                if ((retval = xf_delete_acl_rule(&(node->data->tcam_handle[i]))) != 0)
                {
                    // printf("error in deleting rule %d\n", node->data->tcam_handle[i]);
                    pthread_mutex_unlock(&mtt_lock); 
                    return -1; 
                }
                xf_mtt_slots[node->data->tcam_handle[i]] = 0;
                num_rules_removed++;
                xf_num_tcam_entries--;
            }
            node->data->flag_remove = 0;
            xf_mtt_list_delete(&xf_mtt_handle, vtt_node_id);
            //printf("%d rules removed from TCAM, now TCAM entries = %d, MTT_NUM_RULES = %d\n", 
            //        num_rules_removed, xf_num_tcam_entries, xf_mtt_num_rules);
        }

        else if (node->data->flag_insert == 1)
        {
            //printf("inserting node %d with %d rules, num_mtt_rules = %d, TCAM entries = %d\n", 
            //        node->data->vtt_node_id, node->data->nrules, xf_mtt_num_rules, xf_num_tcam_entries);
            tree = node->data->tree;
            vtt_node_id = node->data->vtt_node_id;

            for (i = 0 ; i < node->data->nrules; i++)
            {
                rule_id = tree->node_set[vtt_node_id].rule_id[i];

                node->data->tcam_handle[i] = find_empty_slot();
                // printf("mtt insert flag set, for slot: %d\n", node->data->tcam_handle[i]);
                xf_mtt_slots[node->data->tcam_handle[i]] = 1;

                rule = &node->data->tree->rule[rule_id];
                //gettimeofday(&t1, NULL);
                if ((retval = xf_insert_rule(&acl_rule, rule, node->data->tcam_handle[i])) != 0)
                {
                    // printf("error in inserting rule %d\n", node->data->tcam_handle[i]);
                    printf("MTT num rules = %d, TCAM entries = %d\n", xf_mtt_num_rules, xf_num_tcam_entries);
                    pthread_mutex_unlock(&mtt_lock); 
                    return -1; 
                }
                num_rules_inserted++;
                xf_num_tcam_entries++;
            }
            //printf("%d rules inserted, TCAM entries = %d, MTT_NUM_RULES = %d\n", 
            //        num_rules_inserted, xf_num_tcam_entries, xf_mtt_num_rules);
            node->data->flag_insert = 0;            
        }
        node = node->next;
        if (num_rules_inserted + num_rules_removed > 500)
            break;
    }

    pthread_mutex_unlock(&mtt_lock);
    
    if (num_rules_inserted + num_rules_removed > 0)
        xf_apply_acl();

    printf("%d removals\n", num_rules_removed);
    printf("%d insertions\n", num_rules_inserted);
    node = xf_mtt_handle;
    while (node)
    {
        if (node->data->flag_insert == 0 && node->data->flag_remove == 0)
        {
            // printf("sync_stats for node %d\n", node->data->vtt_node_id);
            xf_mtt_sync_stats(node);
        }
        node = node -> next;
    }
    return 0;
}

void* xf_sync_tcam(void* t)
{
    struct timeval t1, t2;
    float sync_time = 0.0;
    // static fm_aclCounters counters;
    while (1) 
    {
        sleep(1);
        gettimeofday(&t1, NULL);
        //printf("Starting sync_tcam\n");
        xf_mtt_tcam_sync();
        gettimeofday(&t2, NULL);
        sync_time = ((t2.tv_sec - t1.tv_sec) * 1000.0 * 1000.0) + (t2.tv_usec - t1.tv_usec);
        if (sync_time > 0.01)
            printf("sync_time = %.2f usec\n", sync_time);
        sync_time = 0.0;
        t1.tv_sec = 0;
        t2.tv_sec = 0;
        t1.tv_usec = 0;
        t2.tv_usec = 0;

        // fmGetACLCountExt(0, 0, 3071, &counters);
        // printf("trap count = %d\n", counters.cntPkts);

    }
    pthread_exit(NULL);
    return t;
}

void xf_test_insert_rule(int num_rules)
{
    static int k = 0;
    fm_status err;
    fm_int sw, acl, rule;
    fm_aclCondition cond;
    fm_aclValue value;
    fm_aclAction action;
    fm_aclParamExt param;
    fm_text statusText = (char *)malloc(200 * sizeof(char));
    fm_int statusTextLength = 200;
    struct in_addr src, dst, smask, dmask;
    int i = 0;
    sw = 0;
    acl = 0;

    for (k = num_rules; k < (num_rules+1); k++)
    {
            rule = k;
            cond = FM_ACL_MATCH_SRC_IP | FM_ACL_MATCH_DST_IP;

            inet_aton("192.168.1.1", &src);
            inet_aton("10.3.24.57", &dst);
            inet_aton("255.255.0.0",&smask);
            inet_aton("255.255.255.255",&dmask);

            value.srcIp.addr[0] = src.s_addr;
            value.srcIp.isIPv6 = 0;
            value.srcIpMask.addr[0] = smask.s_addr;
            value.srcIpMask.isIPv6 = 0;

            value.dstIp.addr[0] = dst.s_addr;
            value.dstIp.isIPv6 = 0;
            value.dstIpMask.addr[0] = dmask.s_addr;
            value.dstIpMask.isIPv6 = 0;

            action = FM_ACL_ACTIONEXT_DENY | FM_ACL_ACTIONEXT_COUNT;

            if ( ( err = fmAddACLRuleExt(sw, acl, rule, cond, &value, \
                                         action, &param) ) != FM_OK )
            {
                cleanup("fmAddACLRuleExt", err);
            }
    }

}

