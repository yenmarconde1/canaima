/*
 * utils.h             General purpose utilities
 *
 * $Id: utils.h 1 2004-10-17 17:32:34Z tgr $
 *
 * Copyright (c) 2001-2004 Thomas Graf <tgraf@suug.ch>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef __BMON_UTILS_H_
#define __BMON_UTILS_H_

#include <bmon/bmon.h>

#define COPY_TS(tv1,tv2)                                     \
    do {                                                     \
        (tv1)->tv_sec = (tv2)->tv_sec;                       \
        (tv1)->tv_usec = (tv2)->tv_usec;                     \
    } while (0)

extern float read_delta;

extern void * xcalloc(size_t n, size_t s);
extern void * xrealloc(void *p, size_t s);
extern void xfree(void *d);
extern void quit (const char *fmt, ...);

const char * xinet_ntop(struct sockaddr *src, char *dst, socklen_t cnt);

extern double sumup(b_cnt_t l, char **unit);

extern inline float ts_to_float(timestamp_t *src);
extern inline void float_to_ts(timestamp_t *dst, float src);

extern inline void ts_add(timestamp_t *dst, timestamp_t *src1, timestamp_t *src2);
extern inline void ts_sub(timestamp_t *dst, timestamp_t *src1, timestamp_t *src2);
extern inline int ts_le(timestamp_t *a, timestamp_t *b);
extern inline void update_ts(timestamp_t *dst);

extern float time_diff(timestamp_t *t1, timestamp_t *t2);
extern float diff_now(timestamp_t *t1);

#endif
