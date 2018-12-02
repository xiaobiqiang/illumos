#!/bin/ksh -p
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#

#
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

#
# Copyright (c) 2013, 2016 by Delphix. All rights reserved.
#

. $STF_SUITE/include/libtest.shlib

#
# DESCRIPTION:
#	Verify that the ZFS mdb dcmds and walkers are working as expected.
#
# STRATEGY:
#	1) Given a list of dcmds and walkers
#	2) Step through each element of the list
#	3) Verify the output by checking for "mdb:" in the output string
#

function cleanup
{
	rm -f $OUTFILE
}

verify_runnable "global"
log_onexit cleanup

tmpfile=$(mktemp)
log_must zpool scrub $TESTPOOL

typeset spa=$(mdb -ke "::spa" | awk "/$TESTPOOL/ {print \$1}")
typeset off_ub=$(mdb -ke "::offsetof spa_t spa_uberblock | =J")
typeset off_rbp=$(mdb -ke "::offsetof uberblock_t ub_rootbp | =J")
typeset bp=$(mdb -ke "$spa + $off_ub + $off_rbp =J")

# dcmds and walkers skipped due to being DEBUG only or difficult to run:
# ::zfs_params
# ::refcount

set -A dcmds "::abuf_find 1 2" \
	"::arc" \
	"::arc -b" \
	"::arc_compression_stats" \
	"$bp ::blkptr" \
	"$bp ::dva" \
	"::walk spa" \
	"::spa" \
	"$spa ::spa " \
	"$spa ::spa -c" \
	"$spa ::spa -h" \
	"$spa ::spa -v" \
	"$spa ::spa -Mmh" \
	"$spa ::spa_config" \
	"$spa ::spa_space" \
	"$spa ::spa_space -b" \
	"$spa ::spa_vdevs" \
	"$spa ::print spa_t spa_root_vdev | ::vdev" \
	"$spa ::print spa_t spa_root_vdev | ::vdev -re" \
	"$spa ::print -a spa_t spa_dsl_pool->dp_dirty_datasets | ::walk txg_list" \
	"$spa ::print -a spa_t spa_uberblock.ub_rootbp | ::blkptr" \
	"$spa ::walk metaslab" \
	"$spa ::walk metaslab | ::head -1 | ::metaslab_weight" \
	"$spa ::walk metaslab | ::head -1 | ::metaslab_trace" \
	"$spa ::walk zio_root | ::zio -c" \
	"$spa ::walk zio_root | ::zio -r" \
	"$spa ::walk zms_freelist"
	"$spa ::zfs_blkstats -v" \
	"::dbufs" \
	"::dbufs -n mos -o mdn -l 0 -b 0" \
	"::dbufs | ::dbuf" \
	"::dbuf_stats" \
	"dbuf_cache ::walk multilist"
#
# The commands above were supplied by the ZFS development team. The idea is to
# do as much checking as possible without the need to hardcode addresses.
#

log_assert "Verify that the ZFS mdb dcmds and walkers are working as expected."

typeset -i RET=0

i=0
while (( $i < ${#dcmds[*]} )); do
	log_note "Verifying: '${dcmds[i]}'"
        echo "${dcmds[i]}" | mdb -k > $OUTFILE 2>&1
	RET=$?
	if (( $RET != 0 )); then
		log_fail "mdb '${dcmds[i]}' returned error $RET"
	fi

	#
	# mdb prefixes all errors with "mdb: " so we check the output.
	#
	grep "mdb:" $OUTFILE > /dev/null 2>&1
	RET=$?
	if (( $RET == 0 )); then
		echo "mdb '${dcmds[i]}' contained 'mdb:'"
		# Using tail limits the number of lines in the log
		tail -100 $OUTFILE
		log_fail "mdb walker or dcmd failed"
	fi

        ((i = i + 1))
done

log_pass "The ZFS mdb dcmds and walkers are working as expected."
