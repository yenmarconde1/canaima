/*
 * utils.c             General purpose utilities
 *
 * $Id: utils.c 1 2004-10-17 17:32:34Z tgr $
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

#include <bmon/bmon.h>
#include <bmon/conf.h>
#include <bmon/utils.h>

void *
xcalloc(size_t n, size_t s)
{
	void *d = calloc(n, s);

	if (NULL == d) {
		fprintf(stderr, "xalloc: Out of memory\n");
		exit(ENOMEM);
	}

	return d;
}

void *
xrealloc(void *p, size_t s)
{
	void *d = realloc(p, s);

	if (NULL == d) {
		fprintf(stderr, "xrealloc: Out of memory!\n");
		exit(ENOMEM);
	}

	return d;
}

void
xfree(void *d)
{
	if (d)
		free(d);
}

inline float
ts_to_float(timestamp_t *src)
{
	return (float) src->tv_sec + ((float) src->tv_usec / 1000000.0f);
}

inline void
float_to_ts(timestamp_t *dst, float src)
{
	dst->tv_sec = (time_t) src;
	dst->tv_usec = (src - ((float) ((time_t) src))) * 1000000.0f;
}

inline void
ts_add(timestamp_t *dst, timestamp_t *src1, timestamp_t *src2)
{
	dst->tv_sec = src1->tv_sec + src2->tv_sec;
	dst->tv_usec = src1->tv_usec + src2->tv_usec;

	if (dst->tv_usec >= 1000000) {
		dst->tv_sec++;
		dst->tv_usec -= 1000000;
	}
}

inline void
ts_sub(timestamp_t *dst, timestamp_t *src1, timestamp_t *src2)
{
	dst->tv_sec = src1->tv_sec - src2->tv_sec;
	dst->tv_usec = src1->tv_usec - src2->tv_usec;
	if (dst->tv_usec <= -1000000) {
		dst->tv_sec--;
		dst->tv_usec += 1000000;
	}
}

inline int
ts_le(timestamp_t *a, timestamp_t *b)
{
	if (a->tv_sec > b->tv_sec)
		return 0;

	if (a->tv_sec < b->tv_sec || a->tv_usec <= b->tv_usec)
		return 1;
	
	return 0;
}

inline void
update_ts(timestamp_t *dst)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	dst->tv_sec = tv.tv_sec;
	dst->tv_usec = tv.tv_usec;
}
	

float
time_diff(timestamp_t *t1, timestamp_t *t2)
{
	timestamp_t ts;
	ts_sub(&ts, t2, t1);
	return ts_to_float(&ts);
}

float
diff_now(timestamp_t *t1)
{
	timestamp_t now;
	update_ts(&now);
    return time_diff(t1, &now);
}

double
sumup(b_cnt_t l, char **unit)
{
    *unit = "B ";

	switch (get_y_unit()) {
		case Y_BYTE:
			*unit = "B  ";
			return (double) l;

		case Y_KILO:
			*unit = "KiB";
			return ((double) l / 1024);

		case Y_MEGA:
			*unit = "MiB";
			return ((double) l / 1048576);

		case Y_GIGA:
			*unit = "GiB";
			return ((double) l / 1073741824);

		case Y_TERA:
			*unit = "TiB";
			return ((double) l /1099511627776LL);

		case Y_DYNAMIC:
			if (l >= 1099511627776LL) {
				*unit = "TiB";
				return ((double) l) / 1099511627776LL;
			} else if (l >= 1073741824) {
				*unit = "GiB";
				return ((double) l) / 1073741824L;
			} else if (l >= 1048576L) {
				*unit = "MiB";
				return ((double) l) / 1048576;
			} else if (l >= 1024) {
				*unit = "KiB";
				return ((double) l) / 1024;
			} else {
				*unit = "B  ";
				return (double) l;
			}
			break;
	}
	
	return (double) l;
}

const char *
xinet_ntop(struct sockaddr *src, char *dst, socklen_t cnt)
{
	void *s;
	int family;

	if (src->sa_family == AF_INET6) {
		s = &((struct sockaddr_in6 *) src)->sin6_addr;
		family = AF_INET6;
	} else if (src->sa_family == AF_INET) {
		s = &((struct sockaddr_in *) src)->sin_addr;
		family = AF_INET;
	} else
		return NULL;

	return inet_ntop(family, s, dst, cnt);
}
