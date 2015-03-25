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
 * Implementation of dynamic heap data structure
 * Maintains a subset of items in {1,...,m}, where item has a key.
 */

#include "xf_dheap.h"

/*
 * parent of item, leftmost and rightmost children
 */
#define p(x , d) (((x)+(d-2))/d)
#define left(x , d) (d*((x)-1)+2)
#define right(x , d) (d*(x)+1)

/*
 * Initialize dynamic heap to store items in {1,...,N}.
 */
struct dheap* dheap_init(int N1, int d1) {
    struct dheap* dheap_node = (struct dheap*)malloc(sizeof(struct dheap));
    static int x = 0;
    int i;
    x++;
    dheap_node->N = N1;
    dheap_node->d = d1;
    dheap_node->n = 0;
    dheap_node->h = (item*)calloc(dheap_node->N + 1, sizeof(item));
    dheap_node->pos = (int*)calloc(dheap_node->N + 1, sizeof(int));
    dheap_node->kvec = (keytyp*)calloc(dheap_node->N + 1, sizeof(keytyp));

    for (i = 1; i <= dheap_node->N; i++) {
        dheap_node->pos[i] = Null;
    }

    return dheap_node;
}

/*
 * Destroy the dynamic heap
 */
void dheap_exit(struct dheap* dheap_node) {
    free(dheap_node->h);
    free(dheap_node->pos);
    free(dheap_node->kvec);
    free(dheap_node);
}

/*
 * Insert item i with specified key to heap.
 */
void dheap_insert(struct dheap* dheap_node, item i, keytyp k) {
    dheap_node->kvec[i] = k;
    dheap_node->n++;
    dheap_siftup(dheap_node, i, dheap_node->n);
}

/*
 * Remove item i from heap.
 * Name remove is used since delete is C++ keyword.
 */
void dheap_remove(struct dheap* dheap_node, item i) {
    int j = dheap_node->h[dheap_node->n--];    /* if i==j, i item is removed */

    if (i != j && dheap_node->kvec[j] <= dheap_node->kvec[i]) {
        dheap_siftup(dheap_node, j, dheap_node->pos[i]);
    } else if (i != j && dheap_node->kvec[j] >  dheap_node->kvec[i]) {
        dheap_siftdown(dheap_node, j, dheap_node->pos[i]);
    }

    dheap_node->pos[i] = Null;
}
/*
 * Remove and return item with smallest key.
 */
int dheap_deletemin(struct dheap* dheap_node) {
    item i;

    if (dheap_node->n == 0) {
        return Null;
    }

    i = dheap_node->h[1];
    dheap_remove(dheap_node, dheap_node->h[1]);
    return i;
}

/*
 * Shift item i up from position x to restore heap order.
 */
void dheap_siftup(struct dheap* dheap_node, item i, int x) {
    int px = p(x, dheap_node->d);

    while (x > 1 && dheap_node->kvec[dheap_node->h[px]] > dheap_node->kvec[i]) {
        dheap_node->h[x] = dheap_node->h[px];
        dheap_node->pos[dheap_node->h[x]] = x;
        x = px;
        px = p(x, dheap_node->d);
    }

    dheap_node->h[x] = i;
    dheap_node->pos[i] = x;
}

/*
 * Shift item i down from position x to restore heap order.
 */
void dheap_siftdown(struct dheap* dheap_node, item i, int x) {
    int cx = dheap_minchild(dheap_node, x);

    while (cx != Null
            && dheap_node->kvec[dheap_node->h[cx]] < dheap_node->kvec[i]) {
        dheap_node->h[x] = dheap_node->h[cx];
        dheap_node->pos[dheap_node->h[x]] = x;
        x = cx;
        cx = dheap_minchild(dheap_node, x);
    }

    dheap_node->h[x] = i;
    dheap_node->pos[i] = x;
}

/*
 * Return position of the child of the item at position x having minimum key.
 */
int dheap_minchild(struct dheap* dheap_node, int x) {
    int y, minc;

    if ((minc = left(x, dheap_node->d)) > dheap_node->n) {
        return Null;
    }

    for (y = minc + 1; y <= right(x, dheap_node->d) && y <= dheap_node->n;
            y++) {
        if (dheap_node->kvec[dheap_node->h[y]] <
                dheap_node->kvec[dheap_node->h[minc]]) {
            minc = y;
        }
    }

    return minc;
}

/*
 * Change the key of item i and restore heap order.
 */
void dheap_changekey(struct dheap* dheap_node, item i, keytyp k) {
    keytyp ki = dheap_node->kvec[i];
    dheap_node->kvec[i] = k;

    if (k < ki) {
        dheap_siftup(dheap_node, i, dheap_node->pos[i]);
    } else if (k > ki) {
        dheap_siftdown(dheap_node, i, dheap_node->pos[i]);
    }
}

/*
 * Print the contents of the heap.
 */
void dheap_print(struct dheap* dheap_node) {
    int x;
    printf("*******************heap*********************\n");
    printf("   h:");

    for (x = 1; x <= dheap_node->n; x++) {
        printf(" %10d", dheap_node->h[x]);
    }

    printf("\nkvec:");

    for (x = 1; x <= dheap_node->n; x++) {
        printf(" %10lu", dheap_node->kvec[dheap_node->h[x]]);
    }

    printf("\npos:");

    for (x = 1; x <= dheap_node->n; x++) {
        printf(" %10d", dheap_node->pos[dheap_node->h[x]]);
    }

    putchar('\n');
    printf("********************************************\n");
}

/*
 * Return item with smallest key.
 */
inline int dheap_findmin(struct dheap* dheap_node) {
    return dheap_node->n == 0 ? Null : dheap_node->h[1];
}

/*
 * Return key of item i.
 *
 */
inline keytyp dheap_key(struct dheap* dheap_node, item i) {
    return i == Null ? Null : dheap_node->kvec[i];
}

/*
 * Return true if item i in heap, else false.
 */
inline bit dheap_member(struct dheap* dheap_node, item i) {
    return dheap_node->pos[i] != Null;
}

/*
 * Return true if heap is empty, else false.
 */
inline bit dheap_empty(struct dheap* dheap_node) {
    return dheap_node->n == 0;
}
