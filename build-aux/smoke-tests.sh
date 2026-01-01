#!/bin/bash

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

set -exuo pipefail

function assert_exists {
    { local old_opts=$-; set +x; } 2>/dev/null
    filename=$1
    echo -n "Checking '$1' exists... "
    if [ -f "$filename" ]; then
        echo "OK"
    else
        echo "FAILED"
        echo "Error: '$filename' does not exist" >&2
        exit 1
    fi
    set -$old_opts 2>/dev/null || true
}

assert_exists "ncpd/ncpd"
assert_exists "plpftp/plpftp"
assert_exists "plpprint/plpprintd"
assert_exists "sisinstall/sisinstall"
