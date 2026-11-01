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

fatal() {
    echo "$1" >&2
    exit 1
}

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

require_fuse() {
    # Ensure $FUSE_VERSION matches $1; exit with a fatal error otherwise.
    EXPECTED_FUSE_VERSION="$1"
    if [ "$FUSE_VERSION" != "$EXPECTED_FUSE_VERSION" ]; then
        fatal "Unsupported FUSE version; expected $EXPECTED_FUSE_VERSION, got $FUSE_VERSION"
    fi
}

debian_install_libfuse_dev() {
    if [ "$FUSE_VERSION" = "2" ]; then
        become apt-get install -y libfuse-dev
    elif [ "$FUSE_VERSION" = "3" ]; then
        become apt-get install -y libfuse3-dev
    else
        fatal "Unsupported FUSE version."
    fi
}

: "${BUILD_PLPFUSE:?BUILD_PLPFUSE must be set to 'true' or 'false'}"
if $BUILD_PLPFUSE ; then
    : "${FUSE_VERSION:?FUSE_VERSION must be set}"
fi

PLATFORM=$(uname -s)
case "$PLATFORM" in
    Linux)
        echo "Installing dependencies for Linux..."
        . /etc/os-release
        case "$ID" in
            ubuntu)
                become apt-get update -y
                become apt-get -y install build-essential m4 libtool git libreadline-dev pkg-config gettext autopoint libattr1-dev
                if $BUILD_PLPFUSE ; then
                    debian_install_libfuse_dev
                fi
                ;;
            debian)
                become apt-get update -y
                become apt-get install -y build-essential git libtool gpg pkg-config gettext autopoint libreadline-dev
                if $BUILD_PLPFUSE ; then
                    debian_install_libfuse_dev
                fi
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
        brew install coreutils m4 readline pkg-config libtool automake gettext
        if $BUILD_PLPFUSE ; then
            require_fuse 2
            brew install macfuse
        fi
        ;;
    FreeBSD)
        echo "Installing dependencies for FreeBSD..."
        become pkg install -y m4 libtool git gnupg pkgconf automake autoconf gettext gmake
        if $BUILD_PLPFUSE ; then
            require_fuse 2
        fi
        ;;
    NetBSD)
        echo "Installing dependencies for NetBSD..."
        become pkgin -y install m4 libtool gnupg2 pkgconf autoconf automake gettext coreutils gmake git gettext-tools
        if $BUILD_PLPFUSE ; then
            require_fuse 2
        fi
        ;;
    OpenBSD)
        echo "Installing dependencies for OpenBSD..."
        become pkg_add -v m4 libtool gnupg autoconf%2.72 automake%1.18 gettext-tools readline gmake git
        if $BUILD_PLPFUSE ; then
            require_fuse 2
        fi
        ;;
    *)
        exit_unsupported "Unsupported platform ($PLATFORM)."
        ;;
esac
