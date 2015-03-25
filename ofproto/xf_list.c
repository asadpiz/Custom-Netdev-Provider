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
 * Implementation of Hypercut's List for NOVA1
 */

#include "xf_list.h"

/**
  *
  *@brief              Initialize the Hypercut's List
  *
  */
struct xf_list* xf_list_init(int N1) {
    int i;
    struct xf_list* node_list = (struct xf_list*)malloc(sizeof(struct xf_list));
    node_list->N = N1;
    node_list->first = node_list->last = Null;
    node_list->next = (int*)calloc(N1 + 1, sizeof(int));

    for (i = 1; i <= node_list->N; i++) {
        node_list->next[i] = -1;
    }

    /* node_list->next[Null] = Null; */
    return node_list;
}

/**
  *
  *@brief             Destroy the Hypercut's List
  *
  */
void xf_list_exit(struct xf_list* node_list) {
    free(node_list->next);
    free(node_list);
}


/**
  *
  *@brief              Add i to the front of the list.
  *
  */
void xf_list_push(struct xf_list* node_list, int i) {
    if (node_list->next[i] != -1) {
        fprintf(stderr, "Fatal:list_push: item already in list\n");
        fprintf(stderr, "i=%d,node_list->next[i]=%d != Null\n",
                i, node_list->next[i]);
        exit(1);
    }

    if (node_list->first == -1) {
        node_list->last = i;
    }

    node_list->next[i] = node_list->first;
    node_list->first = i;
}

/**
  *
  *@brief              Add i to the end of the list.
  *
  */
void xf_list_append(struct xf_list* node_list, int i) {
    if (node_list->next[i] != -1) {
        fprintf(stderr, "list_append: item already in list\n");
        fprintf(stderr, "i=%d,node_list->next[i]=%d !Null\n",
                i, node_list->next[i]);
    }

    if (node_list->first == Null) {
        node_list->first = i;
    } else {
        node_list->next[node_list->last] = i;
    }

    node_list->next[i] = Null;
    node_list->last = i;
}

/**
  *
  *@brief              Return i'th element of the list
  *
  */
int xf_list_get(struct xf_list* node_list, int i) {
    int j;

    if (i == 1) {
        return node_list->first;
    }

    for (j = node_list->first; j != Null && --i; j = node_list->next[j]) {}

    return j;
}

/**
  *
  *@brief              Remove the first i items.
  *
  */
void xf_list_trunc(struct xf_list* node_list, int i) {
    while (node_list->first != Null && i--) {
        int f = node_list->first;
        node_list->first = node_list->next[f];
        node_list->next[f] = -1;
    }
}

/**
  *
  *@brief              Return the successor of i.
  *
  */
int xf_list_suc(struct xf_list* node_list, int i) {
    if (node_list->next[i] == -1) {
        fprintf(stderr, "Fatal: list_suc:item not on list ");
        fprintf(stderr, "node_list->next[%d]=Null\n", i);
        exit(0);
    }

    return node_list->next[i];
}

/**
  *
  *@brief              Assign value of second list to first list.
  *
  */
void xf_list_assign(struct xf_list* node_list, struct xf_list* L) {
    int i;

    if (node_list->N < L->N) {
        node_list->N = L->N;
        free(node_list->next);
        node_list->next = (int*)calloc(L->N + 1, sizeof(int));
        node_list->first = node_list->last = Null;

        for (i = 1; i <= node_list->N; i++) {
            node_list->next[i] = -1;
        }

        /* node_list->next[Null] = Null; */
    } else {
        xf_list_clear(node_list);
    }

    for (i = xf_list_get(L, 1); i != -1; i = xf_list_suc(L, i)) {
        xf_list_append(node_list, i);
    }
}

/**
  *
  *@brief              Print the items on list
  *
  */
void xf_list_print(struct xf_list* node_list) {
    int i;

    for (i = node_list->first; i != Null; i = node_list->next[i]) {
        printf("%d ", i);
    }

    putchar('\n');
}

/**
  *
  *@brief              Change size of list. Discard old value.
  *
  */
void xf_list_reset(struct xf_list* node_list, int N1) {
    int i;
    free(node_list->next);
    node_list->N = N1;
    node_list->next = (int*)calloc(N1 + 1, sizeof(int));
    node_list->first = node_list->last = Null;

    for (i = 1; i <= node_list->N; i++) {
        node_list->next[i] = -1;
    }

    /* node_list->next[Null] = Null; */
}

/**
  *
  *@brief              Remove all elements from list.
  *
  */
void xf_list_clear(struct xf_list* node_list) {
    while (node_list->first != -1) {
        int i = node_list->first;
        node_list->first = node_list->next[i];
        node_list->next[i] = -1;
    }
}


/**
  *
  *@brief              Return true if i in list, else false.
  *
  */
inline bit xf_list_mbr(struct xf_list* node_list, int i) {
    return node_list->next[i] != -1;
}

/**
  *
  *@brief              Return last item on list
  *
  */
inline int xf_list_tail(struct xf_list* node_list) {
    return node_list->last;
}
