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


#include "simple-queue.h"

/*initializes the queues*/
void sq_init(struct sq_pkt_q q_list[])
{
    int i = 0;
    for (; i<SQ_NUM_QUEUES; i++){
        q_list[i].n_pkts = 0;
        q_list[i].head = NULL;
    }
}

/*prints the contents of q. Meant for testing only. Assumes that the data is always char* type*/
void sq_print_q(int q_id, struct sq_pkt_q* q_list)
{
    int i = q_list[q_id].n_pkts;
    struct sq_pkt *temp_pkt = q_list[q_id].head;
    if (temp_pkt == NULL)
        return;
    while(temp_pkt != NULL){
        printf("%03d -> %s\n", q_list[q_id].n_pkts-i--, (char*)temp_pkt->data);
        temp_pkt = temp_pkt->next_pkt;
    }
}

/*
* pushes an item in the beginning of q
* does not free data
* returns 1 if success
*/
int sq_push(int q_id, struct sq_pkt_q* q_list, void* data, size_t size)
{
     struct sq_pkt* new_pkt = NULL, *pkt_p;
     if((new_pkt = (struct sq_pkt*) malloc(sizeof(struct sq_pkt))) == NULL)
         return 0;
     new_pkt->data = malloc(size);
     if(new_pkt->data == NULL)
  	  return 0;
    
      q_list[q_id].n_pkts++;
      new_pkt->size = size;
      memcpy(new_pkt->data, data, size);
      new_pkt->next_pkt = NULL;

    if(q_list[q_id].head == NULL){
        q_list[q_id].head = new_pkt;
        return 1;
    }
    pkt_p = q_list[q_id].head;
    while(pkt_p->next_pkt != NULL)
        pkt_p = pkt_p->next_pkt;
    pkt_p->next_pkt = new_pkt;
    return 1;
}
/*
* pops a packet from q_id index of q_list
* assumes data has been allocated 'size' by caller
* returns 0 if no data otherwise returns size of received data 
*/
int sq_pop(int q_id, struct sq_pkt_q* q_list, void* data, size_t size)
{
    struct sq_pkt* tmp_pkt = q_list[q_id].head;
    int pkt_size = 0;
  
    if(tmp_pkt == NULL)
        return 0;
  
    if(q_list[q_id].n_pkts < 1)
      return 0;
  
    if(tmp_pkt->size > size){
        printf("[WARN] Popped pkt's payload size is greater than the \
                     buffer provided. So returning.\n"); 
        return 0;
    }
    memcpy(data, tmp_pkt->data, tmp_pkt->size);
    pkt_size = tmp_pkt->size;
    q_list[q_id].head = tmp_pkt->next_pkt;
    q_list[q_id].n_pkts--;
    free(tmp_pkt->data);
    free(tmp_pkt);
    return pkt_size;
}
