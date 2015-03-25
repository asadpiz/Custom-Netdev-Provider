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
 * Standard Include File.
 */

#ifndef XF_STDINC_H
#define XF_STDINC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <limits.h>

typedef char bit;

extern const int Null;
extern const int BIGINT;
extern const int EOS;

inline int max_int(int x, int y);
inline unsigned max_unsigned(unsigned x, unsigned y);
inline double max_double(double x, double y);
inline int min_int(int x, int y);
inline unsigned min_unsigned(unsigned x, unsigned y);
inline double min_double(double x, double y);
inline int abs(int x);

inline void warning(const char* p);
inline void fatal(const char* string);

/* inline double pow(double, double);
inline double log(double); */
inline double exp(double), log(double), pow(double, double);

inline double randfrac(void);
inline int randint(int lo, int hi);
inline double randexp(double mu);
inline int randgeo(double p);
inline double randpar(double mu, double s);

#endif  /* XF_STDINC_H */
