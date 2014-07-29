/*
 * conf.h                         Config Crap
 *
 * $Id: conf.h 13 2004-10-27 20:57:00Z tgr $
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

#ifndef __BMON_CONF_H_
#define __BMON_CONF_H_

#include <bmon/bmon.h>

typedef enum x_unit_e {
	X_SEC,
	X_MIN,
	X_HOUR,
	X_DAY,
	X_READ,
} x_unit_t;

typedef enum y_unit_e {
	Y_DYNAMIC,
	Y_BYTE,
	Y_KILO,
	Y_MEGA,
	Y_GIGA,
	Y_TERA,
} y_unit_t;

enum {
	LAYOUT_UNSPEC,
	LAYOUT_DEFAULT,
	LAYOUT_STATUSBAR,
	LAYOUT_HEADER,
	LAYOUT_LIST,
	LAYOUT_SELECTED,
	__LAYOUT_MAX,
};

#define NR_LAYOUTS (__LAYOUT_MAX - 1)

typedef struct layout_s
{
	int fg;
	int bg;
	int attr;
} layout_t;

extern layout_t layout[];

typedef struct tv_s
{
	char *type;
	char *value;
	struct tv_s *next;
} tv_t;

typedef struct module_conf_s
{
	char *name;
	tv_t *attrs;
	struct module_conf_s *next;
} module_conf_t;

extern void read_configfile(void);
extern module_conf_t * parse_module_param(const char *data);

extern inline void set_read_interval(const char *i);
extern inline float get_read_interval(void);
extern inline void get_read_interval_as_ts(timestamp_t *ts);
extern inline void set_configfile(const char *file);
extern inline void set_x_unit(const char *x, int force);
extern inline x_unit_t get_x_unit(void);
extern inline void set_y_unit(const char *y);
extern inline y_unit_t get_y_unit(void);
extern inline void set_fg_char(char c);
extern inline char get_fg_char(void);
extern inline void set_bg_char(char c);
extern inline char get_bg_char(void);
extern inline char get_noise_char(void);
extern inline void set_noise_char(char c);
extern inline void set_sleep_time(const char *s);
extern inline unsigned long get_sleep_time(void);
extern inline void set_signal_output(int i);
extern inline int get_signal_output(void);
extern inline void set_show_only_running(int i);
extern inline int get_show_only_running(void);

#endif
