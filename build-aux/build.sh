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

SANITIZE_ADDRESS=false
DISTCHECK=false
while [ $# -gt 0 ]
do
    key="$1"
    case $key in
        --sanitize-address)
            SANITIZE_ADDRESS=true
            shift
            ;;
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
        if $SANITIZE_ADDRESS ; then
            >&2 echo "Warning: Skipping address sanitize..."
            SANITIZE_ADDRESS=false
        fi
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
    *)
        echo "Unsupported platform ($PLATFORM)."
        echo ""
        echo "See https://github.com/plptools/plptools/wiki/Builds for instructions on building on other platforms."
        exit 1
        ;;
esac

./bootstrap --skip-po

if $SANITIZE_ADDRESS ; then
    SANITIZE_FLAGS="-fsanitize=address -fsanitize=undefined"
    ./configure CFLAGS="-g3 $SANITIZE_FLAGS" CXXFLAGS="-g3 $SANITIZE_FLAGS" LDFLAGS="${LDFLAGS:-} $SANITIZE_FLAGS"
else
    ./configure
fi

$MAKE V=1

if $DISTCHECK ; then
    $MAKE distcheck || ( cat ./plptools-*/_build/sub/tests/test-suite.log; exit 1 )
fi
