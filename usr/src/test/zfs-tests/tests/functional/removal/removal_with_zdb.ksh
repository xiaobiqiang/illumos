#! /bin/ksh -p
#
# CDDL HEADER START
#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#
# CDDL HEADER END
#

#
# Copyright (c) 2014, 2017 by Delphix. All rights reserved.
#

. $STF_SUITE/include/libtest.shlib
. $STF_SUITE/tests/functional/removal/removal.kshlib

zdbout=${TMPDIR:-/tmp}/zdbout.$$

function cleanup
{
	default_cleanup_noexit
	log_must rm -f $zdbout
}

default_setup_noexit "$DISKS"
log_onexit cleanup

function callback
{
	if ! ksh -c "zdb -cudi $TESTPOOL >$zdbout 2>&1"; then
		log_note "Output: zdb -cudi $TESTPOOL"
		cat $zdbout
		log_fail "zdb detected errors."
	fi
	log_note "zdb -cudi $TESTPOOL >zdbout 2>&1"
	return 0
}

test_removal_with_operation callback

if ! ksh -c "zdb -cudi $TESTPOOL >$zdbout 2>&1"; then
	log_note "Output: zdb -cudi $TESTPOOL"
	cat $zdbout
	log_fail "zdb detected errors."
fi

log_pass "Can use zdb during removal"
