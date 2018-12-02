/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * innstr.c
 * 
 * XCurses Library
 *
 * Copyright 1990, 1995 by Mortice Kern Systems Inc.  All rights reserved.
 *
 */

#if M_RCSID
#ifndef lint
static char rcsID[] = "$Header: /rd/src/libc/xcurses/rcs/innstr.c 1.1 1995/06/14 15:26:08 ant Exp $";
#endif
#endif

#include <private.h>

int
(innstr)(s, n)
char *s;
int n;
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("innstr(%p, %d)", s, n);
#endif

	code = winnstr(stdscr, s, n);

	return __m_return_code("innstr", code);
}

int
(mvinnstr)(y, x, s, n)
int y, x;
char *s;
int n;
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("mvinnstr(%d, %d, %p, %d)", y, x, s, n);
#endif

	if ((code = wmove(stdscr, y, x)) == OK)
		code = winnstr(stdscr, s, n);

	return __m_return_code("mvinnstr", code);
}

int
(mvwinnstr)(w, y, x, s, n)
WINDOW *w;
int y, x;
char *s;
int n;
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("mvwinnstr(%p, %d, %d, %p, %d)", w, y, x, s, n);
#endif

	if ((code = wmove(w, y, x)) == OK)
		code = winnstr(w, s, n);

	return __m_return_code("mvwinnstr", code);
}

int
(instr)(s)
char *s;
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("instr(%p)", s);
#endif

	code = winnstr(stdscr, s, -1);

	return __m_return_code("instr", code);
}

int
(mvinstr)(y, x, s)
int y, x;
char *s;
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("mvinstr(%d, %d, %p)", y, x, s);
#endif

	if ((code = wmove(stdscr, y, x)) == OK)
		code = winnstr(stdscr, s, -1);

	return __m_return_code("mvinstr", code);
}

int
(mvwinstr)(w, y, x, s)
WINDOW *w;
int y, x;
char *s;
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("mvwinstr(%p, %d, %d, %p)", w, y, x, s);
#endif

	if ((code = wmove(w, y, x)) == OK)
		code = winnstr(w, s, -1);

	return __m_return_code("mvwinstr", code);
}

int
(winstr)(w, s)
WINDOW *w;
char *s;
{
	int code;

#ifdef M_CURSES_TRACE
	__m_trace("winstr(%p, %p)", w, s);
#endif

	code = winnstr(w, s, -1);

	return __m_return_code("winstr", code);
}

