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
#include <vector>

/**
* Conveniences for working with paths.
*
* These methods currently have a mishmash of C and C++ as they're a collection of various utilities from around
* plptools. Hopefully they can be unified in the future.
*/
namespace pathutils {

enum class PathFormat {
    kPOSIX = 0,
    kWindows = 1,

#if defined(_WIN32)
    kHost = kWindows,
#else
    kHost = kPOSIX,
#endif
    kEPOC = kWindows,
};

extern char path_separator(const PathFormat format);

/**
* Returns the last path component of an EPOC path.
*
* If the path doesn't contain any EPOC path separators (`\`), the returned string matches the path.
*/
extern std::string epoc_basename(std::string path);

/**
* Compute parent directory of an EPOC directory.
*/
extern char *epoc_dirname(const char *path);

/**
* Returns a new absolute EPOC path, determined by resolving `path` relative to `initialPath`.
*
* If `path` is already an absolute path, this returns `path`.
*/
extern char *resolve_epoc_path(const char *path, const char *initialPath);

/**
* Split a path, @p path, into its components, using the path separator, @p separator.
*
* If the path is absolute, the first element of the path separator will represent the root component appropriate to
* the path type (POSIX or Windows) as implied by the path separator (e.g., '/' or 'C:').
*
* If the path ends in a trailing separator, the returned vector will contain a last empty string component as an
* intentional marker.
*
* @param path Path to split.
* @param format Path format.
*
* @return Vector containing the path components.
*/
extern std::vector<std::string> split(const std::string &path, const PathFormat format);

/**
* Return a new path by joining the path components, @p components, with path separator, @p separator.
*
* For absolute paths, the first path component should be a root component (e.g., '/' or 'C:').
*
* Empty strings indicate an intentional directory marker. These are valid anywhere, but have no effect and are therefore
* ignored unless they occur at the end of the path where they cause the returned path to have a trailing separator. If a
* path comprises only a single empty string component (directory marker) then the returned path will be the relative
* path '.' followed by the relevant separator.
*
* @param components Path components to join.
* @param format Path format.
*
* @return String containing the resulting path.
*/
extern std::string join(const std::vector<std::string> &components, const PathFormat format);

/**
* Check if a path is absolute.
*
* For a Windows path to be absolute, it must be both fully qualified (with a drive), and rooted (with a leading path
* separator.
*
* @param path Path to test.
* @param format Path format.
*
* @return true if the path, @p path, is absolute; false otherwise.
*/
extern bool is_absolute(const std::string &path, const PathFormat format);

/**
* Convenience wrapper for @ref join that returns a new path resulting from appending path components, @p components,
* to path, @p path, using separator, @p separator.
*
* @param path Base path to append components to.
* @param components Components to append to @p path.
* @param format Path format.
*
* @return String containing the resulting path.
*/
extern std::string appending_components(const std::string &path,
                                        const std::vector<std::string> &components,
                                        const PathFormat format);

/**
* Returns a path by resolving a relative or absolute path against a starting path.
*
* @p startingPath may be relative or absolute, but @p path must be contained within that path.
*/
extern std::string resolve_path(const std::string &path, const std::string &startingPath, const PathFormat format);

};
