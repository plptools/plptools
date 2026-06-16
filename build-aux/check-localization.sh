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

set -e
set -o pipefail
set -x
set -u

# Per-platform environment setup.
case `uname` in
    Linux)
    ;;
    Darwin)
        export PATH="$(brew --prefix coreutils)/libexec/gnubin:$PATH"
        export PATH="$(brew --prefix m4)/bin:$PATH"

        export CPPFLAGS="-I$(brew --prefix gettext)/include -I$(brew --prefix readline)/include"
        export LDFLAGS="-L$(brew --prefix gettext)/lib -L$(brew --prefix readline)/lib -F/Library/Filesystems/macfuse.fs/Contents/Frameworks"
    ;;
esac

# Configure.
./bootstrap --skip-po
./configure

# Generate the localization definition.
make -C po update-po

# Check the localizations.
msgcmp po/de.po po/plptools.pot
msgcmp po/sv.po po/plptools.pot
