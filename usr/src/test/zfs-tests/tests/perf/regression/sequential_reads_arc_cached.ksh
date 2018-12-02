#!/usr/bin/ksh

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

#
# Copyright (c) 2015, 2016 by Delphix. All rights reserved.
#

#
# Description:
# Trigger fio runs using the sequential_reads job file. The number of runs and
# data collected is determined by the PERF_* variables. See do_fio_run for
# details about these variables.
#
# The files to read from are created prior to the first fio run, and used
# for all fio runs. The ARC is not cleared to ensure that all data is cached.
#

. $STF_SUITE/include/libtest.shlib
. $STF_SUITE/tests/perf/perf.shlib

function cleanup
{
	recreate_perf_pool
}

log_onexit cleanup

recreate_perf_pool
populate_perf_filesystems

# Make sure the working set can be cached in the arc. Aim for 1/2 of arc.
export TOTAL_SIZE=$(($(get_max_arc_size) / 2))

# Variables for use by fio.
if [[ -n $PERF_REGRESSION_WEEKLY ]]; then
	export PERF_RUNTIME=${PERF_RUNTIME:-$PERF_RUNTIME_WEEKLY}
	export PERF_RUNTYPE=${PERF_RUNTYPE:-'weekly'}
	export PERF_NTHREADS=${PERF_NTHREADS:-'8 16 32 64'}
	export PERF_NTHREADS_PER_FS=${PERF_NTHREADS_PER_FS:-'0'}
	export PERF_SYNC_TYPES=${PERF_SYNC_TYPES:-'1'}
	export PERF_IOSIZES=${PERF_IOSIZES:-'8k 64k 128k'}
elif [[ -n $PERF_REGRESSION_NIGHTLY ]]; then
	export PERF_RUNTIME=${PERF_RUNTIME:-$PERF_RUNTIME_NIGHTLY}
	export PERF_RUNTYPE=${PERF_RUNTYPE:-'nightly'}
	export PERF_NTHREADS=${PERF_NTHREADS:-'64 128'}
	export PERF_NTHREADS_PER_FS=${PERF_NTHREADS_PER_FS:-'0'}
	export PERF_SYNC_TYPES=${PERF_SYNC_TYPES:-'1'}
	export PERF_IOSIZES=${PERF_IOSIZES:-'128k 1m'}
fi

# Layout the files to be used by the read tests. Create as many files as the
# largest number of threads. An fio run with fewer threads will use a subset
# of the available files.
export NUMJOBS=$(get_max $PERF_NTHREADS)
export FILE_SIZE=$((TOTAL_SIZE / NUMJOBS))
export DIRECTORY=$(get_directory)
log_must fio $FIO_SCRIPTS/mkfiles.fio

# Set up the scripts and output files that will log performance data.
lun_list=$(pool_to_lun_list $PERFPOOL)
log_note "Collecting backend IO stats with lun list $lun_list"
export collect_scripts=(
    "kstat zfs:0 1"  "kstat"
    "vmstat -T d 1"       "vmstat"
    "mpstat -T d 1"       "mpstat"
    "iostat -T d -xcnz 1" "iostat"
    "dtrace -Cs $PERF_SCRIPTS/io.d $PERFPOOL $lun_list 1" "io"
    "dtrace -Cs $PERF_SCRIPTS/prefetch_io.d $PERFPOOL 1"  "prefetch"
    "dtrace  -s $PERF_SCRIPTS/profile.d"                  "profile"
)

log_note "Sequential cached reads with $PERF_RUNTYPE settings"
do_fio_run sequential_reads.fio false false
log_pass "Measure IO stats during sequential cached read load"
