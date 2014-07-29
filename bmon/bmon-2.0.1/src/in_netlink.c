/*
 * in_netlink.c            rtnetlink input (Linux)
 *
 * $Id: in_netlink.c 23 2004-10-31 14:32:00Z tgr $
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
#include <bmon/input.h>
#include <bmon/node.h>
#include <bmon/intf.h>
#include <bmon/conf.h>
#include <bmon/utils.h>

#if defined HAVE_NL && defined SYS_LINUX

#define TC_H_UNSPEC	(0U)
#define TC_H_ROOT	(0xFFFFFFFFU)
#define TC_H_INGRESS    (0xFFFFFFF1U)

static int c_notc = 0;

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/link.h>
#include <netlink/qdisc.h>
#include <netlink/class.h>
#include <netlink/helpers.h>

#include <net/if.h>

static struct nl_handle nl_h = NL_INIT_HANDLE();
static struct nl_cache link_cache = RTNL_INIT_LINK_CACHE();
static struct nl_cache qdisc_cache = RTNL_INIT_QDISC_CACHE();
static struct nl_cache class_cache = RTNL_INIT_CLASS_CACHE();

struct xdata {
	intf_t *intf;
	struct rtnl_link *link;
	int level;
	int parent;
};

static void find_sub_classes(struct xdata *, int, uint32_t);
static void find_sub_qdiscs(struct xdata *, int, uint32_t);

static void
handle_class(struct nl_common *c, void *arg)
{
	struct rtnl_class *class = (struct rtnl_class *) c;
	struct xdata *x = arg;
	intf_t *intf;
	char name[IFNAMSIZ];

	snprintf(name, sizeof(name), "c:%s %s", class->tc_kind,
		nl_handle2str(class->tc_handle));

	intf = lookup_intf(get_local_node(), name, class->tc_handle,
		x->parent);

	if (NULL == intf)
		return;

	intf->i_link = x->intf->i_index;
	intf->i_is_child = 1;
	intf->i_level = x->level;
	intf->i_tx_packets.r_total = class->tc_stats.tcs_basic.packets;
	intf->i_tx_bytes.r_total = class->tc_stats.tcs_basic.bytes;
	
	update_attr(intf, DROP, 0, class->tc_stats.tcs_queue.drops, TX_PROVIDED);
	update_attr(intf, OVERLIMITS, 0, class->tc_stats.tcs_queue.overlimits, TX_PROVIDED);
	update_attr(intf, BPS, 0, class->tc_stats.tcs_rate_est.bps, TX_PROVIDED);
	update_attr(intf, PPS, 0, class->tc_stats.tcs_rate_est.pps, TX_PROVIDED);
	update_attr(intf, QLEN, 0, class->tc_stats.tcs_queue.qlen, TX_PROVIDED);
	update_attr(intf, BACKLOG, 0, class->tc_stats.tcs_queue.backlog, TX_PROVIDED);
	update_attr(intf, REQUEUES, 0, class->tc_stats.tcs_queue.requeues, TX_PROVIDED);

	notify_update(intf);
	increase_lifetime(intf, 1);

	if (class->tc_info) {
		/* Class has qdisc attached */
		find_sub_qdiscs(x, intf->i_index, class->tc_handle);
	}

	find_sub_classes(x, intf->i_index, class->tc_handle);
}

static void
find_sub_classes(struct xdata *x, int parent, uint32_t parent_handle)
{
	struct rtnl_class filter = RTNL_INIT_CLASS();

	struct xdata xn = {
		.intf = x->intf,
		.link = x->link,
		.level = x->level + 1,
		.parent = parent,
	};

	rtnl_class_set_parent(&filter, parent_handle);
	rtnl_class_set_ifindex(&filter, x->link->l_index);

	nl_cache_foreach_filter(&class_cache, (struct nl_common *) &filter,
		handle_class, &xn);
}

