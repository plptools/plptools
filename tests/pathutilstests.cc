/*
 * This file is part of plptools.
 *
 *  Copyright (c) 2026 Jason Morley <hello@jbmorley.co.uk>
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

#include <stdlib.h>
#include <stdio.h>

#include "doctest.h"
#include "pathutils.h"

using namespace pathutils;

TEST_CASE("pathutils::ensuring_trailing_separator") {

    CHECK(ensuring_trailing_separator("", kWindowsSeparator) == "\\");
    CHECK(ensuring_trailing_separator("\\", kWindowsSeparator) == "\\");
    CHECK(ensuring_trailing_separator("C:", kWindowsSeparator) == "C:\\");
    CHECK(ensuring_trailing_separator("C:\\", kWindowsSeparator) == "C:\\");

    // N.B. These tests assume a POSIX host and will need updating for future Windows support.
    CHECK(ensuring_trailing_separator("", kPOSIXSeparator) == "/");
    CHECK(ensuring_trailing_separator("/", kPOSIXSeparator) == "/");
    CHECK(ensuring_trailing_separator("/mnt", kPOSIXSeparator) == "/mnt/");
    CHECK(ensuring_trailing_separator("/mnt/", kPOSIXSeparator) == "/mnt/");

    // Unsupported path normalization and separator conversion.
    CHECK(ensuring_trailing_separator("/mnt/", kWindowsSeparator) == "/mnt/\\");
    CHECK(ensuring_trailing_separator("C:\\", kPOSIXSeparator) == "C:\\/");
    CHECK(ensuring_trailing_separator("C:\\\\", kWindowsSeparator) == "C:\\\\");
    CHECK(ensuring_trailing_separator("/mnt//", kPOSIXSeparator) == "/mnt//");
}

TEST_CASE("pathutils::epoc_basename") {
    CHECK(epoc_basename("C:\\Random") == "Random");
    CHECK(epoc_basename("C:\\Documents\\foo.txt") == "foo.txt");

    // Legacy behavior (might be a bug).
    CHECK(epoc_basename("C:\\Random\\") == "");
    CHECK(epoc_basename("hello") == "hello");
    CHECK(epoc_basename("") == "");
}

TEST_CASE("pathutils::epoc_dirname") {
    SUBCASE("C:\\Random") {
        std::string result;
        result = epoc_dirname("C:\\Random");
        CHECK(result == "C:\\");
    }
    SUBCASE("C:\\Random\\") {
        std::string result;
        result = epoc_dirname("C:\\Random\\");
        CHECK(result == "C:\\");
    }
    SUBCASE("C:\\Documents\\foo.txt") {
        std::string result;
        result = epoc_dirname("C:\\Documents\\foo.txt");
        CHECK(result == "C:\\Documents");
    }
    SUBCASE("C:\\") {
        std::string result;
        result = epoc_dirname("C:\\");
        CHECK(result == "C:");
    }

    // Legacy behavior (might be a bug).
    SUBCASE("C:") {
        std::string result;
        result = epoc_dirname("C:");
        CHECK(result == "C");
    }
}

TEST_CASE("pathutils::is_absolute") {

    CHECK(is_absolute("/", Platform::kPOSIX) == true);
    CHECK(is_absolute("\\", Platform::kWindows) == false);

    CHECK(is_absolute("C:\\", Platform::kWindows) == true);
    CHECK(is_absolute("C:\\", Platform::kPOSIX) == false);

    CHECK(is_absolute("C:", Platform::kWindows) == true);
    CHECK(is_absolute("C:", Platform::kPOSIX) == false);
    CHECK(is_absolute("C:foo", Platform::kWindows) == false);
    CHECK(is_absolute("C:foo", Platform::kPOSIX) == false);

    CHECK(is_absolute("", Platform::kWindows) == false);
    CHECK(is_absolute("", Platform::kPOSIX) == false);

    CHECK(is_absolute("foo", Platform::kWindows) == false);
    CHECK(is_absolute("foo", Platform::kPOSIX) == false);
    CHECK(is_absolute("foo\\bar", Platform::kWindows) == false);
    CHECK(is_absolute("foo/bar", Platform::kPOSIX) == false);

    CHECK(is_absolute("\\C:\\", Platform::kWindows) == false);
    CHECK(is_absolute("/C:/", Platform::kPOSIX) == true);
}

TEST_CASE("pathutils::appending_components") {
    CHECK(appending_components("C:\\", {"Documents"}, '\\') == "C:\\Documents");
    CHECK(appending_components("C:\\", {"Documents"}, '\\') == "C:\\Documents");
}

TEST_CASE("pathutils::split") {
    CHECK(split("", Platform::kWindows) == std::vector<std::string>({}));
    CHECK(split("one\\two\\three", Platform::kWindows) == std::vector<std::string>({"one", "two", "three"}));
    CHECK(split("one\\two\\three\\", Platform::kWindows) == std::vector<std::string>({"one", "two", "three"}));
    CHECK(split("one\\two\\\\three\\", Platform::kWindows) == std::vector<std::string>({"one", "two", "three"}));
    CHECK(split("C:\\one\\two\\\\three\\", Platform::kWindows) == std::vector<std::string>({"C:", "one", "two", "three"}));
    CHECK(split("\\one\\two\\\\three\\", Platform::kWindows) == std::vector<std::string>({"\\", "one", "two", "three"}));

    CHECK(split("", Platform::kPOSIX) == std::vector<std::string>({}));
    CHECK(split("one/two/three", Platform::kPOSIX) == std::vector<std::string>({"one", "two", "three"}));
    CHECK(split("one/two/three/", Platform::kPOSIX) == std::vector<std::string>({"one", "two", "three"}));
    CHECK(split("/two//three/", Platform::kPOSIX) == std::vector<std::string>({"one", "two", "three"}));
    CHECK(split("/one/two/three/", Platform::kPOSIX) == std::vector<std::string>({"/", "one", "two", "three"}));

    // TODO: Support windows paths without drive letters.
}

TEST_CASE("pathutils::join") {
    CHECK(join({""}, '/') == "");
    CHECK(join({"hello"}, '/') == "hello");
    CHECK(join({"hello", "world"}, '/') == "hello/world");
    CHECK(join({"/hello", "world"}, '/') == "/hello/world");
    CHECK(join({"/hello/", "world"}, '/') == "/hello/world");
    CHECK(join({"/", "hello", "world"}, '/') == "/hello/world");
    CHECK(join({"..", ".."}, '/') == "../..");
    CHECK(join({"hello", "world"}, '\\') == "hello\\world");
    CHECK(join({"C:\\hello", "world"}, '\\') == "C:\\hello\\world");
    CHECK(join({"C:\\hello\\", "world"}, '\\') == "C:\\hello\\world");
    CHECK(join({"C:\\", "hello", "world"}, '\\') == "C:\\hello\\world");
    CHECK(join({"C:", "hello", "world"}, '\\') == "C:\\hello\\world");
    CHECK(join({"C:", "\\", "hello", "world"}, '\\') == "C:\\hello\\world");
    CHECK(join({"..", ".."}, '\\') == "..\\..");
}

TEST_CASE("pathutils::resolve_path") {

    // Posix.
    CHECK(resolve_path("/foo/bar/baz", "", Platform::kPOSIX) == "/foo/bar/baz");
    CHECK(resolve_path("/foo/bar/baz", "/one/two", Platform::kPOSIX) == "/foo/bar/baz");
    CHECK(resolve_path("baz", "/foo/bar", Platform::kPOSIX) == "/foo/bar/baz");
    CHECK(resolve_path("../baz", "/foo/bar", Platform::kPOSIX) == "/foo/baz");
    CHECK(resolve_path("../..", "/foo", Platform::kPOSIX) == "../..");  // TODO: Should this fail instead?
    CHECK(resolve_path("../../..", "/foo", Platform::kPOSIX) == "../../..");  // TODO: Should this fail instead?
    CHECK(resolve_path("../../..", "foo", Platform::kPOSIX) == "../..");
    CHECK(resolve_path("../../bar", "foo", Platform::kPOSIX) == "../bar");
    CHECK(resolve_path("..", "bob", Platform::kPOSIX) == ".");
    CHECK(resolve_path("../foo/../bar/../baz", "bob", Platform::kPOSIX) == "baz");

    // CHECK(pathutils::resolve_path("../foo/bar/../foo/../baz", "bob", pathutils::Platform::kPOSIX) == "baz");

    // Windows (and EPOC).
    CHECK(resolve_path("C:\\foo\\bar\\baz", "", Platform::kWindows) == "C:\\foo\\bar\\baz");
    CHECK(resolve_path("C:\\foo\\bar\\baz", "C:\\one\\two", Platform::kWindows) == "C:\\foo\\bar\\baz");
    CHECK(resolve_path("baz", "C:\\foo\\bar", Platform::kWindows) == "C:\\foo\\bar\\baz");
    CHECK(resolve_path("..\\baz", "D:\\foo\\bar", Platform::kWindows) == "D:\\foo\\baz");
    CHECK(resolve_path("..\\..", "C:\\foo", Platform::kWindows) == "..\\..");  // TODO: Should this fail instead?
    CHECK(resolve_path("..\\..\\..", "C:\\foo", Platform::kWindows) == "..\\..\\..");  // TODO: Should this fail instead?
    CHECK(resolve_path("..\\..\\..", "foo", Platform::kWindows) == "..\\..");
    CHECK(resolve_path("..\\..\\bar", "foo", Platform::kWindows) == "..\\bar");
    CHECK(resolve_path("..", "bob", Platform::kWindows) == ".");
    CHECK(resolve_path("..\\foo\\..\\bar\\..\\baz", "bob", Platform::kWindows) == "baz");
}
