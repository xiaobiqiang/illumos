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
 * Copyright 1998 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1983, 1984, 1985, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved	*/

/*
 * Portions of this source code were derived from Berkeley 4.3 BSD
 * under license from the Regents of the University of California.
 */

#ifndef	_RPCSVC_YPUPD_H
#define	_RPCSVC_YPUPD_H

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Compiled from ypupdate_prot.x using rpcgen
 * This is NOT source code!
 * DO NOT EDIT THIS FILE!
 */
#define	MAXMAPNAMELEN 255
#define	MAXYPDATALEN 1023
#define	MAXERRMSGLEN 255

#define	YPU_PROG ((ulong_t)100028)
#define	YPU_VERS ((ulong_t)1)
#define	YPU_CHANGE ((ulong_t)1)
extern uint_t *ypu_change_1();
#define	YPU_INSERT ((ulong_t)2)
extern uint_t *ypu_insert_1();
#define	YPU_DELETE ((ulong_t)3)
extern uint_t *ypu_delete_1();
#define	YPU_STORE ((ulong_t)4)
extern uint_t *ypu_store_1();

typedef struct {
	uint_t yp_buf_len;
	char *yp_buf_val;
} yp_buf;
bool_t xdr_yp_buf();

struct ypupdate_args {
	char *mapname;
	yp_buf key;
	yp_buf datum;
};
typedef struct ypupdate_args ypupdate_args;
bool_t xdr_ypupdate_args();

struct ypdelete_args {
	char *mapname;
	yp_buf key;
};
typedef struct ypdelete_args ypdelete_args;
bool_t xdr_ypdelete_args();

#ifdef	__cplusplus
}
#endif

#endif	/* _RPCSVC_YPUPD_H */
