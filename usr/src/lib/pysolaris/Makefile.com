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
# Copyright (c) 2009, 2010, Oracle and/or its affiliates. All rights reserved.
# Copyright 2018 OmniOS Community Edition (OmniOSce) Association.
#

LIBRARY =	misc.a
VERS =
OBJECTS =	misc.o

PYSRCS=		__init__.py

include ../../Makefile.lib

LIBLINKS =
SRCDIR =	../common
ROOTLIBDIR=	$(ROOT)/usr/lib/python$(PYTHON_VERSION)/vendor-packages/solaris
ROOTLIBDIR64=	$(ROOTLIBDIR)/64
PYOBJS=		$(PYSRCS:%.py=$(SRCDIR)/%.pyc)
PYFILES=	$(PYSRCS) $(PYSRCS:%.py=%.pyc)
ROOTPYSOLFILES=	$(PYFILES:%=$(ROOTLIBDIR)/%)

CSTD=		$(CSTD_GNU99)
C99LMODE=	-Xc99=%all

LIBS =		$(DYNLIB)
LDLIBS +=	-lc -lsec -lidmap -lpython$(PYTHON_VERSION)$(PYTHON_SUFFIX)
CFLAGS +=	$(CCVERBOSE)
CERRWARN +=	-_gcc=-Wno-unused-variable
CPPFLAGS +=	\
	-I$(ADJUNCT_PROTO)/usr/include/python$(PYTHON_VERSION)$(PYTHON_SUFFIX)

all:

.KEEP_STATE:

$(ROOTLIBDIR)/%: %
	$(INS.pyfile)

$(ROOTLIBDIR)/%: ../common/%
	$(INS.pyfile)

lint: lintcheck

include ../../Makefile.targ
