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
 * Test PCLMULQDQ related instructions
 */

.text
.align 16
.globl libdis_test
.type libdis_test, @function
libdis_test:
	pclmulqdq	$0x2, %xmm0, %xmm1
	pclmulqdq	$0x2, (%eax), %xmm2
	pclmulqdq	$0x2, 0x10(%ebx), %xmm3

	pclmullqlqdq	%xmm0, %xmm1
	pclmullqlqdq	(%eax), %xmm2
	pclmullqlqdq	0x4(%ebx), %xmm2

	pclmulhqlqdq	%xmm0, %xmm1
	pclmulhqlqdq	(%eax), %xmm2
	pclmulhqlqdq	0x4(%ebx), %xmm2

	pclmullqhqdq	%xmm0, %xmm1
	pclmullqhqdq	(%eax), %xmm2
	pclmullqhqdq	0x4(%ebx), %xmm2

	pclmulhqhqdq	%xmm0, %xmm1
	pclmulhqhqdq	(%eax), %xmm2
	pclmulhqhqdq	0x4(%ebx), %xmm2
.size libdis_test, [.-libdis_test]
