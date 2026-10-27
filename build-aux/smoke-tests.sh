#!/bin/sh

# This file is part of plptools.
#
#  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
#  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
#  Copyright (C) 2006-2025 Reuben Thomas <rrt@sc3d.org>
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

# Smoke tests for the different plptools.
#
# This script is intended to be run after CI builds to help us catch basic dependency related and high-level failures
# when targeting a wide range of platforms.

set -e
set -x
set -u

ROOT_DIRECTORY="$( cd "$( dirname "$0" )/.." > /dev/null 2>&1 && pwd )"

BUILD_PLPFUSE=${BUILD_PLPFUSE:-false}

fatal() {
    echo "$1" >&2
    exit 1
}

check_version() {
    COMMAND="$1"
    echo "Checking '$COMMAND'..."
    if [ ! -f "$COMMAND" ]; then
        fatal "Command '$COMMAND' does not exist."
    fi
    OUTPUT=$("$COMMAND" --version | awk '{print $3}')
    if [ "$OUTPUT" != "$VERSION" ]; then
        fatal "Command '$COMMAND' reported the wrong version ($OUTPUT instead of $VERSION)."
    else
        echo "Command '$COMMAND' reported the correct version."
    fi
}

cd "$ROOT_DIRECTORY"

VERSION=$(cat "$ROOT_DIRECTORY/VERSION.txt")

# Check the versions.
check_version ncpd/ncpd
check_version plpftp/plpftp
if $BUILD_PLPFUSE ; then
    check_version plpfuse/plpfuse
fi
check_version plpprint/plpprintd
check_version sisinstall/sisinstall
