#!/usr/sbin/dtrace -s
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
 * Copyright 2018 Nexenta Systems, Inc.  All rights reserved.
 */

/*
 * User-level dtrace for the smbd authentication service
 * Usage: dtrace -s smbd-authsvc.d -p `pgrep smbd`
 */

#pragma D option flowindent

self int trace;
self int mask;

/*
 * The smbd_authsvc_work() function is a good place to start tracing
 * to watch authentication.  This function executes all the actions
 * associated with a single session setup conversation (even though
 * that conversation will usually involve multiple SMB requests).
 */
pid$target:*smbd:smbd_authsvc_work:entry
{
	self->trace++;
}

/*
 * If traced and not masked, print entry/return
 */
pid$target:*smbd::entry,
pid$target:libmlsvc.so.1::entry,
pid$target:libmlrpc.so.2::entry,
pid$target:libsmbns.so.1::entry,
pid$target:libsmb.so.1::entry,
pid$target:libsmbfs.so.1::entry
/self->trace > 0 && self->mask == 0/
{
	printf("\t0x%x", arg0);
	printf("\t0x%x", arg1);
	printf("\t0x%x", arg2);
	printf("\t0x%x", arg3);
	printf("\t0x%x", arg4);
	printf("\t0x%x", arg5);
}

/*
 * Mask (don't print) all function calls below these functions.
 * These make many boring, repetitive function calls like
 * smb_mbtowc, smb_msgbuf_has_space, ...
 *
 * Also, libmlrpc has rather deep call stacks, particularly under
 * ndr_encode_decode_common(), so this stops traces below there.
 * Remove that from the mask actions to see the details.
 */
pid$target::ndr_encode_decode_common:entry,
pid$target::smb_msgbuf_decode:entry,
pid$target::smb_msgbuf_encode:entry,
pid$target::smb_strlwr:entry,
pid$target::smb_strupr:entry,
pid$target::smb_wcequiv_strlen:entry
{
	self->mask++;
}

/*
 * Now inverses of above, unwind order.
 */

pid$target::ndr_encode_decode_common:return,
pid$target::smb_msgbuf_decode:return,
pid$target::smb_msgbuf_encode:return,
pid$target::smb_strlwr:return,
pid$target::smb_strupr:return,
pid$target::smb_wcequiv_strlen:return
{
	self->mask--;
}

pid$target:*smbd::return,
pid$target:libmlsvc.so.1::return,
pid$target:libmlrpc.so.2::return,
pid$target:libsmbns.so.1::return,
pid$target:libsmb.so.1::return,
pid$target:libsmbfs.so.1::return
/self->trace > 0 && self->mask == 0/
{
	printf("\t0x%x", arg1);
}

pid$target:*smbd:smbd_authsvc_work:return
{
	self->trace--;
}
