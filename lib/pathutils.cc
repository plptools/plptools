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

static bool is_windows_drive(const std::string &pathComponent) {
    return (pathComponent.size() == 2 &&
            std::isalpha(static_cast<unsigned char>(pathComponent[0])) &&
            pathComponent[1] == ':');
}

static bool is_separator(const std::string &pathComponent, const pathutils::PathFormat format) {
    return pathComponent.length() == 1 && pathComponent[0] == pathutils::path_separator(format);
}

static bool is_absolute(const std::vector<std::string> &components, const pathutils::PathFormat format) {
    switch (format) {
        case pathutils::PathFormat::kPOSIX:
            return components.size() >= 1 && components[0] == "/";
        case pathutils::PathFormat::kWindows:
            return components.size() >= 2 && is_windows_drive(components[0]) && components[1] == "\\";
    }
}

static bool is_rooted(const std::vector<std::string> &components, const pathutils::PathFormat format) {
    switch (format) {
        case pathutils::PathFormat::kPOSIX:
            return components.size() >= 1 && components[0] == "/";
        case pathutils::PathFormat::kWindows:
            return ((components.size() >= 2 && is_windows_drive(components[0]) && components[1] == "\\") ||
                    (components.size() >= 1 && components[0] == "\\"));
    }
}

static std::string drive_component(const std::vector<std::string> components, const pathutils::PathFormat format) {
    if (format != pathutils::PathFormat::kWindows || components.empty()) {
        return "";
    }
    std::string front = components.front();
    if (!is_windows_drive(front)) {
        return "";
    }
    front[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(front[0])));
    return front;
}

char pathutils::path_separator(const PathFormat format) {
    switch (format) {
    case pathutils::PathFormat::kPOSIX:
        return '/';
    case pathutils::PathFormat::kWindows:
        return '\\';
    }
}

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

std::vector<std::string> pathutils::split(const std::string &path, const PathFormat format) {
    auto separator = path_separator(format);
    std::vector<std::string> result;
    size_t pathStartIndex = 0;

    // If we're on windows, we need to consume the drive first so we pattern match on that and update the starting
    // index accordingly.
    if (format == PathFormat::kWindows && path.length() >= 2 && is_windows_drive(path.substr(0, 2))) {
        result.push_back(path.substr(0, 2));
        pathStartIndex = 2;
    }

    // Once we've handled the drive, it's safe to treat the remaining path consistently across Windows and POSIX---we
    // want to preserve the leading path separator regardless of path format.
    size_t previousIndex = pathStartIndex;
    size_t index = pathStartIndex;
    bool foundNonRootSeparator = false;
    while ((index = path.find(separator, previousIndex)) != std::string::npos) {
        // If the index of the first separator is 0, then we know the path is absolute and we insert the root directory
        // path component.
        if (index == pathStartIndex) {
            result.push_back(path.substr(index, 1));
        } else {
            foundNonRootSeparator = true;
        }
        size_t length = index - previousIndex;
        if (length > 0) {
            result.push_back(path.substr(previousIndex, length));
        }
        previousIndex = index + 1;
    }
    if (previousIndex < path.length() || foundNonRootSeparator) {
        result.push_back(path.substr(previousIndex));
    }
    return result;
}

std::string pathutils::join(const std::vector<std::string> &components, const PathFormat format) {
    std::string drive;
    auto begin = components.begin();

    // Special-case handling of Windows drives.
    if (format == PathFormat::kWindows && components.size() >= 1 && is_windows_drive(components[0])) {
        drive += components[0];
        ++begin;
    }

    // Handle drive paths consistently, taking care not to make relative paths absolute by over-inserting a separator.
    std::string path;
    auto separator = path_separator(format);
    for (auto iterator = begin; iterator != components.end(); iterator++) {
        if (iterator != begin && path.back() != separator) {
            path += separator;
        }

        // An empty string is a directory marker. We don't need to do anything special if the path is already non-empty
        // but if the path is empty then we need to set it to '.' followed by the path separator to ensure it can be
        // correctly resolved by the platform.
        // Note that this implementation implicitly performs some path normalization. While this does go beyond the
        // intended responsibility of this function, it also ensures the output paths are clean. We might choose to
        // move this normalization out into a separate method which walks the components and drops unnecessary directory
        // markers (and flattens parent markers, etc).
        if ((*iterator).empty() && path.empty()) {
            path += ".";
            path += separator;
        }

        path += *iterator;
    }

    return drive + path;
}

