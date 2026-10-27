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

check_command() {
    COMMAND="$1"
    EXPECTED="$2"

    # Check that the command doesn't exist if it's not expected.
    if ! $EXPECTED ; then
        if [ -f "$COMMAND" ]; then
            fatal "Command '$COMMAND' exists unexpectedly."
        fi
        return
    fi

    # Check the command exists.
    echo "Checking '$COMMAND'..."
    if [ ! -f "$COMMAND" ]; then
        fatal "Command '$COMMAND' does not exist."
    fi

    # Check that the command outputs the correct version.
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
check_command ncpd/ncpd true
check_command plpftp/plpftp true
check_command plpfuse/plpfuse $BUILD_FUSE
check_command plpprint/plpprintd true
check_command sisinstall/sisinstall true
