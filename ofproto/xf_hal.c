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
 *
 *
 * Implementation of HAL for NOVA1
 * This contain three type of functions
 * xf_hal_* are HAL functions.
 * xf_rule_* are rule related functions.
 * xf_hal_list_* are linked list related functions
 */

#include "xf_hal.h"

/**
 *@brief                Initialize the HAL and loads all the rules from file
 *
 *@return               0 for success, other means fail.
 */
int xf_hal_init(void) {
    xf_hal_handle = NULL;   /* HAL Handle. Pointer to head of linked list */
    /* xf_hal_load_file(); */
    return 0;
}

/**
 *@brief                Initialize the HAL and loads all the rules from file
 *
 *@return               0 for success, other means fail.
 */
int xf_hal_load_file(void) {
    FILE* fp = fopen("TCAM", "r");

    if (fp == NULL) {
        fp = fopen("TCAM", "w");
    }

    while (1) {
        struct xf_hal_rule* s_rule =
            (struct xf_hal_rule*)malloc(sizeof(struct xf_hal_rule));

        if (fscanf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                   &s_rule->id, &s_rule->rule_id, &s_rule->field[0].high,
                   &s_rule->field[0].low, &s_rule->field[1].high,
                   &s_rule->field[1].low, &s_rule->field[2].high,
                   &s_rule->field[2].low, &s_rule->field[3].high,
                   &s_rule->field[3].low, &s_rule->field[4].high,
                   &s_rule->field[4].low, &s_rule->stats.packets,
                   &s_rule->stats.bytes) != 14) {
            free(s_rule);
            break;
        }

        /* push value onto Linked List */
        xf_hal_list_push_last(&xf_hal_handle, s_rule);
    }

    rewind(fp);
    printf("%d Elements Loaded from TCAM \n", xf_hal_list_len(xf_hal_handle));
    fclose(fp);
    return 0;
}

/**
 *@brief                Insert the rule in TCAM
 *
 *@param  rule_id       Rule id to insert
 *
 *@return               Return rule id, -1 means fail.
*/
int xf_hal_insert(int rule_id) {
    if (xf_hal_list_elem(&xf_hal_handle, rule_id)) {
        printf("Rule with rule_id %d already in HAL\n", rule_id);
        return -1;
    } else {
        struct xf_hal_rule* s_rule =
            (struct xf_hal_rule*)malloc(sizeof(struct xf_hal_rule));
        struct xf_rule* rule = (&vtt_handle)->rule;
        s_rule->id = rule_id;
        s_rule->rule_id = rule_id;

        if (rule != NULL) {
            s_rule->field[0].high = rule[rule_id].field[0].high;
            s_rule->field[0].low = rule[rule_id].field[0].low;
            s_rule->field[1].high = rule[rule_id].field[1].high;
            s_rule->field[1].low = rule[rule_id].field[1].low;
            s_rule->field[2].high = rule[rule_id].field[2].high;
            s_rule->field[2].low = rule[rule_id].field[2].low;
            s_rule->field[3].high = rule[rule_id].field[3].high;
            s_rule->field[3].low = rule[rule_id].field[3].low;
            s_rule->field[4].high = rule[rule_id].field[4].high;
            s_rule->field[4].low = rule[rule_id].field[4].low;
            s_rule->stats.packets = 0;
            s_rule->stats.bytes = 0;
        } else {
            struct timeval t1;
            gettimeofday(&t1, NULL);
            srand(t1.tv_usec * t1.tv_sec);
            s_rule->id = rand() % 100;
            s_rule->rule_id = s_rule->id;
            s_rule->field[0].high = rand();
            s_rule->field[0].low = rand();
            s_rule->field[1].high = rand();
            s_rule->field[1].low = rand();
            s_rule->field[2].high = rand() % 0xFFFF;
            s_rule->field[2].low = rand() % 0xFFFF;
            s_rule->field[3].high = rand() % 0xFFFF;
            s_rule->field[3].low = rand() % 0xFFFF;
            s_rule->field[4].high = rand() % 0xFF;
            s_rule->field[4].low = rand() % 0xFF;
            s_rule->stats.bytes = rand() % 100;
            s_rule->stats.packets = rand() % 10;
        }

        /* push value onto Linked List */
        xf_hal_list_push_last(&xf_hal_handle, s_rule);
        /* printf("Rule with id %d inserted in HAL\n",s_rule->rule_id); */
        return s_rule->id;
    }
}

/**
 *@brief                Reads a rule from TCAM
 *
 *@param  rule_id       Id for reading a rule
 *
 *@return               Return rule id, -1 means fail.
*/
int xf_hal_read(int rule_id) {
    struct xf_hal_rule* s_rule;

    if ((s_rule = xf_hal_list_elem(&xf_hal_handle, rule_id))) {
        xf_hal_rule_print(s_rule);
        return s_rule->id;
    } else {
        printf("Rule with id %d not in HAL for read\n", rule_id);
        return -1;
    }
}

