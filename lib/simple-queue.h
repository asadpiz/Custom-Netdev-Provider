/*
 * Copyright (c) 2010, 2011 xFlow Research.
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
*/
#ifndef SIMPLE_QUEUE_H
#define SIMPLE_QUEUE_H 1


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SQ_NUM_QUEUES  12 /*one-one mapping for queues-Intel ports*/

/*
 * structure to store a packet ==> fm_buffer
*/
struct sq_pkt{
    void* data;
    int size;
    struct sq_pkt* next_pkt;
};

/*
 * simple link list to store packets as a queue
*/
struct sq_pkt_q{
    int n_pkts;
    struct sq_pkt** head;
};

struct sq_pkt_q soft_q[SQ_NUM_QUEUES];
/*initializes the queues*/
void sq_init(struct sq_pkt_q q_list[]);

/*prints the contents of q. Meant for testing only. Assumes that the data is always char* type*/
void sq_print_q(int q_id, struct sq_pkt_q* q_list);

/*pushes an item in the beginning of q*/
int sq_push(int q_id, struct sq_pkt_q* q_list, void* data, size_t size);

/*pops an item from the end of the queue*/
int sq_pop(int q_id, struct sq_pkt_q* q_list, void* data, size_t size);

#endif  /*simple-queue.h*/
