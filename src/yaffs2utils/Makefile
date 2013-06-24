#
# Makefile for yaffs2utils.
#
# yaffs2utils: Utilities to make/extract a YAFFS2/YAFFS1 image
# Copyright (C) 2010-2011 Luen-Yung Lin <penguin.lin@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

CROSS		=
CC		= $(CROSS)gcc

CFLAGS		=  -I. -I./yaffs2
CFLAGS		+= -O2
CFLAGS		+= -Wall -Wshadow -Winline -Wpointer-arith -Wnested-externs \
		   -Wwrite-strings -Wstrict-prototypes -Wmissing-declarations \
		   -Wmissing-prototypes -Wredundant-decls

CFLAGS		+= -D_HAVE_MMAP

#CFLAGS		+= -D_MKYAFFS2_DEBUG
#CFLAGS		+= -D_UNYAFFS2_DEBUG

LDFLAGS		+= -lm

YAFFS2SRCS	= yaffs2/yaffs_hweight.c yaffs2/yaffs_ecc.c \
		  yaffs2/yaffs_packedtags1.c yaffs2/yaffs_packedtags2.c
YAFFS2OBJS	= $(YAFFS2SRCS:.c=.o)

LIBSRCS		= safe_rw.c endian_convert.c progress_bar.c
LIBOBJS		= $(LIBSRCS:.c=.o)

MKYAFFS2SRCS	= mkyaffs2.c
MKYAFFS2OBJS	= $(MKYAFFS2SRCS:.c=.o)

UNYAFFS2SRCS	= unyaffs2.c
UNYAFFS2OBJS	= $(UNYAFFS2SRCS:.c=.o)

UNSPARE2SRCS	= unspare2.c
UNSPARE2OBJS	= $(UNSPARE2SRCS:.c=.o)

TARGET		= mkyaffs2 unyaffs2 unspare2

INSTALLDIR	= /bin


all: $(TARGET)

install:
	cp $(TARGET) $(INSTALLDIR)

mkyaffs2: $(YAFFS2OBJS) $(LIBOBJS) $(MKYAFFS2OBJS)
	$(CC) -o $@ $(YAFFS2OBJS) $(LIBOBJS) $(MKYAFFS2OBJS) $(LDFLAGS)

unyaffs2: $(YAFFS2OBJS) $(LIBOBJS) $(UNYAFFS2OBJS)
	$(CC) -o $@ $(YAFFS2OBJS) $(LIBOBJS) $(UNYAFFS2OBJS) $(LDFLAGS)

unspare2: $(YAFFS2OBJS) $(LIBOBJS) $(UNSPARE2OBJS)
	$(CC) -o $@ $(YAFFS2OBJS) $(LIBOBJS) $(UNSPARE2OBJS) $(LDFLAGS)

clean:
	rm -rf $(YAFFS2OBJS) $(LIBOBJS) \
	       $(MKYAFFS2OBJS) $(UNYAFFS2OBJS) $(UNSPARE2OBJS)

distclean: clean
	rm -rf $(TARGET)

.PHONY: all clean distclean $(TARGET)
