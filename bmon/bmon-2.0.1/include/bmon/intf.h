/*
 * intf.h             Interface Management
 *
 * $Id: intf.h 17 2004-10-30 12:57:18Z tgr $
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

#ifndef __BMON_INTF_H_
#define __BMON_INTF_H_

#include <bmon/bmon.h>

#define OVERFLOW_LIMIT  4294967296ULL

#define HISTORY_SIZE 60

#define IFNAME_MAX 32
#define ATTR_HASH_MAX 32

typedef struct hist_data_s
{
	rate_cnt_t      hd_data[HISTORY_SIZE];
	b_cnt_t         hd_prev_total;
	unsigned int    hd_overflows;
} hist_data_t;

typedef struct hist_elem_s
{
	hist_data_t     he_rx;
	hist_data_t     he_tx;
	int             he_index;
	timestamp_t     he_last_update;
} hist_elem_t;

typedef struct history_s
{
	hist_elem_t     h_read;
	hist_elem_t     h_sec;
	hist_elem_t     h_min;
	hist_elem_t     h_hour;
	hist_elem_t     h_day;
} history_t;

typedef struct rate_s
{
	b_cnt_t         r_total;
	b_cnt_t         r_prev_total;
	rate_cnt_t      r_tps; 
	int             r_overflows;
	int             r_is64bit;
	timestamp_t     r_last_update;
} rate_t;

#define RX_PROVIDED 1
#define TX_PROVIDED 2

typedef struct intf_attr_s
{
	int           a_type;
	int           a_rx_enabled;
	int           a_tx_enabled;
	b_cnt_t       a_rx;
	b_cnt_t       a_tx;
	timestamp_t   a_last_distribution;
	timestamp_t   a_updated;
	struct intf_attr_s *a_next;
} intf_attr_t;

struct node_s;


typedef struct intf_s
{
	char            i_name[IFNAME_MAX];
	uint32_t        i_handle;
	struct node_s * i_node;
	int             i_index;
	int             i_parent;
	int             i_level;
	int             i_folded;
	int             i_link;
	int             i_is_child;
	int             i_nattrs; 
	intf_attr_t *   i_attrs[ATTR_HASH_MAX];
	rate_t          i_rx_bytes;
	rate_t          i_tx_bytes;
	history_t       i_bytes_hist;
	rate_t          i_rx_packets;
	rate_t          i_tx_packets;
	history_t       i_packets_hist;
	int             i_updated;
	int             i_lifetime;

} intf_t;



 /**
  * Attribute Types
  */
enum {
	BYTES,
	PACKETS,
	ERRORS,
	DROP,
	FIFO,
	FRAME,
	COMPRESSED,
	MULTICAST,
	BROADCAST,
	LENGTH_ERRORS,
	OVER_ERRORS,
	CRC_ERRORS,
	MISSED_ERRORS,
	ABORTED_ERRORS,
	CARRIER_ERRORS,
	HEARTBEAT_ERRORS,
	WINDOW_ERRORS,
	COLLISIONS,
	OVERLIMITS,
	BPS,
	PPS,
	QLEN,
	BACKLOG,
	REQUEUES,
	__ATTR_MAX,
};

#define ATTR_MAX (__ATTR_MAX - 1)
	
extern intf_t * lookup_intf(struct node_s *node, const char *name, uint32_t handle, int parent);
extern void foreach_child(struct node_s *node, intf_t *parent, void (*cb)(intf_t *, void *), void *arg);
extern void notify_update(intf_t *i);
extern void increase_lifetime(intf_t *i, int l);
extern void reset_intf(intf_t *i);
extern void remove_unused_intf(intf_t *i);

extern void update_attr(intf_t *i, int type, b_cnt_t rx, b_cnt_t tx, int flags);
extern void intf_parse_policy(const char *policy);
extern void foreach_attr(intf_t *i, void (*cb)(intf_attr_t *, void *), void *arg);
extern const char * type2name(int type);
extern intf_t * get_intf(struct node_s *node, int ifindex);

#endif
