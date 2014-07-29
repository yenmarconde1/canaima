/*
 * out_curses.c          Curses Output
 *
 * $Id: out_curses.c 18 2004-10-30 14:10:00Z tgr $
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
#include <bmon/input.h>
#include <bmon/output.h>
#include <bmon/node.h>
#include <bmon/bindings.h>
#include <bmon/utils.h>

static int initialized;
static int print_help = 0;
static int quit_mode = 0;
static int row;
static int rows;
static int cols;

static int c_graph_height = 6;
static int c_use_colors = 1;
static int c_combined_node_list = 0;
static int c_graphical_in_list = 0;
static int c_detailed_in_list = 0;
static int c_list_in_list = 1;

#define NEXT_ROW {                      \
    row++;                              \
    if (row >= rows-1)                  \
        return;                         \
    move(row,0);                        \
}

static void
putl(const char *fmt, ...)
{
    va_list args;
    char buf[2048];
    int x, y;

    getyx(stdscr, y, x);

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (strlen(buf) > cols-x)
        buf[cols-x] = '\0';
    else
        memset(&buf[strlen(buf)], ' ', cols-strlen(buf)-x);

    addstr(buf);
}

static void
curses_init(void)
{
	if (!initscr())
		exit(1);

	initialized = 1;
	
	if (!has_colors())
		c_use_colors = 0;

	if (c_use_colors) {
		int i;
		
		start_color();

#if defined HAVE_USE_DEFAULT_COLORS
		use_default_colors();
#endif
		for (i = 1; i < NR_LAYOUTS+1; i++)
			init_pair(i, layout[i].fg, layout[i].bg);
	}
		
	keypad(stdscr, TRUE);
	nonl();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);	  /* getch etc. must be non-blocking */
	clear();
	curs_set(0);
}

static void
curses_shutdown(void)
{
	if (initialized)
		endwin();
}

static void
draw_intf_list_entry(intf_t *intf, node_t *node)
{
	float rx, tx;
	char *rx_u, *tx_u;
	char pad[IFNAMSIZ + 32];
	
	rx = sumup(intf->i_rx_bytes.r_tps, &rx_u);
	tx = sumup(intf->i_tx_bytes.r_tps, &tx_u);

	memset(pad, ' ', sizeof(pad));
	pad[sizeof(pad) - 1] = '\0';
	if (intf->i_level >= 15)
		strcpy(&pad[sizeof(pad) - 1], intf->i_name);
	else
		strcpy(&pad[2 * intf->i_level], intf->i_name);

	NEXT_ROW;

	if (intf->i_index == node->n_selected && node == get_current_node()) {
		if (c_use_colors)
			attrset(COLOR_PAIR(LAYOUT_SELECTED) | layout[LAYOUT_SELECTED].attr);
		else
			attron(A_REVERSE);
	}
	
	printw("  %-3d%s", intf->i_index, intf->i_folded ? "+" : " ");

	if (intf->i_index == node->n_selected && node == get_current_node()) {
		if (c_use_colors)
			attrset(COLOR_PAIR(LAYOUT_LIST) | layout[LAYOUT_LIST].attr);
		else
			attroff(A_REVERSE);
	}

	
	putl("%-20s %10.2f%s %10d %10.2f%s %10d", pad,
		rx, rx_u, intf->i_rx_packets.r_tps,
		tx, tx_u, intf->i_tx_packets.r_tps);
}

static void
handle_child(intf_t *intf, void *arg)
{
	node_t *node = arg;

	draw_intf_list_entry(intf, node);
	foreach_child(node, intf, handle_child, node);
}


static void
draw_intf(node_t *node, intf_t *intf)
{
	if (!intf->i_name[0] || row >= (rows - 1) || intf->i_is_child)
		return;

	draw_intf_list_entry(intf, node);

	if (!intf->i_folded)
		foreach_child(node, intf, handle_child, node);
}


static void
draw_node(node_t *node, void *arg)
{
	int i;

	attrset(A_BOLD);
	NEXT_ROW; 
	putl("%s (source: %s)", node->n_name,
		node->n_from ? node->n_from : "local");
	attroff(A_BOLD);

	if (c_use_colors)
		attrset(COLOR_PAIR(LAYOUT_LIST) | layout[LAYOUT_LIST].attr);

	for (i = 0; i < node->n_nintf; i++)
		draw_intf(node, &node->n_intf[i]);

	if (c_use_colors)
		attrset(COLOR_PAIR(LAYOUT_DEFAULT) | layout[LAYOUT_DEFAULT].attr);
}

