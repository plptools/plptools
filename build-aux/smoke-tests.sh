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

ROOT_DIRECTORY="$( cd "$( dirname "$( dirname "${BASH_SOURCE[0]}" )" )" &> /dev/null && pwd )"

function fatal {
    echo $1 >&2
    exit 1
}

cd "$ROOT_DIRECTORY"

# Check the binaries exist.
[ -f "ncpd/ncpd" ] || fatal "npd missing"
[ -f "plpftp/plpftp" ] || fatal "plpftp missing"
# TODO: Exercise and smoke test conditional plpfuse builds in CI #58
#       https://github.com/plptools/plptools/issues/58
# [ -f "plpfuse/plpfuse" ] || fatal "plpfuse missing"
[ -f "plpprint/plpprintd" ] || fatal "plpprintd missing"
[ -f "sisinstall/sisinstall" ] || fatal "sisinstall missing"

# Check the versions.
ncpd/ncpd --version || fatal "Failed to get ncpd version"
plpftp/plpftp --version || fatal "Failed to get plpftp version"
# TODO: plpfuse doesn't exit when printing the version #57
#       https://github.com/plptools/plptools/issues/57
# plpfuse/plpfuse --version || fatal "Failed to get ncpd version"
plpprint/plpprintd --version || fatal "Failed to get plpfuse version"
sisinstall/sisinstall --version || fatal "Failed to get sisinstall version"
