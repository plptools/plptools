# plpprint/Makefile.am
#
# This file is part of plptools.
#
# Copyright (C) 2002 Fritz Elfert <felfert@to.com>
# Copyright (C) 2007-2025 Reuben Thomas <rrt@sc3d.org>
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

sbin_PROGRAMS = plpprintd
plpprintd_CPPFLAGS = -DPKGDATADIR="\"$(pkgdatadir)\"" -I$(top_srcdir)/lib -I$(top_srcdir)/libgnu -I$(top_builddir)/libgnu
plpprintd_CXXFLAGS = $(WARN_CXXFLAGS)
plpprintd_LDADD = $(LIB_PLP) $(INTLLIBS) $(SERVENT_LIB) $(top_builddir)/libgnu/libgnu.a
plpprintd_SOURCES = plpprintd.cc

EXTRA_DIST = prolog.ps.in fontmap

pkgdata_DATA = prolog.ps fontmap