/**
 *@brief                Deletes a rule from TCAM
 *
 *@param  rule_id       Id for deleting a rule
 *
 *@return               Return rule id, -1 means fail.
*/
int xf_hal_delete(int rule_id) {
    if (xf_hal_list_delete(&xf_hal_handle, rule_id)) {
        /* printf("Rule with id %d Deleted from HAL\n", rule_id); */
        return rule_id;
    } else {
        printf("Rule with id %d not in HAL for delete\n", rule_id);
        return -1;
    }
}

/**
 *@brief                checks if the node is present in MTT
 *
 *@param  vtt_node_id   vtt_node_id for reading a node
 *
 *@return               1 for node_id is present in MTT, 0 for otherwose.
*/
int xf_hal_ispresent(int rule_id) {
    return xf_hal_list_elem(&xf_hal_handle, rule_id) != NULL ? 1 : 0;
}

/**
 *@brief                Return current no of nodes in MTT
 *
 *@return               length of MTT
*/
int xf_hal_length(void) {
    return xf_hal_list_len(xf_hal_handle);
}

/**
 *@brief                Prints the HAL
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_print(void) {
    printf("==========HAL PRINT==========\n");
    xf_hal_list_print(&xf_hal_handle);
    return 0;
}

/**
 *@brief                Exit the HAL and save all the rules in TCAM
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_exit(void) {
    xf_hal_dump_file();
    xf_hal_list_clear(&xf_hal_handle);
    return 0;
}

/**
 *@brief              Save all the rules in TCAM
 *
 *@return             0 for success, other means fail.
*/
int xf_hal_dump_file(void) {
    struct xf_hal_node* node_curr = xf_hal_handle;
    struct xf_hal_rule* s_rule = NULL;
    int i = 0;
    FILE* fp;
    fp = fopen("TCAM", "w");

    if (fp == NULL) {
        printf("ERROR: Can not open TCAM File for HAL dump\n");
        exit(0);
    }

    while (node_curr) {
        s_rule = node_curr -> data;
        fprintf(fp, "%u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                s_rule->id, s_rule->rule_id, s_rule->field[0].high,
                s_rule->field[0].low, s_rule->field[1].high,
                s_rule->field[1].low, s_rule->field[2].high,
                s_rule->field[2].low, s_rule->field[3].high,
                s_rule->field[3].low, s_rule->field[4].high,
                s_rule->field[4].low, s_rule->stats.packets,
                s_rule->stats.bytes);
        node_curr = node_curr -> next;
        i++;
    }

    printf("%d Elements saved in TCAM on Exit\n", i);
    fclose(fp);
    return 0;
}

/**
 *@brief                Push a rule in the linked list
 *
 *@param  hal_handle Pointer to linked list
 *@param  data          Rule to be pushed in linked list
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_list_push(struct xf_hal_node** hal_handle,
                     struct xf_hal_rule* data) {
    struct xf_hal_node* node_new = malloc(sizeof(struct xf_hal_node));
    node_new -> data = data;
    node_new -> next = *hal_handle;
    *hal_handle = node_new;
    return 0;
}

/**
 *@brief                Add element on the end of linked list
 *
 *@param  hal_handle Pointer to linked list
 *@param  data          Rule to be inserted in linked list
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_list_push_last(struct xf_hal_node** hal_handle,
                          struct xf_hal_rule* data) {
    struct xf_hal_node* node_curr = *hal_handle;

    if (!node_curr) {
        xf_hal_list_push(hal_handle, data);
    } else  {
        /* find the last node */
        while (node_curr -> next) {
            node_curr = node_curr -> next;
        }

        /* build the node after it */
        xf_hal_list_push(&(node_curr -> next), data);
    }

    return 0;
}

/**
 *@brief                Get an element from the list
 *
 *@param  hal_handle Pointer to linked list head
 *
 *@return               Rule from linked list
*/
struct xf_hal_rule* xf_hal_list_pop(struct xf_hal_node** hal_handle) {
    struct xf_hal_node* node_curr = *hal_handle;
    struct xf_hal_rule* data = NULL;

    if (hal_handle)  {
        data = node_curr -> data;
        *hal_handle = node_curr -> next;
        free(node_curr->data);
        free(node_curr);
    }

    return data;
}


