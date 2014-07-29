/*
 * config.h             Global Config
 *
 * $Id: config.h 1 2004-10-17 17:32:34Z tgr $
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

#ifndef __BMON_CONFIG_H_
#define __BMON_CONFIG_H_

#include <bmon/defs.h>
#include <bmon/compile-fixes.h>

#if STDC_HEADERS != 1
#error "*** ERROR: ANSI C headers required for compilation ***"
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <signal.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>

#include <sys/time.h>

#ifdef TIME_WITH_SYS_TIME
#include <time.h>
#endif

#if defined HAVE_STRING_H
#include <string.h>
#elif defined HAVE_STRINGS_H
#include <strings.h>
#else
#error "*** ERROR: No string header file found ***"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INITTYPES_H
#include <inittypes.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if defined HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <sys/stat.h>
#include <grp.h>
#include <errno.h>

#if defined HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#if defined HAVE_NCURSES
#  if defined HAVE_NCURSES_NCURSES_H
#    include <ncurses/ncurses.h>
#  elif defined HAVE_NCURSES_H
#    include <ncurses.h>
#  elif defined HAVE_CURSES_H
#    warn "*** WARNING: Broken system: libncurses is present but not ncurses.h, using curses.h ***"
#    include <curses.h>
#  else
#    error "*** ERROR: Broken system: libncurses present but no corresponding header file ***"
#  endif /* HAVE_*_*CURSES_H */
#  if !defined HAVE_CURSES
#    define HAVE_CURSES
#  endif /* !HAVE_CURSES */
#elif defined HAVE_CURSES
#  include <curses.h>
#else
#  error "*** ERROR: Neither ncurses nor curses is present on your system. ***"
#endif /* HAVE_[N]CURSES */

#include <netinet/in.h>

#if defined HAVE_NETINET6_IN6_H
#include <netinet6/in6.h>
#endif

#include <sys/socket.h>
#include <arpa/inet.h>


#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif


#endif
