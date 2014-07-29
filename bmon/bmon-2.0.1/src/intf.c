/*
 * intf.c             Interface Manamgement
 *
 * $Id: intf.c 23 2004-10-31 14:32:00Z tgr $
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
#include <bmon/node.h>
#include <bmon/conf.h>
#include <bmon/intf.h>
#include <bmon/input.h>
#include <bmon/utils.h>

#define DEFAULT_LIFETIME        10
#define MAX_POLICY             255
#define SECOND                 1.0f
#define MINUTE                60.0f
#define HOUR                3600.0f
#define DAY                86400.0f

static char * allowed_intf[MAX_POLICY];
static char * denied_intf[MAX_POLICY];

static inline uint8_t
attr_hash(int type)
{
	return (type & 0xFF) % ATTR_HASH_MAX;
}

void
update_attr(intf_t *i, int type, b_cnt_t rx, b_cnt_t tx, int flags)
{
	intf_attr_t *a;
	uint8_t h = attr_hash(type);
	
	for (a = i->i_attrs[h]; a; a = a->a_next)
		if (a->a_type == type)
			goto found;
	
	a = xcalloc(1, sizeof(intf_attr_t));
	a->a_type = type;
	a->a_next = i->i_attrs[h];
	i->i_attrs[h] = a;
	i->i_nattrs++;

found:
	if (flags & RX_PROVIDED) {
		if (a->a_rx != rx)
			update_ts(&a->a_updated); /* XXX: use read ts */
		a->a_rx = rx;
		a->a_rx_enabled = 1;
	}

	if (flags & TX_PROVIDED) {
		if (a->a_tx != tx)
			update_ts(&a->a_updated);
		a->a_tx = tx;
		a->a_tx_enabled = 1;
	}
}

void
foreach_attr(intf_t *i, void (*cb)(intf_attr_t *, void *), void *arg)
{
	int m;

	for (m = 0; m < ATTR_HASH_MAX; m++) {
		intf_attr_t *a;
		for (a = i->i_attrs[m]; a; a = a->a_next)
			cb(a, arg);
	}
}

const char *
type2name(int type)
{
	switch (type)
	{
		case BYTES:
			return "Bytes";
		case PACKETS:
			return "Packets";
		case ERRORS:
			return "Errors";
		case DROP:
			return "Dropped";
		case FIFO:
			return "FIFO Err";
		case FRAME:
			return "Frame Err";
		case COMPRESSED:
			return "Compressed";
		case MULTICAST:
			return "Multicast";
		case BROADCAST:
			return "Broadcast";
		case LENGTH_ERRORS:
			return "Length Err";
		case OVER_ERRORS:
			return "Over Err";
		case CRC_ERRORS:
			return "CRC Err";
		case MISSED_ERRORS:
			return "Missed Err";
		case ABORTED_ERRORS:
			return "Aborted Err";
		case CARRIER_ERRORS:
			return "Carrier Err";
		case HEARTBEAT_ERRORS:
			return "HBeat Err";
		case WINDOW_ERRORS:
			return "Window Err";
		case COLLISIONS:
			return "Collisions";
		case OVERLIMITS:
			return "Overlimits";
		case BPS:
			return "Bits/s";
		case PPS:
			return "Packets/s";
		case QLEN:
			return "Queuen Len";
		case BACKLOG:
			return "Backlog";
		case REQUEUES:
			return "Requeues";
		default:
		{
			static char str[256];
			snprintf(str, sizeof(str), "unknown (%d)", type);
			return str;
		}
	}
}

static int
match_mask(const char *mask, const char *str)
{
	int i, n;
	char c;

	if (!mask || !str)
		return 0;
	
	for (i = 0, n = 0; mask[i] != '\0'; i++, n++) {
		if (mask[i] == '*') {
			c = tolower(mask[i+1]);
			
			if (c == '\0')
				return 1; /* nothing after wildcard, matches in any case */
			
			/*look for c in str, c is the character after the wildcard */
			for (; tolower(str[n]) != c; n++)
				if (str[n] == '\0')
					return 0; /* character after wildcard was not found */
			
			n--;
		} else if (tolower(mask[i]) != tolower(str[n]))
			return 0;
	}

	return str[n] == '\0' ? 1 : 0;
}

