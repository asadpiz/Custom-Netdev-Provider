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
 * MTT Test File for NOVA1
 */

#include "xf_mtt.h"

/**
 *@breif             Main function to test MTT Functions
 *
 *@param  argc       Number of arguments
 *@param  argv       Arguments from command line
 *
 *@return            0 for success, other means fail.
 */
int main(int argc, char* argv[]) {
    struct xf_tree* tree = (struct xf_tree*)malloc(sizeof(struct xf_tree));
    int ch = 0;
    /* seed for rand() */
    struct timeval t1;
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);
    parseargs(tree, argc, argv);
    xf_tree_build(tree);
    /* xf_partition_rules(tree); */
    /* insertNode(tree); */
    /* tracesLookup(tree); */
    xf_mtt_init();
    fm_init();
    printf("FM init done\n");

    /* XF_ACL initialize */
    if ((xf_acl_init()) < 0) {
        printf("ACL not initialized\n");
        exit(1);  // exit if ACL could not be initialized
    } else {
        printf("ACL Initilized\n");
    }

    printf("Enter Node # to insert (50 to exit)\n");
    scanf("%d", &ch);

    while (ch != 50) {
        xf_mtt_insert(tree, ch);
        printf("Enter Node # to insert (50 to exit)\n");
        scanf("%d", &ch);
    }

    printf("Enter Node # to delete (50 to exit)\n");
    scanf("%d", &ch);

    while (ch != 50) {
        xf_mtt_delete(ch);
        printf("Enter Node # to delete (50 to exit)\n");
        scanf("%d", &ch);
    }

    xf_mtt_exit();
    xf_tree_destroy(tree);
    return 0;
}
