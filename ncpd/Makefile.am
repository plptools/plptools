# ncpd/Makefile.am
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

sbin_PROGRAMS = ncpd
ncpd_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_srcdir)/libgnu -I$(top_builddir)/libgnu
ncpd_CFLAGS = $(THREADED_CFLAGS)
ncpd_CXXFLAGS = $(THREADED_CXXFLAGS)
ncpd_LDADD = $(LIB_PLP) $(INTLLIBS) $(LIBPMULTITHREAD) $(LIBTHREAD) $(NANOSLEEP_LIB) $(PTHREAD_SIGMASK_LIB) $(SELECT_LIB) $(top_builddir)/libgnu/libgnu.a
ncpd_SOURCES = \
	channel.cc \
	channel.h \
	link.cc \
	link.h  \
	linkchan.cc \
	linkchan.h \
	main.cc \
	mp_serial.c \
	mp_serial.h \
	ncp_log.cc \
	ncp_log.h \
	ncp_session.cc \
	ncp_session.h \
	ncp.cc \
	ncp.h \
	packet.cc \
	packet.h \
	socketchan.cc \
	socketchan.h
