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
 * Implementation of Standard Include Functions.
 */

#include "xf_stdinc.h"

const int false = 0;
const int true = 1;
const int Null = -1;
const int BIGINT = 0x7fffffff;
const int EOS = '\0';

/*
 * Return maximum integer
 */
inline int max_int(int x, int y) {
    return x > y ? x : y;
}

/*
 * Return maximum unsigned integer
 */
inline unsigned max_unsigned(unsigned x, unsigned y) {
    return x > y ? x : y;
}

/*
 * Return maximum double
 */
inline double max_double(double x, double y) {
    return x > y ? x : y;
}

/*
 * Return minimum integer
 */
inline int min_int(int x, int y) {
    return x < y ? x : y;
}

/*
 * Return minimum unsigned integer
 */
inline unsigned min_unsigned(unsigned x, unsigned y) {
    return x < y ? x : y;
}

/*
 * Return minimum double
 */
inline double min_double(double x, double y) {
    return x < y ? x : y;
}

/*
 * Return absolute integer
 */
inline int abs(int x) {
    return x < 0 ? -x : x;
}

/*
 * Print the warning message
 */
inline void warning(const char* p) {
    fprintf(stderr, "Warning:%s \n", p);
}

/*
 * Print the fatal error message
 */
inline void fatal(const char* string) {
    fprintf(stderr, "Fatal:%s\n", string);
    exit(1);
}

/*
 * Math Library Functions
 */
/*
inline double pow(double, double);
inline double log(double);
inline long random();
inline double exp(double), log(double);
*/

/*
 * Return a random number in [0,1]
 */
inline double randfrac(void) {
    return (double)(((double) random()) / BIGINT);
}

/*
 * Return a random integer in the range [lo,hi].
 * Not very good if range is larger than 10**7.
 */
inline int randint(int lo, int hi) {
    return lo + (random() % (hi + 1 - lo));
}

/*
 * Return a random number from an exponential distribution with mean mu
 */
inline double randexp(double mu) {
    return -mu * log(randfrac());
}

/*
 * Return a random number from a geometric distribution with mean 1/p
 */
inline int randgeo(double p) {
    return (int)(.999999 + log(randfrac()) / log(1 - p));
}

/*
 * Return a random number from a Pareto distribution with mean mu and shape s
 */
inline double randpar(double mu, double s) {
    return mu * (1 - 1 / s) / exp((1 / s) * log(randfrac()));
}
