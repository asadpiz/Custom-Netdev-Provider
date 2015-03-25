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
 * Implementation of test for dynamic heap
 */

#include "xf_dheap.h"

/**
 *@brief             Main function to test dheap Functions
 *
 *@param  argc       Number of arguments
 *@param  argv       Arguments from command line
 *
 *@return            0 for success, other means fail.
 */
int main(void) {
    int i = 0;
    struct dheap* H1 = dheap_init(9, 2);

    for (i = 4; i > 0; i--) {
        dheap_insert(H1, i, 100 + i);
        dheap_print(H1);
    }

    dheap_exit(H1);
    return 0;
}