static void
handle_qdisc(struct nl_common *c, void *arg)
{
	struct rtnl_qdisc *qdisc = (struct rtnl_qdisc *) c;
	struct xdata *x = arg;
	intf_t *intf;
	char name[IFNAMSIZ];

	snprintf(name, sizeof(name), "q:%s %s", qdisc->tc_kind,
		nl_handle2str(qdisc->tc_handle));

	intf = lookup_intf(get_local_node(), name, qdisc->tc_handle,
		x->parent);

	if (NULL == intf)
		return;

	intf->i_link = x->intf->i_index;
	intf->i_is_child = 1;
	intf->i_level = x->level;
	if (0xffff0000 == qdisc->tc_handle) {
		intf->i_rx_packets.r_total = qdisc->tc_stats.tcs_basic.packets;
		intf->i_rx_bytes.r_total = qdisc->tc_stats.tcs_basic.bytes;
		update_attr(intf, DROP, qdisc->tc_stats.tcs_queue.drops, 0, RX_PROVIDED);
		update_attr(intf, OVERLIMITS, qdisc->tc_stats.tcs_queue.overlimits, 0, RX_PROVIDED);
		update_attr(intf, BPS, qdisc->tc_stats.tcs_rate_est.bps, 0, RX_PROVIDED);
		update_attr(intf, PPS, qdisc->tc_stats.tcs_rate_est.pps, 0, RX_PROVIDED);
		update_attr(intf, QLEN, qdisc->tc_stats.tcs_queue.qlen, 0, RX_PROVIDED);
		update_attr(intf, BACKLOG, qdisc->tc_stats.tcs_queue.backlog, 0, RX_PROVIDED);
		update_attr(intf, REQUEUES, qdisc->tc_stats.tcs_queue.requeues, 0, RX_PROVIDED);
    } else {
		intf->i_tx_packets.r_total = qdisc->tc_stats.tcs_basic.packets;
		intf->i_tx_bytes.r_total = qdisc->tc_stats.tcs_basic.bytes;
		update_attr(intf, DROP, 0, qdisc->tc_stats.tcs_queue.drops, TX_PROVIDED);
		update_attr(intf, OVERLIMITS, 0, qdisc->tc_stats.tcs_queue.overlimits, TX_PROVIDED);
		update_attr(intf, BPS, 0, qdisc->tc_stats.tcs_rate_est.bps, TX_PROVIDED);
		update_attr(intf, PPS, 0, qdisc->tc_stats.tcs_rate_est.pps, TX_PROVIDED);
		update_attr(intf, QLEN, 0, qdisc->tc_stats.tcs_queue.qlen, TX_PROVIDED);
		update_attr(intf, BACKLOG, 0, qdisc->tc_stats.tcs_queue.backlog, TX_PROVIDED);
		update_attr(intf, REQUEUES, 0, qdisc->tc_stats.tcs_queue.requeues, TX_PROVIDED);
    }

	notify_update(intf);
	increase_lifetime(intf, 1);

	find_sub_classes(x, intf->i_index, qdisc->tc_handle);
}

static void
find_sub_qdiscs(struct xdata *x, int parent, uint32_t parent_handle)
{
	struct rtnl_qdisc filter = RTNL_INIT_QDISC();

	struct xdata xn = {
		.intf = x->intf,
		.link = x->link,
		.level = x->level + 1,
		.parent = parent,
	};

	rtnl_qdisc_set_parent(&filter, parent_handle);
	rtnl_qdisc_set_ifindex(&filter, x->link->l_index);
	nl_cache_foreach_filter(&qdisc_cache, (struct nl_common *) &filter,
			handle_qdisc, &xn);
}

static void
handle_tc(intf_t *intf, struct rtnl_link *link)
{
	int adv_router_support = 1;
	struct xdata x = {
		.level = 0,
		.intf = intf,
		.link = link,
		.parent = intf->i_index,
	};

	QDISC_CACHE_IFINDEX(&qdisc_cache) = link->l_index;
	if (nl_cache_update(&nl_h, &qdisc_cache) < 0)
		adv_router_support = 0;

	CLASS_CACHE_IFINDEX(&class_cache) = link->l_index;
	if (nl_cache_update(&nl_h, &class_cache) < 0)
		adv_router_support = 0;

	if (adv_router_support) {
		find_sub_qdiscs(&x, intf->i_index, TC_H_UNSPEC);
		find_sub_qdiscs(&x, intf->i_index, TC_H_INGRESS);
	}
}

