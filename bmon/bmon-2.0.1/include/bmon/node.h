/*
 * node.h            node handling
 *
 * $Id: node.h 1 2004-10-17 17:32:34Z tgr $
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

#ifndef __BMON_NODE_H_
#define __BMON_NODE_H_

#include <bmon/bmon.h>
#include <bmon/intf.h>

typedef struct node_s
{
	int           n_index;
	char *        n_name;
	char *        n_from;
	intf_t *      n_intf;
	size_t        n_nintf;
	int           n_selected;
} node_t;

extern node_t * lookup_node(const char *name, int creat);
extern node_t * get_local_node(void);
extern int get_nnodes(void);
extern void reset_nodes(void);
extern void remove_unused_node_intfs(void);
extern void foreach_node(void (*cb)(node_t *, void *), void *arg);
extern void foreach_intf(node_t *n, void (*cb)(intf_t *, void *), void *arg);
extern void foreach_node_intf(void (*cb)(node_t *, intf_t *, void *), void *arg);

extern node_t * get_current_node(void);
extern int first_node(void);
extern int last_node(void);
extern int prev_node(void);
extern int next_node(void);
extern intf_t * get_current_intf(void);
extern int first_intf(void);
extern int last_intf(void);
extern int next_intf(void);
extern int prev_intf(void);

#endif
