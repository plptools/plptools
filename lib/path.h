/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2006-2025 Reuben Thomas <rrt@sc3d.org>
 *  Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "config.h"

#include <string>

/**
 * Conveniences for working with paths.
 *
 * These methods currently have a mishmash of C and C++ as they're a collection of various utilities from around
 * plptools. Hopefully they can be unified in the future.
 */
class Path {
public:

    /**
     * Returns the last path component of an EPOC path.
     *
     * If the path doesn't contain any EPOC path separators (`\`), the returned string matches the path.
     */
    static std::string getEPOCBasename(std::string path);

    /**
     * Compute parent directory of an EPOC directory.
     */
    static char *getEPOCDirname(const char *path);

    /**
     * Returns a new absolute EPOC path, determined by resolving `path` relative to `initialPath`.
     *
     * If `path` is already an absolute path, this returns `path`.
     */
    static char *resolveEPOCPath(const char *path, const char *initialPath);

};