/**
 *@brief                Deletes a rule in the linked list
 *
 *@param  hal_handle Pointer to linked list
 *@param  data          Rule to be Deleted in linked list
 *
 *@return               1 for success, 0 means not deleted.
*/
int xf_hal_list_delete(struct xf_hal_node** hal_handle, int rule_id) {
    struct xf_hal_node* node_curr_prev = *hal_handle;
    struct xf_hal_node* node_curr = node_curr_prev->next;

    /* pop the Linked List's head if rule is present on head */
    if (node_curr_prev -> data->rule_id == rule_id) {
        xf_hal_list_pop(hal_handle);
        return 1;
    }

    while (node_curr) {
        if (node_curr -> data->rule_id == rule_id) {
            node_curr_prev->next = node_curr->next;
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
 *@brief                Find if rule is present
 *
 *@param  hal_handle Pointer to linked list
 *@param  rule_id       Rule to find from linked list
 *
 *@return               1 for rule found, 0 for not found.
*/
struct xf_hal_rule* xf_hal_list_elem(struct xf_hal_node** hal_handle,
                                     int rule_id) {
    struct xf_hal_node* node_curr = *hal_handle;

    while (node_curr) {
        if (node_curr -> data->rule_id == rule_id) {
            return node_curr->data;
        } else {
            node_curr = node_curr -> next;
        }
    }

    return NULL;
}

/**
 *@brief                Return the length of linked list
 *
 *@param  hal_handle Pointer to linked list
 *
 *@return               Length of the linked list. Currently stored rules
*/
int xf_hal_list_len(struct xf_hal_node* hal_handle) {
    struct xf_hal_node* curr = hal_handle;
    int len = 0;

    while (curr) {
        ++len;
        curr = curr -> next;
    }

    return len;
}

/**
 *@brief                Print the currently stored rules
 *
 *@param  hal_handle Pointer to linked list
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_list_print(struct xf_hal_node** hal_handle) {
    struct xf_hal_node* node_curr = *hal_handle;

    if (!node_curr) {
        printf("HAL is empty\n");
    } else  {
        while (node_curr) {
            xf_hal_rule_print(node_curr -> data);
            node_curr = node_curr -> next;
        }
    }

    return 0;
}

/**
 *@brief                Delete all rules stored
 *
 *@param  hal_handle Pointer to linked list
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_list_clear(struct xf_hal_node** hal_handle) {
    while (*hal_handle) {
        xf_hal_list_pop(hal_handle);
    }

    return 0;
}


/**
 *@brief                Prints a rule
 *
 *@param  rule          Rule to print
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_rule_print(struct xf_hal_rule* s_rule) {
    if (s_rule != NULL) {
        /*        printf("RULE: %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",
                       s_rule->id,s_rule->rule_id,s_rule->field[0].high,
                       s_rule->field[0].low,s_rule->field[1].high,
                       s_rule->field[1].low,s_rule->field[2].high,
                       s_rule->field[2].low,s_rule->field[3].high,
                       s_rule->field[3].low,s_rule->field[4].high,
                       s_rule->field[4].low,s_rule->stats.packets,
                       s_rule->stats.bytes);*/
        printf("Rule ID: %u %u pkts/bytes %u/%u\n",
               s_rule->id, s_rule->rule_id, s_rule->stats.packets,
               s_rule->stats.bytes);
    }

    return 0;
}


/**
 *@brief                updates the TCAM stats randomly
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_update_stats(void) {
    xf_hal_list_update_stats(&xf_hal_handle);
    return 0;
}

/**
 *@brief                update the stats in linked list
 *
 *@param  hal_handle Pointer to linked list
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_list_update_stats(struct xf_hal_node** hal_handle) {
    struct xf_hal_node* node_curr = *hal_handle;
    struct timeval t1;
    int ShouldUpdate;
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);

    while (node_curr) {
        ShouldUpdate = rand() % 2;

        if (ShouldUpdate) {
            xf_hal_rule_update_stats(node_curr->data);
            /* printf("Rule %d Stats Updated\n", node_curr->data->rule_id); */
        } else {
            /* printf("Rule %d Stats Not updated\n", node_curr->data->rule_id); */
        }

        node_curr = node_curr -> next;
    }

    return 0;
}


/**
 *@brief                Update stats of the rule randomly
 *
 *@param  rule          Rule to update stats
 *
 *@return               0 for success, other means fail.
*/
int xf_hal_rule_update_stats(struct xf_hal_rule* s_rule) {
    if (s_rule) {
        int pkts = rand() % 10;
        s_rule->stats.packets += pkts;
        /* packet size 30 bytes to 60 bytes randomly */
        s_rule->stats.bytes += ((pkts * rand() % 30) + 30);
    }

    return 0;
}

/**
 *@brief                Reads a rule from TCAM
 *
 *@param  rule_id       Id for reading a rule
 *
 *@return               -1 for fail, other means success.
*/
int xf_hal_sync_stats(int rule_id, struct xf_hal_rule_stats* node) {
    struct xf_hal_rule* s_rule;

    if ((s_rule = xf_hal_list_elem(&xf_hal_handle, rule_id))) {
        node->packets = s_rule->stats.packets;
        node->bytes = s_rule->stats.bytes;
        return 0;
    } else {
        printf("Rule with id %d not in HAL for sync\n", rule_id);
        return -1;
    }
}
