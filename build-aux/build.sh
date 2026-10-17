#!/bin/bash

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
set -o pipefail
set -u

POSITIONAL=()
ASAN=false
DISTCHECK=false
while [[ $# -gt 0 ]]
do
    key="$1"
    case $key in
        --sanitize-address)
            ASAN=true
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

BOOTSTRAP_ARGS=()
BOOTSTRAP_ARGS+=(--skip-po)

CONFIGURE_ARGS=()

PLATFORM=$(uname -s)
case "$PLATFORM" in
    Linux)
        echo "Building for Linux..."
        ;;
    Haiku)
        echo "Building for Haiku..."
        if $ASAN ; then
            >&2 echo "Warning: Skipping address sanitize..."
            ASAN=false
        fi
        ;;
    Darwin)
        echo "Building for Darwin..."
        export PATH="$(brew --prefix m4)/bin:/usr/local/opt/gettext/bin:$PATH"
        export LDFLAGS="-L$(brew --prefix gettext)/lib -L$(brew --prefix readline)/lib -F/Library/Filesystems/macfuse.fs/Contents/Frameworks"
        export CPPFLAGS="-I$(brew --prefix gettext)/include -I$(brew --prefix readline)/include"
        ;;
    *)
        echo "Unsupported platform ($PLATFORM)."
        echo ""
        echo "See https://github.com/plptools/plptools/wiki/Builds for instructions on building on other platforms."
        exit 1
        ;;
esac

if $ASAN ; then
    CONFIGURE_ARGS+=(CFLAGS="-g3 -fsanitize=address -fsanitize=undefined" CXXFLAGS="-g3 -fsanitize=address -fsanitize=undefined" LDFLAGS="${LDFLAGS:-} -fsanitize=address -fsanitize=undefined")
fi

# Unfortunately we have to use a pretty grim form of the array expansion here to ensure it's safe with `set -u` on macOS
# as macOS still ships an out-of-date buggy bash.
./bootstrap ${BOOTSTRAP_ARGS[@]+"${BOOTSTRAP_ARGS[@]}"}
./configure ${CONFIGURE_ARGS[@]+"${CONFIGURE_ARGS[@]}"}

make V=1

if $DISTCHECK ; then
    make distcheck || ( cat ./plptools-*/_build/sub/tests/test-suite.log; exit 1 )
fi
