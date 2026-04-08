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

/**
* Psion-native path separator.
*
* Always backslash.
*/
static constexpr char kEPOCSeparator = '\\';

/**
* Host-native path separator.
*
* Forward slash on POSIX operating systems; backslash on Windows.
*/
static constexpr char kHostSeparator = '/';

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
* Return a new string that represents the path, @p path, with a guaranteed
* trailing separator, @p separator.
*
* This function makes no attempt to normalize paths or convert path separators.
*
* @param path Path to test.
* @param separator Path separator to use (should be one of '/' or '\\').
*
* @return @p path + @p separator if path does not end in a separator; @p path, otherwise.
*/
extern std::string ensuring_trailing_separator(const std::string &path, const char separator);

};