static int
lines_required_for_graphical(void)
{
	if (get_current_intf())
		return (2 * c_graph_height) + 5;
	else
		return INT_MAX;
}

static int
lines_required_for_detailed(void)
{
	if (get_current_intf())
		return 3 + ((get_current_intf()->i_nattrs + 1) / 2);
	else
		return INT_MAX;
}

static void
draw_graphic(void)
{
	int w;
	graph_t *g;
	intf_t *intf = get_current_intf();

	if (NULL == intf)
		return;
	
	g = create_configued_graph(&intf->i_bytes_hist, c_graph_height);

	NEXT_ROW;
	putl("RX    %s", g->g_rx.t_y_unit);

	for (w = (c_graph_height - 1); w >= 0; w--) {
		NEXT_ROW;
		putl(" %8.2f %s\n", g->g_rx.t_y_scale[w], (char *) g->g_rx.t_data + (w * (HISTORY_SIZE + 1)));
	}

	move(row, 71);
	putl("[%.2f%%]", rtiming.rt_variance.v_error);
	NEXT_ROW;
	putl("          1   5   10   15   20   25   30   35   40   45   50   55   60 %s", g->g_rx.t_x_unit);

	NEXT_ROW;
	putl("TX    %s", g->g_tx.t_y_unit);

	for (w = (c_graph_height - 1); w >= 0; w--) {
		NEXT_ROW;
		putl(" %8.2f %s\n", g->g_tx.t_y_scale[w], (char *) g->g_tx.t_data + (w * (HISTORY_SIZE + 1)));
	}

	move(row, 71);
	putl("[%.2f%%]", rtiming.rt_variance.v_error);
	NEXT_ROW;
	putl("          1   5   10   15   20   25   30   35   40   45   50   55   60 %s", g->g_tx.t_x_unit);

	free_graph(g);
}

static void
draw_attr_detail(intf_attr_t *attr, void *arg)
{
	int *t = (int *) arg;

	if (0 == *t) {
		NEXT_ROW;
		putl(" %-12s%11llu %11llu",
			type2name(attr->a_type), attr->a_rx, attr->a_tx);

		if (!attr->a_rx_enabled) {
			move(row, 21);
			addstr("N/A");
		}

		if (!attr->a_tx_enabled) {
			move(row, 33);
			addstr("N/A");
		}

		move(row, 44);
		*t = 1;
	} else {
		putl("%-12s%11llu %11llu",
			type2name(attr->a_type), attr->a_rx, attr->a_tx);
		
		if (!attr->a_rx_enabled) {
			move(row, 64);
			addstr("N/A");
		}
		
		if (!attr->a_tx_enabled) {
			move(row, 76);
			addstr("N/A");
		}

		move(row, 0);
		*t = 0;
	}
}

static void
draw_detailed(void)
{
	int start_pos, end_pos, i;
	intf_t *intf;
	double rx, tx;
	char *rx_u, *tx_u;
	int attr_flag = 0;

	intf = get_current_intf();

	if (NULL == intf)
		return;
	
	move(row, 40);
	addch(ACS_TTEE);
	move(row, 0);
	
#ifndef DISABLE_OVERFLOW_WORKAROUND
	if (intf->i_rx_bytes.r_overflows)
		rx = sumup((intf->i_rx_bytes.r_overflows * OVERFLOW_LIMIT) +
			intf->i_rx_bytes.r_total, &rx_u);
#endif
	else
		rx = sumup(intf->i_rx_bytes.r_total, &rx_u);

#ifndef DISABLE_OVERFLOW_WORKAROUND
	if (intf->i_tx_bytes.r_overflows)
		tx = sumup((intf->i_tx_bytes.r_overflows * OVERFLOW_LIMIT) +
			intf->i_tx_bytes.r_total, &tx_u);
#endif
	else
		tx = sumup(intf->i_tx_bytes.r_total, &tx_u);

	NEXT_ROW;
	start_pos = row;
	putl("                      RX          TX                             RX          TX");

	NEXT_ROW;
	putl(" Bytes:        %9.1f %s%8.1f %s    Packets:    %11llu %11llu",
		rx, rx_u, tx, tx_u,
		intf->i_rx_packets.r_total, intf->i_tx_packets.r_total);

	foreach_attr(intf, draw_attr_detail, &attr_flag);

	end_pos = row;
	for (i = start_pos; i <= end_pos; i++) {
		move(i, 40);
		addch(ACS_VLINE);
	}
	move(end_pos, 0);
}

