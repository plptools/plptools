#!/bin/sh

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
set -u

exit_unsupported() {
    echo "$1"
    echo ""
    echo "See https://github.com/plptools/plptools/wiki/Builds for instructions on building on other platforms."
    exit 1
}

become() {
    # $USER is unbound in GitHub Actions containers; use this to infer whether we should use sudo.
    if [ -z ${USER+x} ]; then
        SUDO=""
    else
        SUDO="sudo"
    fi
    $SUDO "$@"
}

PLATFORM=$(uname -s)
case "$PLATFORM" in
    Linux)
        echo "Installing dependencies for Linux..."
        . /etc/os-release
        case "$ID" in
            ubuntu)
                become apt-get update -y
                become apt-get -y install build-essential m4 libtool git libreadline-dev pkg-config gettext autopoint libfuse-dev libattr1-dev
                ;;
            debian)
                become apt-get update -y
                become apt-get install -y build-essential git libtool gpg pkg-config gettext autopoint libreadline-dev
                ;;
            *)
                exit_unsupported "Unsupported Linux distribution ($ID)."
                ;;
        esac
        ;;
    Haiku)
        echo "Installing dependencies for Haiku..."
        pkgman install -y libtool gnupg gettext readline_devel
        ;;
    Darwin)
        echo "Installing dependencies for Darwin..."
        brew install coreutils m4 readline pkg-config libtool automake gettext macfuse
        ;;
    FreeBSD)
        echo "Installing dependencies for FreeBSD..."
        become pkg install -y m4 libtool git gnupg pkgconf automake autoconf gettext gmake
        ;;
    NetBSD)
        echo "Installing dependencies for NetBSD..."
        become pkgin -y install m4 libtool gnupg2 pkgconf autoconf automake gettext coreutils gmake git gettext-tools
        ;;
    OpenBSD)
        echo "Installing dependencies for OpenBSD..."
        become pkg_add -v m4 libtool gnupg autoconf%2.72 automake%1.18 gettext-tools readline gmake git
        ;;
    *)
        exit_unsupported "Unsupported platform ($PLATFORM)."
        ;;
esac
