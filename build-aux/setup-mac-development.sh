#!/bin/bash

# This file is part of plptools.
#
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

# Script to make it easy to set up a macOS machine for development.
#
# At the time of writing, this roughly follows the configure steps described on
# https://github.com/plptools/plptools/wiki/Builds#macos and additionally calls
# `compiledb` to generate `compile_commands.json` which is used by a number of
# language servers for IDE-based development.
#
# Run this whenever the configure scripts or makefiles have changed.

set -e
set -o pipefail
set -x
set -u

ROOT_DIRECTORY="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BUILD_DIRECTORY="$ROOT_DIRECTORY/build"

# Bootstrap.
export PATH="$(brew --prefix coreutils)/libexec/gnubin:$PATH"
export PATH="$(brew --prefix m4)/bin:$PATH"
./bootstrap --skip-po

# Configure.
export CPPFLAGS="-I$(brew --prefix gettext)/include -I$(brew --prefix readline)/include"
export LDFLAGS="-L$(brew --prefix gettext)/lib -L$(brew --prefix readline)/lib"
./configure \
    --prefix="$BUILD_DIRECTORY" \
    CFLAGS="-g -O0" \
    CXXFLAGS="-g -O0"

# Generate compile-commands.json.
compiledb make check TESTS=