static void
print_quit(void)
{
	/*
	 * This could be done with ncurses windows but i'm way too lazy to
	 * look into it
	 */
	int i, y = (rows/2) - 2;
	char *text = " Really Quit? (y/n) ";
	int len = strlen(text);
	int x = (cols/2) - (len / 2);

	attrset(A_STANDOUT);
	mvaddch(y - 2, x - 1, ACS_ULCORNER);
	mvaddch(y + 2, x - 1, ACS_LLCORNER);
	mvaddch(y - 2, x + len, ACS_URCORNER);
	mvaddch(y + 2, x + len, ACS_LRCORNER);

	for (i = 0; i < 3; i++) {
		mvaddch(y - 1 + i, x + len, ACS_VLINE);
		mvaddch(y - 1 + i, x - 1 ,ACS_VLINE);
	}

	for (i = 0; i < len; i++) {
		mvaddch(y - 2, x + i, ACS_HLINE);
		mvaddch(y - 1, x + i, ' ');
		mvaddch(y + 1, x + i, ' ');
		mvaddch(y + 2, x + i, ACS_HLINE);
	}

	mvaddstr(y, x, text);
	attroff(A_STANDOUT);
}

static void
draw_help(void)
{
#define HW 46
#define HH 19
	int i, y = (rows/2) - (HH/2);
	int x = (cols/2) - (HW/2);
	char pad[HW+1];

	memset(pad, ' ', sizeof(pad));
	pad[sizeof(pad) - 1] = '\0';

	attron(A_STANDOUT);

	for (i = 0; i < HH; i++)
		mvaddnstr(y + i, x, pad, -1);

	mvaddch(y  - 1, x - 1, ACS_ULCORNER);
	mvaddch(y + HH, x - 1, ACS_LLCORNER);
	
	mvaddch(y  - 1, x + HW, ACS_URCORNER);
	mvaddch(y + HH, x + HW, ACS_LRCORNER);

	for (i = 0; i < HH; i++) {
		mvaddch(y + i, x -  1, ACS_VLINE);
		mvaddch(y + i, x + HW, ACS_VLINE);
	}

	for (i = 0; i < HW; i++) {
		mvaddch(y  - 1, x + i, ACS_HLINE);
		mvaddch(y + HH, x + i, ACS_HLINE);
	}

	attron(A_BOLD);
	mvaddnstr(y- 1, x+15, "QUICK REFERENCE", -1);
	attron(A_UNDERLINE);
	mvaddnstr(y+ 0, x+3, "Navigation", -1);
	attroff(A_BOLD | A_UNDERLINE);

	mvaddnstr(y+ 1, x+5, "UP      Previous interface", -1);
	mvaddnstr(y+ 2, x+5, "DOWN    Next interface", -1);
	mvaddnstr(y+ 3, x+5, "LEFT    Previous node", -1);
	mvaddnstr(y+ 4, x+5, "RIGHT   Next node", -1);
	mvaddnstr(y+ 5, x+5, "?       Toggle quick reference", -1);
	mvaddnstr(y+ 6, x+5, "q       Quit bmon", -1);

	attron(A_BOLD | A_UNDERLINE);
	mvaddnstr(y+ 7, x+3, "Display Settings", -1);
	attroff(A_BOLD | A_UNDERLINE);

	mvaddnstr(y+ 8, x+5, "g       Toggle graphical statistics", -1);
	mvaddnstr(y+ 9, x+5, "d       Toggle detailed statistics", -1);
	mvaddnstr(y+10, x+5, "c       Toggle combined node list", -1);
	mvaddnstr(y+11, x+5, "l       Toggle interface list", -1);
	mvaddnstr(y+12, x+5, "f       (Un)fold sub interfaces", -1);

	attron(A_BOLD | A_UNDERLINE);
	mvaddnstr(y+13, x+3, "Measurement Units", -1);
	attroff(A_BOLD | A_UNDERLINE);

	mvaddnstr(y+14, x+5, "R       Read Interval", -1);
	mvaddnstr(y+15, x+5, "S       Seconds", -1);
	mvaddnstr(y+16, x+5, "M       Minutes", -1);
	mvaddnstr(y+17, x+5, "H       Hours", -1);
	mvaddnstr(y+18, x+5, "D       Days", -1);
	attroff(A_STANDOUT);
}

