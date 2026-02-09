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

#include "config.h"

#include "path.h"

#include "xalloc.h"
#include "xvasprintf.h"

std::string Path::getEPOCBasename(std::string path) {
    size_t end = path.find_last_of("\\");
    if (end == std::string::npos) {
        return std::string(path);
    }
    return path.substr(end+1);
}

char *Path::getEPOCDirname(const char *path) {
    char *f1 = xstrdup(path);
    char *p = f1 + strlen(f1);

    // Skip trailing slash.
    if (p > f1) {
        *--p = '\0';
    }

    // Skip backwards to next slash.
    while ((p > f1) && (*p != '/') && (*p != '\\')) {
        p--;
    }

    // If we have just a drive letter, colon and slash, keep exactly that.
    if (p - f1 < 3) {
        p = f1 + 3;
    }

    // Truncate the string at the current point, and return it.
    *p = '\0';

    return f1;
}

char *Path::resolveEPOCPath(const char *path, const char *relativeToPath) {
    char *f1;

    // If we have asked for parent dir, get dirname of cwd.
    if (!strcmp(path, "..")) {
        f1 = getEPOCDirname(relativeToPath);
    } else {
        if ((path[0] != '/') && (path[0] != '\\') && (path[1] != ':')) {
            // If path is relative, append it to cwd.
            f1 = xasprintf("%s%s", relativeToPath, path);
        } else {
            // Otherwise, path is absolute, so duplicate it.
            f1 = xstrdup(path);
        }
    }

    /* Convert forward slashes in new path to backslashes. */
    for (char *p = f1; *p; p++) {
        if (*p == '/') {
            *p = '\\';
        }
    }

    return f1;
}