static void
do_link(struct nl_common *item, void *arg)
{
	struct rtnl_link *link = (struct rtnl_link *) item;
	struct rtnl_lstats *st;
	intf_t *intf;

	if (!link->l_name[0])
		return;

	if (get_show_only_running() && !(link->l_flags & IFF_UP))
		return;

	intf = lookup_intf(get_local_node(), link->l_name, 0, 0);

	if (NULL == intf)
		return;

	st = &link->l_stats;

	intf->i_rx_bytes.r_total   = st->ls_rx.bytes;
	intf->i_tx_bytes.r_total   = st->ls_tx.bytes;
	intf->i_rx_packets.r_total = st->ls_rx.packets;
	intf->i_tx_packets.r_total = st->ls_tx.packets;

	update_attr(intf, ERRORS, st->ls_rx.errors, st->ls_tx.errors,
		RX_PROVIDED | TX_PROVIDED);

	update_attr(intf, DROP, st->ls_rx.dropped, st->ls_tx.dropped,
		RX_PROVIDED | TX_PROVIDED);

	update_attr(intf, FIFO, st->ls_rx_fifo_errors,
		st->ls_tx_fifo_errors, RX_PROVIDED | TX_PROVIDED);

	update_attr(intf, COMPRESSED, st->ls_rx.compressed,
		st->ls_tx.compressed, RX_PROVIDED | TX_PROVIDED);

	update_attr(intf, MULTICAST, st->ls_rx.multicast, 0, RX_PROVIDED);
	update_attr(intf, COLLISIONS, 0, st->ls_tx_collisions, TX_PROVIDED);
	update_attr(intf, LENGTH_ERRORS, st->ls_rx_length_errors, 0, RX_PROVIDED);
	update_attr(intf, OVER_ERRORS, st->ls_rx_over_errors, 0, RX_PROVIDED);
	update_attr(intf, CRC_ERRORS, st->ls_rx_crc_errors, 0, RX_PROVIDED);
	update_attr(intf, FRAME, st->ls_rx_frame_errors, 0, RX_PROVIDED);
	update_attr(intf, MISSED_ERRORS, st->ls_rx_missed_errors, 0, RX_PROVIDED);
	update_attr(intf, ABORTED_ERRORS, 0, st->ls_tx_aborted_errors, TX_PROVIDED);
	update_attr(intf, HEARTBEAT_ERRORS, 0, st->ls_tx_heartbeat_errors,
		TX_PROVIDED);
	update_attr(intf, WINDOW_ERRORS, 0, st->ls_tx_window_errors, TX_PROVIDED);
	update_attr(intf, CARRIER_ERRORS, 0, st->ls_tx_carrier_errors, TX_PROVIDED);

	if (!c_notc)
		handle_tc(intf, link);
	
	notify_update(intf);
	increase_lifetime(intf, 1);
}

static void
netlink_read(void)
{
	if (nl_cache_update(&nl_h, &link_cache) < 0)
		quit("%s\n", nl_geterror());

	nl_cache_foreach(&link_cache, do_link, NULL);
}

static void
netlink_shutdown(void)
{
	nl_close(&nl_h);
}

static void
netlink_do_init(void)
{
	if (nl_connect(&nl_h, NETLINK_ROUTE) < 0)
		quit("%s\n", nl_geterror());

	nl_use_default_handlers(&nl_h);
}

static int
netlink_probe(void)
{
	if (nl_connect(&nl_h, NETLINK_ROUTE) < 0)
		return 0;
	
	if (nl_cache_update(&nl_h, &link_cache) < 0) {
		nl_close(&nl_h);
		return 0;
	}
	
	nl_close(&nl_h);
	return 1;
}

static void
print_help(void)
{
	printf(
		"netlink - Netlink statistic collector for Linux\n" \
		"\n" \
		"  Powerful statistic collector for Linux using netlink sockets\n" \
		"  to collect link and traffic control statistics.\n" \
		"  Author: Thomas Graf <tgraf@suug.ch>\n" \
		"\n" \
		"  Options:\n" \
		"    notc           Do not collect traffic control statistics\n");
}

static void
netlink_set_opts(tv_t *attrs)
{
	while (attrs) {
		if (!strcasecmp(attrs->type, "notc"))
			c_notc = 1;
		else if (!strcasecmp(attrs->type, "help")) {
			print_help();
			exit(0);
		}
		attrs = attrs->next;
	}
}
	

static struct input_module netlink_ops = {
	.im_name     = "netlink",
	.im_read     = netlink_read,
	.im_shutdown = netlink_shutdown,
	.im_set_opts = netlink_set_opts,
	.im_probe = netlink_probe,
	.im_init = netlink_do_init,
};

static void __init
netlink_init(void)
{
	register_input_module(&netlink_ops);
}


#endif