static void
print_content(void)
{
	int required_lines = 0, disable_detailed = 0, disabled_graphical = 0;

	if (NULL == get_current_node())
		return;

	if (c_list_in_list) {
		NEXT_ROW;
		putl("");

		if (c_use_colors)
			attrset(COLOR_PAIR(LAYOUT_HEADER) | layout[LAYOUT_HEADER].attr);
	
		NEXT_ROW;
		putl("  #   Tarjeta de Red Yenmar                RX Tasa         RX #   " \
			"  TX Tasax         TX #");
	
		NEXT_ROW;
		hline(ACS_HLINE, cols);

		if (c_combined_node_list)
			foreach_node(draw_node, NULL);
		else
			draw_node(get_current_node(), NULL);
	} else {
		NEXT_ROW;
		hline(ACS_HLINE, cols);
		move(row, 24);
		addstr(" Press l to enable list view ");
		move(row, 0);
	}

	/*
	 * calculate lines required for graphical and detailed stats unfolded
	 */
	if (c_graphical_in_list)
		required_lines += lines_required_for_graphical();
	else
		required_lines++;

	if (c_detailed_in_list)
		required_lines += lines_required_for_detailed();
	else
		required_lines++;

	if ((rows - row) <= (required_lines + 1)) {
		/*
		 * not enough lines, start over with detailed stats disabled
		 */
		required_lines = 0;
		disable_detailed = 1;

		/*
		 * 1 line for folded detailed stats display
		 */
		required_lines++;

		if (c_graphical_in_list)
			required_lines += lines_required_for_graphical();
		else
			required_lines++;

		if ((rows - row) <= (required_lines + 1)) {
			/*
			 * bad luck, not even enough space for graphical stats
			 * reserve 2 lines for displaying folded detailed and
			 * graphical stats
			 */
			required_lines = 2;
			disabled_graphical = 1;
		}
	}

	/*
	 * Clear out spare space
	 */
	while (row < (rows - (required_lines + 2))) {
		NEXT_ROW; putl("");
	}

	NEXT_ROW;
	hline(ACS_HLINE, cols);

	if (c_graphical_in_list) {
		if (disabled_graphical) {
			move(row, 15);
			addstr(" Increase screen size to see graphical statistics ");
			move(row, 0);
		} else
			draw_graphic();
	} else {
		move(row, 20);
		addstr(" Press g to enable graphical statistics ");
		move(row, 0);
	}

	NEXT_ROW;
	hline(ACS_HLINE, cols);

	if (c_detailed_in_list) {
		if (disable_detailed) {
			move(row, 15);
			addstr(" Increase screen size to see detailed statistics ");
			move(row, 0);
		} else
			draw_detailed();
	} else {
		move(row, 20);
		addstr(" Press d to enable detailed statistics ");
		move(row, 0);
	}
}



static void
curses_draw(void)
{
	if (NULL == get_current_node()) {
		first_node();
		first_intf();
	}

    row = 0;
    move(0,0);
	
	getmaxyx(stdscr, rows, cols);
	
	if (cols < 80) {
		clear();
		putl("Screen must be at least 80 columns wide");
		refresh();
		return;
	}

	if (c_use_colors)
		attrset(COLOR_PAIR(LAYOUT_STATUSBAR) | layout[LAYOUT_STATUSBAR].attr);
	else
		attrset(A_REVERSE);

	if (get_current_node() && get_current_intf()) {
		putl(" interface: %s at %s",
			get_current_intf()->i_name, get_current_node()->n_name);
	}

	move(row, COLS - strlen(PACKAGE_STRING) - 1);
	putl("%s", PACKAGE_STRING);
	move(row, 0);
	
	if (c_use_colors)
		attrset(COLOR_PAIR(LAYOUT_DEFAULT) | layout[LAYOUT_DEFAULT].attr);
	else
		attroff(A_REVERSE);
	
	print_content();

	if (quit_mode)
		print_quit();
	else if (print_help)
		draw_help();

	for (; row < rows-2;) {
		move(++row, 0);
		putl("");
	}
	
	row = rows-1;
	move(row, 0);

	if (c_use_colors)
		attrset(COLOR_PAIR(LAYOUT_STATUSBAR) | layout[LAYOUT_STATUSBAR].attr);
	else
		attrset(A_REVERSE);

	putl(" ^ prev interface, v next interface, <- prev node, -> next node, ? help");
	
	attrset(0);
	refresh();
}

static void
fold(void)
{
	intf_t *intf = get_current_intf();
	node_t *node = get_current_node();
	int fix = 0;

	if (NULL == intf || NULL == node)
		return;

	if (intf->i_is_child)
		fix = 1;

	while (intf->i_is_child)
		intf = get_intf(node, intf->i_parent);

	intf->i_folded = intf->i_folded ? 0 : 1;

	if (fix)
		prev_intf();
}

