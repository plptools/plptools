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

#include "pathutils.h"

#include "xalloc.h"
#include "xvasprintf.h"
#include <string>
#include <unistd.h>

std::string pathutils::epoc_basename(std::string path) {
    size_t end = path.find_last_of("\\");
    if (end == std::string::npos) {
        return std::string(path);
    }
    return path.substr(end+1);
}

char *pathutils::epoc_dirname(const char *path) {
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

char *pathutils::resolve_epoc_path(const char *path, const char *relativeToPath) {
    char *f1;

    // If we have asked for parent dir, get dirname of cwd.
    if (!strcmp(path, "..")) {
        f1 = epoc_dirname(relativeToPath);
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

std::vector<std::string> pathutils::split(const std::string string, const char separator) {
    std::vector<std::string> result;
    size_t offset = 0;
    size_t index = 0;
    while ((index = string.find(separator, offset)) != std::string::npos) {
        // If the index of the first separator is 0, then we know the path is absolute and we insert the
        // root directory path component.
        if (index == 0) {
            result.push_back("/");
        }
        int length = index-offset;
        if (length > 0) {
            result.push_back(string.substr(offset, index-offset));
        }
        offset = index + 1;
    }
    if (offset - string.length() > 0) {
        result.push_back(string.substr(offset));
    }
    return result;
}

std::string pathutils::join(const std::vector<std::string> &components, const char separator) {
    std::string result;
    for (const auto &component : components) {
        if (!result.empty() && result.back() != separator) {
            result += separator;
        }
        result += component;
    }
    return result;
}

std::string pathutils::appending_components(const std::string &path,
                                            const std::vector<std::string> &_components,
                                            const char separator) {
    std::vector<std::string> components = {path};
    components.insert(components.end(), _components.begin(), _components.end());
    return join(components, separator);
}

std::string pathutils::ensuring_trailing_separator(const std::string &path,
                                              const char separator) {
    if (!path.empty() && path.back() == separator) {
        return path;
    }
    return path + separator;
}

bool pathutils::is_root(const std::string &pathComponent, const char separator) {
    if (separator == '/' && pathComponent.length() == 1 && pathComponent[0] == separator) {
        return true;
    } else if (separator == '\\' && pathComponent.length() == 2) {
        return std::isalpha(static_cast<unsigned char>(pathComponent[0])) && pathComponent[1] == ':';
    } else {
        return false;
    }
}

bool pathutils::is_absolute(const std::string &path, const char separator) {
    auto components = split(path, separator);
    if (components.empty()) {
        return false;
    }
    return is_root(components.front(), separator);
}

std::string pathutils::resolve_path(const std::string &path,
                                    const std::string &startingPath,
                                    const char separator) {
    std::vector<std::string> pathComponents = split(path, separator);
    std::vector<std::string> startingPathComponents = split(startingPath, separator);

    // Check to see if the path is already absolute by inspecting the first component.
    if (!pathComponents.empty() && is_root(pathComponents.front(), separator)) {
        return path;
    }

    for (const auto &pathComponent : pathComponents) {
        if (pathComponent == "..") {
            // TODO: We should probably crash intentionally if this doesn't work?
            startingPathComponents.pop_back();
        } else {
            startingPathComponents.push_back(pathComponent);
        }
    }

    return pathutils::join(startingPathComponents, separator);
}
