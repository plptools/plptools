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

# Update the localizations.
make -C po update-po

# Ensure the localization files are unchanged.
STATUS=$( git status --porcelain po/plptools.pot po/de.po po/sv.po )
if [ -n "$STATUS" ]; then
    >&2 echo "Committed localization files are out of date."
    echo "$STATUS"
    git diff
    exit 1
fi

# Check the localizations.
# TODO: Make these fatal when we've caught up with the existing localizations.
msgcmp po/de.po po/plptools.pot || true
msgcmp po/sv.po po/plptools.pot || true