std::string pathutils::appending_components(const std::string &path,
                                            const std::vector<std::string> &components,
                                            const PathFormat format) {
    auto pathComponents = split(path, format);
    pathComponents.insert(pathComponents.end(), components.begin(), components.end());
    return join(pathComponents, format);
}

bool pathutils::is_absolute(const std::string &path, const PathFormat format) {
    auto components = split(path, format);
    return ::is_absolute(components, format);
}

/**
* Return the iterator corresponding with the beginning of the path components. May be used to split the path into the
* vectors containing the root markers ('C:', '\\', '/') and the remaining path components.
*
* The implementation is pretty simple---it just scans forward through the path components looking for the drive and/or
* root path component.
*/
static std::vector<std::string>::const_iterator path_find_rootless_components_begin(const std::vector<std::string> &components,
                                                                                    const pathutils::PathFormat format) {
    std::vector<std::string>::const_iterator iterator = components.begin();

    // Check we're not already at the end of the vector.
    if (iterator == components.end()) {
        return iterator;
    }

    // Consume the windows drive component if appropriate and then check we're not at the end of the vector.
    if (format == pathutils::PathFormat::kWindows) {
        if (is_windows_drive(*iterator)) {
            ++iterator;
        }
        if (iterator == components.end()) {
            return iterator;
        }
    }

    // Consume the root component ('\\' or '/') if present.
    if (is_separator(*iterator, format)) {
        ++iterator;
    }

    return iterator;
}

std::string pathutils::resolve_path(const std::string &path,
                                    const std::string &basePath,
                                    const PathFormat format) {
    std::vector<std::string> pathComponents = split(path, format);
    std::vector<std::string> basePathComponents = split(basePath, format);

    // Don't do anything if the path is already absolute.
    if (::is_absolute(pathComponents, format)) {
        return path;
    }

    // Windows paths are pretty gnarly as they can cross drive boundaries. If both paths refer to different drives,
    // we perform some special case checks to ensure we can do something meaningful.
    std::string pathDrive = drive_component(pathComponents, format);
    std::string basePathDrive = drive_component(basePathComponents, format);
    if (!pathDrive.empty() && pathDrive != basePathDrive && !is_rooted(pathComponents, format)) {
        return path;
    }

    // Work out where the path starts.
    auto basePathComponentsPathBegin = path_find_rootless_components_begin(basePathComponents, format);
    std::vector<std::string> resolvedPathRootComponents(basePathComponents.cbegin(), basePathComponentsPathBegin);
    std::vector<std::string> resolvedPathRootlessComponents(basePathComponentsPathBegin, basePathComponents.cend());

    if (is_rooted(pathComponents, format)) {

        // If the path is rooted (but not absolute), then we can simply set the resolved components to that of the path.
        std::string pathSeparator;
        pathSeparator += path_separator(format);
        if (resolvedPathRootComponents.empty() || resolvedPathRootComponents.back() != pathSeparator) {
            resolvedPathRootComponents.push_back(pathSeparator);
        }
        resolvedPathRootlessComponents = std::vector<std::string>(++pathComponents.begin(), pathComponents.end());

    } else {

        // Otherwise, we fold the incoming path components into our base path components.
        for (const auto &pathComponent : pathComponents) {

            // Ensure there aren't any trailing directory markers before processing the next component.
            while (!resolvedPathRootlessComponents.empty() && resolvedPathRootlessComponents.back().empty()) {
                resolvedPathRootlessComponents.pop_back();
            }

            if (pathComponent == "..") {
                if (resolvedPathRootlessComponents.empty() || resolvedPathRootlessComponents.back() == "..") {
                    resolvedPathRootlessComponents.push_back("..");
                } else {
                    resolvedPathRootlessComponents.pop_back();
                }
            } else {
                resolvedPathRootlessComponents.push_back(pathComponent);
            }
        }

    }

    // Reconstitute the path.
    std::vector<std::string> resolvedPathComponents = resolvedPathRootComponents;
    resolvedPathComponents.insert(resolvedPathComponents.end(), resolvedPathRootlessComponents.begin(), resolvedPathRootlessComponents.end());
    std::string resolvedPath = join(resolvedPathComponents, format);

    // If the resulting path is an empty string, we return '.' to ensure it's valid.
    if (resolvedPath.empty()) {
        return ".";
    }

    return resolvedPath;
}
