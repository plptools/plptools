#!/bin/sh

# This file is part of plptools.
#
#  Copyright (c) 2021-2024 Reuben Thomas <rrt@sc3d.org>
#  Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  along with this program; if not, see <https://www.gnu.org/licenses/>.

set -e
set -u
set -x

DISTCHECK=false
while [ $# -gt 0 ]
do
    key="$1"
    case $key in
        --distcheck)
            DISTCHECK=true
            shift
            ;;
        *)
            >&2 echo "Unknown argument '$key'."
            exit 1
            ;;
    esac
done

MAKE=make

PLATFORM=$(uname -s)
case "$PLATFORM" in
    Linux)
        echo "Building for Linux..."
        ;;
    Haiku)
        echo "Building for Haiku..."
        ;;
    Darwin)
        echo "Building for Darwin..."
        GETTEXT_PREFIX=$(brew --prefix --installed gettext)
        M4_PREFIX=$(brew --prefix --installed m4)
        READLINE_PREFIX=$(brew --prefix --installed readline)
        export PATH="$M4_PREFIX/bin:/usr/local/opt/gettext/bin:$PATH"
        export LDFLAGS="-L$GETTEXT_PREFIX/lib -L$READLINE_PREFIX/lib -F/Library/Filesystems/macfuse.fs/Contents/Frameworks"
        export CPPFLAGS="-I$GETTEXT_PREFIX/include -I$READLINE_PREFIX/include"
        ;;
    FreeBSD)
        echo "Building for FreeBSD..."
        export LDFLAGS="-L/usr/local/lib/"
        MAKE="gmake"
        ;;
    NetBSD)
        echo "Building for NetBSD..."
        export LDFLAGS="-L/usr/pkg/lib -Wl,--rpath=/usr/pkg/lib"

        # Ensure we use the package version of xgettext and friends.
        export XGETTEXT=/usr/pkg/bin/xgettext
        export MSGMERGE=/usr/pkg/bin/msgmerge
        export MSGFMT=/usr/pkg/bin/msgfmt
        export GMSGFMT=/usr/pkg/bin/msgfmt

        MAKE="gmake"
        ;;
    OpenBSD)
        echo "Building for OpenBSD..."
        export CPPFLAGS="-I/usr/local/include/ereadline/"
        export AUTOCONF_VERSION="2.72"
        export AUTOMAKE_VERSION="1.18"
        MAKE="gmake"
        ;;
    *)
        echo "Unsupported platform ($PLATFORM)."
        echo ""
        echo "See https://github.com/plptools/plptools/wiki/Builds for instructions on building on other platforms."
        exit 1
        ;;
esac

./bootstrap --skip-po
./configure

$MAKE V=1 -j8

if $DISTCHECK ; then
    $MAKE distcheck || ( cat ./plptools-*/_build/sub/tests/test-suite.log; exit 1 )
fi
