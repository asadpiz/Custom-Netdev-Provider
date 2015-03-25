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
 * Implementation of Test function for Hypercut's List.
 */

#include "xf_list.h"

/**
 *@breif             Main function to test Hypercut's List Functions
 *
 *@param  argc       Number of arguments
 *@param  argv       Arguments from command line
 *
 *@return            0 for success, other means fail.
 */
int main(void) {
    int i = 0;
    struct xf_list* Q = xf_list_init(10);
    struct xf_list* R = xf_list_init(11);
    struct xf_list* S = xf_list_init(15);

    for (i = 1; i < 10; i++) {
        xf_list_append(Q, i);
        xf_list_print(Q);
    }

    printf("5th element=%d\n", xf_list_get(Q, 5));
    xf_list_push(Q, 10);
    printf("5th element=%d\n", xf_list_get(Q, 5));
    xf_list_print(Q);
    xf_list_assign(R, Q);
    printf("Printing R\n");
    xf_list_print(R);
    printf("Printing Q\n");
    xf_list_print(Q);

    for (i = 1; i < 16; i++) {
        xf_list_push(S, i);
    }

    xf_list_assign(R, S);
    printf("Printing R\n");
    xf_list_print(R);
    printf("Printing S\n");
    xf_list_print(S);
    printf("Printing Q\n");
    xf_list_print(Q);
    xf_list_trunc(Q, 1);
    printf("Printing Q\n");
    xf_list_print(Q);
    xf_list_trunc(Q, 2);
    printf("Printing Q\n");
    xf_list_print(Q);
    xf_list_exit(Q);
    xf_list_exit(R);
    xf_list_exit(S);
    return 0;
}
