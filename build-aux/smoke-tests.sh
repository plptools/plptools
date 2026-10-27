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

fatal() {
    echo "$1" >&2
    exit 1
}

check_version() {
    COMMAND="$1"
    echo "Checking '$COMMAND'..."
    OUTPUT=$("$COMMAND" --version | awk '{print $3}')
    if [ "$OUTPUT" != "$VERSION" ]; then
        fatal "$COMMAND reported the wrong version ($OUTPUT instead of $VERSION)."
    else
        echo "$COMMAND reported the correct version."
    fi
}

cd "$ROOT_DIRECTORY"

VERSION=$(cat "$ROOT_DIRECTORY/VERSION.txt")
echo "$VERSION"

# Check the binaries exist.
[ -f "ncpd/ncpd" ] || fatal "ncpd missing"
[ -f "plpftp/plpftp" ] || fatal "plpftp missing"
# TODO: Exercise and smoke test conditional plpfuse builds in CI #58
#       https://github.com/plptools/plptools/issues/58
# [ -f "plpfuse/plpfuse" ] || fatal "plpfuse missing"
[ -f "plpprint/plpprintd" ] || fatal "plpprintd missing"
[ -f "sisinstall/sisinstall" ] || fatal "sisinstall missing"

# Check the versions.
check_version ncpd/ncpd
check_version plpftp/plpftp
check_version plpfuse/plpfuse
check_version plpprint/plpprintd
check_version sisinstall/sisinstall
