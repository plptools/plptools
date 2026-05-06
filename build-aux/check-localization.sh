#!/bin/bash

set -e

./bootstrap --skip-po
./configure

# Generate the localization definition.
make -C po update-po

# Check the localizations.
msgcmp -v po/de.po po/plptools.pot
msgcmp -v po/sv.po po/plptools.pot
