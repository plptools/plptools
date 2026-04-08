# lib/Makefile.am
#
# This file is part of plptools.
#
# Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
# Copyright (C) 2007-2024 Reuben Thomas <rrt@sc3d.org>
# Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# along with this program; if not, see <https://www.gnu.org/licenses/>.

AM_CPPFLAGS = -I$(top_srcdir)/libgnu -I$(top_builddir)/libgnu
AM_CXXFLAGS = $(THREADED_CXXFLAGS) $(WARN_CXXFLAGS)
LDADD = $(HOSTENT_LIB) $(LIBSOCKET)

pkglib_LTLIBRARIES = libplp.la

libplp_la_LIBADD = $(INTLLIBS) $(LIBPMULTITHREAD) $(LIBTHREAD) $(NANOSLEEP_LIB) $(PTHREAD_SIGMASK_LIB) $(SELECT_LIB) $(top_builddir)/libgnu/libgnu.la

libplp_la_SOURCES = \
	bufferarray.cc \
	bufferstore.cc \
	channel.cc \
	cli_utils.cc \
	datalink.cc \
	drive.cc \
	Enum.cc \
	iowatch.cc \
	link.cc \
	linkchannel.cc \
	log.cc \
	mp_serial.c \
	ncp_log.cc \
	ncp.cc \
	ncpsession.cc \
	pathutils.cc \
	plpdirent.cc \
	psibitmap.cpp \
	psiprocess.cc \
	psitime.cc \
	rclip.cc \
	rfsv.cc \
	rfsv16.cc \
	rfsv32.cc \
	rfsvfactory.cc \
	rpcs.cc \
	rpcs16.cc \
	rpcs32.cc \
	rpcsfactory.cc \
	siscomponentrecord.cpp  \
	sisfile.cpp \
	sisfileheader.cpp \
	sisfilerecord.cpp \
	sislangrecord.cpp \
	sisreqrecord.cpp \
	sistypes.cpp \
	socketchannel.cc \
	tcpsocket.cc \
	wprt.cc

noinst_HEADERS = \
	bufferarray.h \
	bufferstore.h \
	channel.h \
	cli_utils.h \
	datalink.h \
	drive.h \
	Enum.h \
	iowatch.h \
	link.h  \
	linkchannel.h \
	log.h \
	mp_serial.h \
	ncp_log.h \
	ncp.h \
	ncpsession.h \
	ncpstatuscallback.h \
	pathutils.h \
	plpdirent.h \
	plpintl.h \
	psibitmap.h \
	psiprocess.h \
	psitime.h \
	rclip.h \
	rfsv.h \
	rfsv16.h \
	rfsv32.h \
	rfsvfactory.h \
	rpcs.h \
	rpcs16.h \
	rpcs32.h \
	rpcsfactory.h \
	siscomponentrecord.h \
	sisfile.h \
	sisfileheader.h \
	sisfilerecord.h \
	sislangrecord.h \
	sisreqrecord.h \
	sistypes.h \
	socketchannel.h \
	tcpsocket.h \
	wprt.h
