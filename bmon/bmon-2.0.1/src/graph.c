/*
 * graph.c             Graph creation utility
 *
 * $Id: graph.c 1 2004-10-17 17:32:34Z tgr $
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
#include <bmon/input.h>
#include <bmon/conf.h>
#include <bmon/intf.h>
#include <bmon/conf.h>
#include <bmon/utils.h>

static b_cnt_t
get_divisor(rate_cnt_t hint, char **unit)
{
	switch (get_y_unit()) {
		case Y_BYTE:
			*unit = "B  ";
			return 1;

		case Y_KILO:
			*unit = "Kib";
			return 1024;

		case Y_MEGA:
			*unit = "MiB";
			return 1048576;

		case Y_GIGA:
			*unit = "GiB";
			return 1073741824;

		case Y_TERA:
			*unit = "TiB";
			return 1099511627776LL;

		case Y_DYNAMIC:
			if (hint >= 1073741824) {
				*unit = "GiB";
				return 1073741824;
			} else if (hint >= 1048576) {
				*unit = "MiB";
				return 1048576;
			} else if (hint >= 1024) {
				*unit = "KiB";
				return 1024;
			} else {
				*unit = "B  ";
				return 1;
			}
	}

	*unit = "B  ";
	return 1;
}

static void
put_col(table_t *t, int data_idx, hist_data_t *src, int hist_idx, rate_cnt_t half_step)
{
	int i;
	char *col = D_AT_COL(t->t_data, data_idx);

	rate_cnt_t tot = src->hd_data[hist_idx];

	if (tot) {
		*(D_AT_ROW(col, 0)) = ':';
		
		for (i = 0; i < t->t_height; i++)
			if (tot >= (t->t_y_scale[i] - half_step))
				*(D_AT_ROW(col, i)) = get_fg_char();
	}
}

static void
create_table(table_t *t, hist_data_t *src, int index, int height)
{
	int i, di;
	size_t dsize = height * (HISTORY_SIZE + 1);
	rate_cnt_t max = 0, half_step, step;
	
	t->t_index = index;
	t->t_height = height;
	t->t_y_scale = xcalloc(height, sizeof(double));
	t->t_data = xcalloc(dsize, sizeof(char));

	memset(t->t_data, get_bg_char(), dsize);

	for (i = 0; i < height; i++)
		*(D_AT_COL(D_AT_ROW(t->t_data, i), HISTORY_SIZE)) = '\0';

	for (i = 0; i < HISTORY_SIZE; i++)
		if (max < src->hd_data[i])
			max = src->hd_data[i];

	step = max / height;
	half_step = step / 2;

	for (i = 0; i < height; i++)
		t->t_y_scale[i] = (i + 1) * step;

	for (di = 0, i = (index - 1); i >= 0; di++, i--)
		put_col(t, di, src, i, half_step);

	for (i = (HISTORY_SIZE - 1); di < HISTORY_SIZE; di++, i--)
		put_col(t, di, src, i, half_step);

	{
		b_cnt_t div;
		int h = (height / 3) * 2;

		if (h >= height)
			h = (height - 1);
		
		div = get_divisor(t->t_y_scale[h], &t->t_y_unit);
		
		for (i = 0; i < height; i++)
			t->t_y_scale[i] /= (double) div;
	}
}

graph_t *
create_graph(hist_elem_t *src, int height)
{
	graph_t *g;

	g = xcalloc(1, sizeof(graph_t));

	create_table(&g->g_rx, &src->he_rx, src->he_index, height);
	create_table(&g->g_tx, &src->he_tx, src->he_index, height);

	return g;
}

graph_t *
create_configued_graph(history_t *src, int height)
{
	graph_t *g;
	hist_elem_t *e = NULL;
	char *u = "s";
	int h = 0;

	switch (get_x_unit()) {
		case X_SEC:  u = "s"; e = &src->h_sec; break;
		case X_MIN:  u = "m"; e = &src->h_min; break;
		case X_HOUR: u = "h"; e = &src->h_hour; break;
		case X_DAY:  u = "d"; e = &src->h_day; break;
		case X_READ: {
			if (get_read_interval() != 1.0f) {
				char buf[32];
				float ri = get_read_interval();

				snprintf(buf, sizeof(buf), "(%.2fs)", ri);
				u = strdup(buf);
				h = 1;
				e = &src->h_read;
			} else {
				u = "s";
				e = &src->h_sec;
			}
		}
		break;
	}

	if (NULL == e)
		BUG();

	g = create_graph(e, height);

	if (h)
		g->g_flags |= GRAPH_HAS_FREEABLE_X_UNIT;

	g->g_rx.t_x_unit = u;
	g->g_tx.t_x_unit = u;

	return g;
}

static void
free_table(table_t *t)
{
	xfree(t->t_y_scale);
	xfree(t->t_data);
}

void
free_graph(graph_t *g)
{
	if (g->g_flags & GRAPH_HAS_FREEABLE_X_UNIT)
		xfree(g->g_rx.t_x_unit);
		
	free_table(&g->g_rx);
	free_table(&g->g_tx);
	xfree(g);
}
