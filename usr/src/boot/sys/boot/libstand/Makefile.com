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
# Copyright 2016 Toomas Soome <tsoome@me.com>
#

include $(SRC)/Makefile.master

AS=	$(GNU_ROOT)/bin/gas
LD=	$(GNU_ROOT)/bin/gld
CC=	$(GNUC_ROOT)/bin/gcc

LIB_BASE=	$(SRC)/boot/lib
LIBSTAND_SRC=	$(LIB_BASE)/libstand

ASFLAGS =	-fPIC
CPPFLAGS =	-nostdinc -I../../../../include -I$(LIBSTAND_SRC)
CPPFLAGS +=	-I../../..  -I../../../sys -I. -I$(SRC)/common/bzip2
CPPFLAGS +=	-D_STANDALONE

CFLAGS =	-O2 -fPIC -ffreestanding -Wformat
CFLAGS +=	-mno-mmx -mno-3dnow -mno-sse -mno-sse2 -mno-sse3 -msoft-float
CFLAGS +=	-Wall -Werror

$(LIBRARY): $(SRCS) $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

include $(LIBSTAND_SRC)/Makefile.inc

clean: clobber
clobber:
	$(RM) $(CLEANFILES) $(OBJS) machine $(LIBRARY)

machine:
	$(RM) machine
	$(SYMLINK) ../../../$(MACHINE)/include machine

x86:
	$(RM) x86
	$(SYMLINK) ../../../x86/include x86

%.o:	$(LIBSTAND_SRC)/%.c
	$(COMPILE.c) $<

%.o:	$(LIB_BASE)/libc/net/%.c
	$(COMPILE.c) $<

%.o:	$(LIB_BASE)/libc/string/%.c
	$(COMPILE.c) $<

%.o:	$(LIB_BASE)/libc/uuid/%.c
	$(COMPILE.c) $<

%.o:	$(LIB_BASE)/libz/%.c
	$(COMPILE.c) $<