static int
intf_allowed(const char *name)
{
	int n;
	
	if (!allowed_intf[0] && !denied_intf[0])
		return 1;

	if (!allowed_intf[0]) {
		for (n = 0; n < MAX_POLICY && denied_intf[n]; n++)
			if (match_mask(denied_intf[n], name))
				return 0;
	
		return 1;
	}

	for (n = 0; n < MAX_POLICY && denied_intf[n]; n++)
		if (match_mask(denied_intf[n], name))
			return 0;
	
	for (n=0; n < MAX_POLICY && allowed_intf[n]; n++)
		if (match_mask(allowed_intf[n], name))
			return 1;

	return 0;
}

void
intf_parse_policy(const char *policy)
{
	static int set = 0;
	int i, a = 0, d = 0, f = 0;
	char *p, *s;

	if (set)
		return;
	set = 1;
	
	s = strdup(policy);
	
	for (i = 0, p = s; ; i++) {
		if (s[i] == ',' || s[i] == '\0') {

			f = s[i] == '\0' ? 1 : 0;
			s[i] = '\0';
			
			if ('!' == *p) {
				if (d > (MAX_POLICY - 1))
					break;
				denied_intf[d++] = strdup(++p);
			} else {
				if(a > (MAX_POLICY - 1))
					break;
				allowed_intf[a++] = strdup(p);
			}
			
			if (f)
				break;
			
			p = &s[i+1];
		}
	}
	
	xfree(s);
}


intf_t *
lookup_intf(node_t *node, const char *name, uint32_t handle, int parent)
{
	int i;
	
	if (NULL == node)
		BUG();
	
	if (NULL == node->n_intf) {
		node->n_nintf = 32;
		node->n_intf = xcalloc(node->n_nintf, sizeof(intf_t));
	}
	
	for (i = 0; i < node->n_nintf; i++)
		if (!strcmp(name, node->n_intf[i].i_name) &&
			node->n_intf[i].i_handle == handle &&
			node->n_intf[i].i_parent == parent)
			return node->n_intf[i].i_updated == 0 ? &node->n_intf[i] : NULL;
	
	if (!handle && !intf_allowed(name))
		return NULL;
	
	for (i = 0; i < node->n_nintf; i++)
		if (node->n_intf[i].i_name[0] == '\0')
			break;
	
	if (i >= node->n_nintf) {
		int oldsize = node->n_nintf;
		node->n_nintf += 32;
		node->n_intf = xrealloc(node->n_intf, node->n_nintf * sizeof(intf_t));
		memset(node->n_intf + oldsize, 0, (node->n_nintf - oldsize) * sizeof(intf_t));
	}
	
	memset(&node->n_intf[i], 0, sizeof(node->n_intf[i]));
	
	strncpy(node->n_intf[i].i_name, name, sizeof(node->n_intf[i].i_name) - 1);
	node->n_intf[i].i_handle = handle;
	node->n_intf[i].i_parent = parent;
	node->n_intf[i].i_index = i;
	node->n_intf[i].i_node = node;
	node->n_intf[i].i_lifetime = DEFAULT_LIFETIME;

	return &node->n_intf[i];
}

void
foreach_child(node_t *node, intf_t *parent, void (*cb)(intf_t *, void *),
	void *arg)
{
	int i;

	for (i = 0; i < node->n_nintf; i++)
		if (node->n_intf[i].i_parent == parent->i_index &&
			node->n_intf[i].i_is_child)
			cb(&node->n_intf[i], arg);
}

void
reset_intf(intf_t *i)
{
	i->i_updated = 0;
}

void
remove_unused_intf(intf_t *i)
{
	if (--(i->i_lifetime) <= 0) {
		int m;
		for (m = 0; m < ATTR_HASH_MAX; m++) {
			intf_attr_t *a, *next;
			for (a = i->i_attrs[m]; a; a = next) {
				next = a->a_next;
				free(a);
			}
		}
		memset(i, 0, sizeof(intf_t));
	}
}

