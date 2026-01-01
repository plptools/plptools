# Top level Makefile.am.
#
# This file is part of plptools.
#
# Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = po libgnu lib ncpd plpftp plpprint sisinstall doc
if BUILD_PLPFUSE
SUBDIRS += plpfuse
endif

if HAVE_SYSTEMD
dist_systemdsystemunit_DATA = etc/ncpd.service etc/plpprintd.service
endif

EXTRA_DIST = AUTHORS COPYING INSTALL NEWS README TODO HISTORY ABOUT-NLS \
	etc/plptools.in \
	etc/ncpd.service.in \
	etc/plpprintd.service.in \
	etc/ttytap.c etc/psidump etc/psidump.1 \
	etc/udev-usbserial-plptools.rules \
	m4/gnulib-cache.m4

DISTCLEANFILES = etc/plptools

uninstall-local:
	rm -f $(DESTDIR)$(initdir)/plptools

install-exec-local:
	$(INSTALL) -d $(DESTDIR)$(initdir)
	$(INSTALL) $(top_builddir)/etc/plptools $(DESTDIR)$(initdir)/plptools

# FIXME: Automatically revert .po files if they're actually clean
# Currently, rather than run `git diff --exit-code', we simply show the
# diff, and eyeball it to check there's nothing other than the datestamps on
# the .po files.
release: distcheck
	git diff && \
	gh release create v$(VERSION) --title "Release v$(VERSION)" $(DIST_ARCHIVES)
