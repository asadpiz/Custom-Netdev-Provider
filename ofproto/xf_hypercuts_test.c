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
 * Test Hypercuts Packet Classification.
 */

#include "xf_hypercuts.h"

/**
 *@breif             Main function to test Hypercut
 *
 *@param  argc       Number of arguments
 *@param  argv       Arguments from command line
 *
 *@return            0 for success, other means fail.
 */
int main(int argc, char* argv[]) {
    struct xf_tree* tree = &vtt_handle;
#ifndef OVS
    parseargs(tree, argc, argv);
#endif
    xf_tree_build(tree);
    xf_partition_rules(tree);
    /* insertNode(tree); */
    tracesLookup(tree);
    xf_tree_destroy(tree);
    return 0;
}