static int
handle_input(int ch)
{
    switch (ch) 
    {
		case 'q':
			quit_mode = quit_mode ? 0 : 1;
			return 1;

		case 0x1b:
			quit_mode = 0;
			print_help = 0;
			return 1;

		case 'y':
			if (quit_mode)
				exit(0);
			break;

		case 'n':
			if (quit_mode) {
				quit_mode = 0;
				return 1;
			}
			break;

		case 'f':
			fold();
			return 1;

		case 12:
		case KEY_CLEAR:
#ifdef HAVE_REDRAWWIN
			redrawwin(stdscr);
#endif
			clear();
			return 1;

		case 'c':
			c_combined_node_list = c_combined_node_list ? 0 : 1;
			return 1;

        case 'S':
            clear();
			set_x_unit("s", 1);
            return 1;

        case 'M':
            clear();
			set_x_unit("m", 1);
            return 1;

        case 'H':
            clear();
			set_x_unit("h", 1);
            return 1;

		case 'D':
			clear();
			set_x_unit("d", 1);
			return 1;

		case 'R':
			clear();
			set_x_unit("r", 1);
			return 1;

        case '?':
            clear();
			print_help = print_help ? 0 : 1;
			return 1;

        case 'g':
			c_graphical_in_list = c_graphical_in_list ? 0 : 1;
			return 1;

		case 'd':
			c_detailed_in_list = c_detailed_in_list ? 0 : 1;
			return 1;

		case 'l':
			c_list_in_list = c_list_in_list ? 0 : 1;
			return 1;

		case KEY_LEFT:
			prev_node();
			return 1;

		case KEY_RIGHT:
			next_node();
			return 1;

		case KEY_DOWN:
			if (next_intf() == END_OF_LIST) {
				if (next_node() != END_OF_LIST)
					first_intf();
			}
			return 1;

		case KEY_UP:
			if (prev_intf() == END_OF_LIST) {
				if (prev_node() != END_OF_LIST)
					last_intf();
			}
			return 1;
		default:
			if (get_current_intf())
				if (handle_bindings(ch, get_current_intf()->i_name))
					return 1;
			break;
    }

    return 0;
}

static void
curses_pre(void)
{
	for (;;) {
		int ch = getch();

		if (ch == -1)
			break;

		if (handle_input(ch))
			curses_draw();
	}
}

static int
curses_probe(void)
{
	/* XXX? */
	return 1;
}

static void
print_module_help(void)
{
	printf(
		"curses - Curses Output\n" \
		"\n" \
		"  Interactive curses UI. Type '?' to get some help.\n" \
		"  Author: Thomas Graf <tgraf@suug.ch>\n" \
		"\n" \
		"  Options:\n" \
		"    fgchar=CHAR    Foreground character (default: '*')\n" \
		"    bgchar=CHAR    Background character (default: '.')\n" \
		"    nchar=CHAR     Noise character (default: ':')\n" \
		"    height=NUM     Height of graph (default: 6)\n" \
		"    xunit=UNIT     X-Axis Unit (default: seconds)\n" \
		"    yunit=UNIT     Y-Axis Unit (default: dynamic)\n" \
		"    nocolors       Do not use colors\n" \
		"    cnl            Combined node list\n" \
		"    graph          Show graphical stats by default\n" \
		"    detail         Show detailed stats by default\n");
}

static void
curses_set_opts(tv_t *attrs)
{
	while (attrs) {
		if (!strcasecmp(attrs->type, "fgchar") && attrs->value)
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
		else if (!strcasecmp(attrs->type, "cnl"))
			c_combined_node_list = 1;
		else if (!strcasecmp(attrs->type, "graph"))
			c_graphical_in_list = 1;
		else if (!strcasecmp(attrs->type, "detail"))
			c_detailed_in_list = 1;
		else if (!strcasecmp(attrs->type, "nocolors"))
			c_use_colors = 0;
		else if (!strcasecmp(attrs->type, "help")) {
			print_module_help();
			exit(0);
		}
		
		attrs = attrs->next;
	}
}

static struct output_module curses_ops = {
	.om_name = "curses",
	.om_init = curses_init,
	.om_shutdown = curses_shutdown,
	.om_pre = curses_pre,
	.om_draw = curses_draw,
	.om_set_opts = curses_set_opts,
	.om_probe = curses_probe,
};

static void __init
do_curses_init(void)
{
	register_output_module(&curses_ops);
}