static void
calc_rate(rate_t *rate, timestamp_t *ts)
{
    float diff;

	if (0 == rate->r_prev_total) {
		rate->r_prev_total = rate->r_total;
		COPY_TS(&rate->r_last_update, ts);
		return;
	}
	
	diff = time_diff(&rate->r_last_update, ts);
	
	if (diff >= 1.0f) {
		if (rate->r_total) {

			b_cnt_t t = (rate->r_total - rate->r_prev_total);

#ifndef DISABLE_OVERFLOW_WORKAROUND
			/* HACK:
			 *
			 * Workaround for counter overflow, all input methods
			 * except kstat in 64bit mode use a 32bit counter which
			 * tends to overflow. We can work around the problem when
			 * assuming that there will be no  scenario with 4GiB/s and
			 * no more than 1 overflow per second.
			 */
			if (t >= OVERFLOW_LIMIT && !rate->r_is64bit) {
				rate->r_tps = OVERFLOW_LIMIT - (rate->r_prev_total - rate->r_total);
                rate->r_overflows++;
            } else
#endif
				rate->r_tps  = (rate_cnt_t) t;

            rate->r_tps /= diff;
            rate->r_prev_total = rate->r_total;
        }

        COPY_TS(&rate->r_last_update, ts);
    }
}

static inline b_cnt_t
get_real_total(rate_t *r, unsigned int prev_overflows, b_cnt_t prev_total)
{
	b_cnt_t res;
	unsigned int new_overflows = (r->r_overflows - prev_overflows);

#ifndef DISABLE_OVERFLOW_WORKAROUND
	if (new_overflows)
		res = (new_overflows * OVERFLOW_LIMIT) + r->r_total - prev_total;
	else
#endif
		res = r->r_total - prev_total;

	return res;
}

static inline void
update_history_data(hist_data_t *hd, rate_t *r, int index, double diff)
{
	double t = (double) get_real_total(r, hd->hd_overflows, hd->hd_prev_total) / diff;
	
	hd->hd_data[index] = (rate_cnt_t) t;
	hd->hd_prev_total = r->r_total;
	hd->hd_overflows = r->r_overflows;
}

static void
update_history_element(hist_elem_t *he, rate_t *rx, rate_t *tx, timestamp_t *ts, float unit)
{
	double diff = time_diff(&he->he_last_update, ts);

	if (0 == he->he_last_update.tv_sec) {
		he->he_rx.hd_prev_total = rx->r_total;
		he->he_tx.hd_prev_total = tx->r_total;

	/*
	 * The timing code might do shorter intervals than requested to
	 * adjust previous intervals being too long, we make sure that
	 * a history == reading interval always gets updated. The
	 * rate will be fixed according to the error.
	 */
	} else if (diff >= unit || get_read_interval() == unit) {
		update_history_data(&he->he_rx, rx, he->he_index, diff);
		update_history_data(&he->he_tx, tx, he->he_index, diff);
	} else
		return;

	if (he->he_index >= (HISTORY_SIZE - 1))
		he->he_index = 0;
	else
		he->he_index++;

	COPY_TS(&he->he_last_update, ts);
}

static inline void
update_history(history_t *hist, rate_t *rx, rate_t *tx, timestamp_t *ts)
{
	if (get_read_interval() != 1.0f)
		update_history_element(&hist->h_read, rx, tx, ts, get_read_interval());
	update_history_element(&hist->h_sec, rx, tx, ts, SECOND);
	update_history_element(&hist->h_min, rx, tx, ts, MINUTE);
	update_history_element(&hist->h_hour, rx, tx, ts, HOUR);
	update_history_element(&hist->h_day, rx, tx, ts, DAY);
}

void
notify_update(intf_t *i)
{
	i->i_updated = 1;

	calc_rate(&i->i_rx_bytes,   &rtiming.rt_last_read);
	calc_rate(&i->i_tx_bytes,   &rtiming.rt_last_read);
	calc_rate(&i->i_rx_packets, &rtiming.rt_last_read);
	calc_rate(&i->i_tx_packets, &rtiming.rt_last_read);

	update_history(&i->i_bytes_hist, &i->i_rx_bytes, &i->i_tx_bytes,
		&rtiming.rt_last_read);
	update_history(&i->i_packets_hist, &i->i_rx_packets, &i->i_tx_packets,
		&rtiming.rt_last_read);
}

void
increase_lifetime(intf_t *i, int l)
{
	i->i_lifetime += l;
}

intf_t *
get_intf(node_t *node, int index)
{
	int i;

	for (i = 0; i < node->n_nintf; i++)
		if (node->n_intf[i].i_index == index)
			return &node->n_intf[i];
	return NULL;
}
