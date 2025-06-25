# lib/Makefile.am
#
# This file is part of plptools.
#
# Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
# Copyright (C) 2007-2024 Reuben Thomas <rrt@sc3d.org>
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

libplp_la_SOURCES = bufferarray.cc  bufferstore.cc iowatch.cc ppsocket.cc \
	rfsv16.cc rfsv32.cc rfsvfactory.cc log.cc rfsv.cc rpcs32.cc rpcs16.cc \
	rpcs.cc rpcsfactory.cc psitime.cc Enum.cc plpdirent.cc wprt.cc \
	rclip.cc siscomponentrecord.cpp  sisfile.cpp sisfileheader.cpp \
	sisfilerecord.cpp sislangrecord.cpp sisreqrecord.cpp sistypes.cpp \
	psibitmap.cpp psiprocess.cc
noinst_HEADERS = bufferarray.h bufferstore.h iowatch.h ppsocket.h \
	rfsv.h rfsv16.h rfsv32.h rfsvfactory.h log.h rpcs32.h rpcs16.h rpcs.h \
	rpcsfactory.h psitime.h Enum.h plpdirent.h wprt.h plpintl.h rclip.h \
	siscomponentrecord.h sisfile.h sisfileheader.h sisfilerecord.h \
	sislangrecord.h sisreqrecord.h sistypes.h psibitmap.h psiprocess.h
