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
# Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#

LIBRARY =	libreparse_smb.a
VERS =		.1

OBJS_COMMON =	smbrp_plugin.o


OBJECTS=        $(OBJS_COMMON)

include ../../../Makefile.lib
include ../../Makefile.lib

ROOTLIBDIR =    $(ROOT)/usr/lib/reparse
ROOTLIBDIR64 =  $(ROOT)/usr/lib/reparse/$(MACH64)

LDLIBS +=	$(MACH_LDLIBS)
LDLIBS += -lc

CPPFLAGS += -D_REENTRANT

SRCS=   $(OBJS_COMMON:%.o=$(SRCDIR)/%.c)

include ../../Makefile.targ
include ../../../Makefile.targ
