/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright 2016 Joyent, Inc.
 */

/*
 * Test bmi1 related instructions
 */

.text
.align 16
.globl libdis_test
.type libdis_test, @function
libdis_test:
	andn	%eax, %ebx, %edx
	andn	(%eax), %ebx, %edx
	andn	0x40(%eax), %ebx, %edx
	bextr	%ebx, %eax, %edx
	bextr	%ebx, (%eax), %edx
	bextr	%ebx, 0x40(%eax), %edx
	blsi	%eax, %edx
	blsi	(%eax), %edx
	blsi	0x40(%eax), %edx
	blsmsk	%eax, %edx
	blsmsk	(%eax), %edx
	blsmsk	0x40(%eax), %edx
	blsr	%eax, %edx
	blsr	(%eax), %edx
	blsr	0x40(%eax), %edx
	tzcnt	%ax, %dx
	tzcnt	(%eax), %dx
	tzcnt	0x40(%eax), %dx
	tzcnt	%eax, %edx
	tzcnt	(%eax), %edx
	tzcnt	0x40(%eax), %edx
.size libdis_test, [.-libdis_test]
