/*
 * out_ascii.c           ASCII Output
 *
 * $Id: out_ascii.c 1 2004-10-17 17:32:34Z tgr $
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
#include <bmon/graph.h>
#include <bmon/conf.h>
#include <bmon/output.h>
#include <bmon/node.h>
#include <bmon/utils.h>

typedef enum diagram_type_e {
	D_LIST,
	D_GRAPH,
	D_DETAILS,
} diagram_type_t;


static int c_print_header = INT_MAX;
static diagram_type_t c_diagram_type = D_LIST;
static int c_graph_height = 6;
static int c_quit_after = -1;

static inline int
get_print_header(void)
{
	static int hdr_rem;

	if (c_print_header) {
		if (0 == hdr_rem) {
			hdr_rem = (c_print_header - 1);
			return 1;
		}

		if (c_print_header != INT_MAX)
			hdr_rem--;
	}

	return 0;
}

static inline void
set_diagram_type(const char *t)
{
	if (tolower(*t) == 'l')
		c_diagram_type = D_LIST;
	else if (tolower(*t) == 'g')
		c_diagram_type = D_GRAPH;
	else if (tolower(*t) == 'd')
		c_diagram_type = D_DETAILS;
	else
		quit("Unknown diagram type '%s'\n", t);
}


static void
print_intf(intf_t *i)
{
	double rx, tx;
	char *rx_u, *tx_u;
	char pad[IFNAMSIZ + 32];
	
	if (get_print_header())
		printf("Interface                   RX Rate        RX #    TX Rate        TX #\n");

	rx = sumup(i->i_rx_bytes.r_tps, &rx_u);
	tx = sumup(i->i_tx_bytes.r_tps, &tx_u);

	memset(pad, ' ', sizeof(pad));
	pad[sizeof(pad) - 1] = '\0';
	if (i->i_level >= 15)
		strcpy(&pad[sizeof(pad) - 1], i->i_name);
	else
		strcpy(&pad[2 * i->i_level], i->i_name);

	printf("%-24s%10.2f%s%10.1f%10.2f%s%10.1f\n", pad,
		rx, rx_u, (float) i->i_rx_packets.r_tps,
		tx, tx_u, (float) i->i_tx_packets.r_tps);
}


static void
handle_child(intf_t *i, void *arg)
{
	node_t *node = arg;

	print_intf(i);
	foreach_child(node, i, handle_child, node);
}

static void
print_list(intf_t *i, node_t *node)
{
	if (i->i_is_child)
		return;
	
	print_intf(i);
	foreach_child(node, i, handle_child, node);
}

static void
print_attr_detail(intf_attr_t *a, void *arg)
{
	printf("  %-14s %12llu     %12llu\n",
		type2name(a->a_type), a->a_rx, a->a_tx);
}

static void
print_details(intf_t *i)
{
	double rx, tx;
	char *rx_u, *tx_u;
	
	if (i->i_handle)
		printf(" %s (%u)\n", i->i_name, i->i_handle);
	else
		printf(" %s\n", i->i_name);

	rx = sumup(i->i_rx_bytes.r_total, &rx_u);
	tx = sumup(i->i_tx_bytes.r_total, &tx_u);

	printf("  Bytes:         %12.2f %s %12.2f %s\n",
		rx, rx_u, tx, tx_u);
	printf("  Packets:       %12llu     %12llu\n",
		i->i_rx_packets.r_total, i->i_tx_packets.r_total);

	foreach_attr(i, print_attr_detail, NULL);
}

static void
print_graph(intf_t *i)
{
	int w;

	graph_t *g = create_configued_graph(&i->i_bytes_hist, c_graph_height);

	printf("%s\n", i->i_name);

	printf("RX   %s\n", g->g_rx.t_y_unit);
	
	for (w = (c_graph_height - 1); w >= 0; w--)
		printf("%8.2f %s\n", g->g_rx.t_y_scale[w], (char *) g->g_rx.t_data + (w * (HISTORY_SIZE + 1)));
	
	printf("         1   5   10   15   20   25   30   35   40   " \
		"45   50   55   60 %s\n", g->g_rx.t_x_unit);

	printf("TX   %s\n", g->g_tx.t_y_unit);
	
	for (w = (c_graph_height - 1); w >= 0; w--)
		printf("%8.2f %s\n", g->g_tx.t_y_scale[w], (char *) g->g_tx.t_data + (w * (HISTORY_SIZE + 1)));
	
	printf("         1   5   10   15   20   25   30   35   40   " \
		"45   50   55   60 %s\n", g->g_tx.t_x_unit);

	free_graph(g);
}

static void
ascii_draw_intf(intf_t *i, void *arg)
{
	node_t *node = arg;

	switch (c_diagram_type) {
		case D_LIST:
			print_list(i, node);
			break;

		case D_DETAILS:
			print_details(i);
			break;

		case D_GRAPH:
			print_graph(i);
			break;
	}

}

static void
ascii_draw_node(node_t *n, void *arg)
{

	if (n->n_name) {

		if (get_nnodes() > 1)
			printf("%s:\n", n->n_name);
		foreach_intf(n, ascii_draw_intf, n);
	}
}

static void
ascii_draw(void)
{
	foreach_node(ascii_draw_node, NULL);

	if (c_quit_after > 0)
		if (--c_quit_after == 0)
			exit(0);
}

static int
ascii_probe(void)
{
	return 1;
}

static void
print_help(void)
{
	printf(
		"ascii - ASCII Output\n" \
		"\n" \
		"  Plain configurable ASCII output.\n" \
		"\n" \
		"  vmstat like: (print header every 10 lines)\n" \
		"      bmon -p eth0 -o ascii:header=10\n" \
		"  scriptable: (output graph for eth1 10 times)\n" \
		"      bmon -p eth1 -o 'ascii:diagram=graph;quitafter=10'\n" \
		"  show details for all ethernet interfaces:\n" \
		"      bmon -p 'eth*' -o 'ascii:diagram=details;quitafter=1'\n" \
		"\n" \
		"  Author: Thomas Graf <tgraf@suug.ch>\n" \
		"\n" \
		"  Options:\n" \
		"    diagram=TYPE   Diagram type\n" \
		"    fgchar=CHAR    Foreground character (default: '*')\n" \
		"    bgchar=CHAR    Background character (default: '.')\n" \
		"    nchar=CHAR     Noise character (default: ':')\n" \
		"    height=NUM     Height of graph (default: 6)\n" \
		"    xunit=UNIT     X-Axis Unit (default: seconds)\n" \
		"    yunit=UNIT     Y-Axis Unit (default: dynamic)\n" \
		"    header[=NUM]   Print header every NUM lines\n" \
		"    noheader       Do not print a header\n" \
		"    quitafter=NUM  Quit bmon after NUM outputs\n");
}

static void
ascii_set_opts(tv_t *attrs)
{
	while (attrs) {
		if (!strcasecmp(attrs->type, "diagram") && attrs->value)
			set_diagram_type(attrs->value);
		else if (!strcasecmp(attrs->type, "fgchar") && attrs->value)
			set_fg_char(attrs->value[0]);
		else if (!strcasecmp(attrs->type, "bgchar") && attrs->value)
			set_bg_char(attrs->value[0]);
		else if (!strcasecmp(attrs->type, "nchar") && attrs->value)
			set_noise_char(attrs->value[0]);
		else if (!strcasecmp(attrs->type, "xunit") && attrs->value)
			set_x_unit(attrs->value, 1);
		else if (!strcasecmp(attrs->type, "yunit") && attrs->value)
			set_y_unit(attrs->value);
		else if (!strcasecmp(attrs->type, "height") && attrs->value)
			c_graph_height = strtol(attrs->value, NULL, 0);
		else if (!strcasecmp(attrs->type, "header")) {
			if (attrs->value)
				c_print_header = strtol(attrs->value, NULL, 0);
			else
				c_print_header = INT_MAX;
		} else if (!strcasecmp(attrs->type, "noheader"))
			c_print_header = 0;
		else if (!strcasecmp(attrs->type, "quitafter") && attrs->value)
			c_quit_after = strtol(attrs->value, NULL, 0);
		else if (!strcasecmp(attrs->type, "help")) {
			print_help();
			exit(0);
		}
		
		attrs = attrs->next;
	}
}

static struct output_module ascii_ops = {
	.om_name = "ascii",
	.om_draw = ascii_draw,
	.om_probe = ascii_probe,
	.om_set_opts = ascii_set_opts,
};

static void __init
ascii_init(void)
{
	register_output_module(&ascii_ops);
}


