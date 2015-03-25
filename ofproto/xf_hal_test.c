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
 * HAL Test File for NOVA1
 */
#include "xf_hal.h"

/**
 *@breif             Main function to test HAL Functions
 *
 *@param  argc       Number of arguments
 *@param  argv       Arguments from command line
 *
 *@return            0 for success, other means fail.
 */
int main(void) {
    int i = 0;
    /* seed for rand() */
    struct timeval t1;
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);
    xf_hal_init();

    for (i = 0; i < 10; i++) {
        xf_hal_insert(rand() % 100);
        printf("Node %d is present %d\n", i, xf_hal_ispresent(i));
        xf_hal_read(rand() % 100);
        xf_hal_print();
        xf_hal_update_stats();
        xf_hal_delete(rand() % 100);
    }

    xf_hal_exit();
    return 0;
}
