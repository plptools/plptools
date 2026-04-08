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

TEST_CASE("pathutils::ensuring_trailing_separator") {

    CHECK(pathutils::ensuring_trailing_separator("", pathutils::kEPOCSeparator) == "\\");
    CHECK(pathutils::ensuring_trailing_separator("\\", pathutils::kEPOCSeparator) == "\\");
    CHECK(pathutils::ensuring_trailing_separator("C:", pathutils::kEPOCSeparator) == "C:\\");
    CHECK(pathutils::ensuring_trailing_separator("C:\\", pathutils::kEPOCSeparator) == "C:\\");

    // N.B. These tests assume a POSIX host and will need updating for future Windows support.
    CHECK(pathutils::ensuring_trailing_separator("", pathutils::kHostSeparator) == "/");
    CHECK(pathutils::ensuring_trailing_separator("/", pathutils::kHostSeparator) == "/");
    CHECK(pathutils::ensuring_trailing_separator("/mnt", pathutils::kHostSeparator) == "/mnt/");
    CHECK(pathutils::ensuring_trailing_separator("/mnt/", pathutils::kHostSeparator) == "/mnt/");

    // Unsupported path normalization and separator conversion.
    CHECK(pathutils::ensuring_trailing_separator("/mnt/", pathutils::kEPOCSeparator) == "/mnt/\\");
    CHECK(pathutils::ensuring_trailing_separator("C:\\", pathutils::kHostSeparator) == "C:\\/");
    CHECK(pathutils::ensuring_trailing_separator("C:\\\\", pathutils::kEPOCSeparator) == "C:\\\\");
    CHECK(pathutils::ensuring_trailing_separator("/mnt//", pathutils::kHostSeparator) == "/mnt//");
}

TEST_CASE("pathutils::epoc_basename") {
    CHECK(pathutils::epoc_basename("C:\\Random") == "Random");
    CHECK(pathutils::epoc_basename("C:\\Documents\\foo.txt") == "foo.txt");

    // Legacy behavior (might be a bug).
    CHECK(pathutils::epoc_basename("C:\\Random\\") == "");
}

// TODO: Rename to get parent path?
TEST_CASE("pathutils::epoc_dirname") {
    SUBCASE("C:\\Random") {
        std::string result;
        result = pathutils::epoc_dirname("C:\\Random");
        CHECK(result == "C:\\");
    }
    SUBCASE("C:\\Random\\") {
        std::string result;
        result = pathutils::epoc_dirname("C:\\Random\\");
        CHECK(result == "C:\\");
    }
    SUBCASE("C:\\Documents\\foo.txt") {
        std::string result;
        result = pathutils::epoc_dirname("C:\\Documents\\foo.txt");
        CHECK(result == "C:\\Documents");
    }
    SUBCASE("C:\\") {
        std::string result;
        result = pathutils::epoc_dirname("C:\\");
        CHECK(result == "C:");
    }

    // Legacy behavior (might be a bug).
    SUBCASE("C:") {
        std::string result;
        result = pathutils::epoc_dirname("C:");
        CHECK(result == "C");
    }
}

TEST_CASE("pathutils::is_absolute") {

    CHECK(pathutils::is_absolute("C:\\", '\\') == true);
    CHECK(pathutils::is_absolute("C:\\", '/') == false);

    CHECK(pathutils::is_absolute("C:", '\\') == true);
    CHECK(pathutils::is_absolute("C:", '/') == false);

    CHECK(pathutils::is_absolute("", '\\') == false);
    CHECK(pathutils::is_absolute("", '/') == false);

    CHECK(pathutils::is_absolute("foo", '\\') == false);
    CHECK(pathutils::is_absolute("foo", '/') == false);
    CHECK(pathutils::is_absolute("foo\\bar", '\\') == false);
    CHECK(pathutils::is_absolute("foo/bar", '/') == false);

    CHECK(pathutils::is_absolute("\\C:\\", '\\') == false);
    CHECK(pathutils::is_absolute("/C:/", '/') == true);
}

TEST_CASE("pathutils::appending_components") {
    CHECK(pathutils::appending_components("C:\\", {"Documents"}, '\\') == "C:\\Documents");
    CHECK(pathutils::appending_components("C:\\", {"Documents"}, '\\') == "C:\\Documents");
}

TEST_CASE("pathutils::split") {
    CHECK(pathutils::split("", '\\') == std::vector<std::string>({}));
    CHECK(pathutils::split("one\\two\\three", '\\') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(pathutils::split("one\\two\\three\\", '\\') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(pathutils::split("one\\two\\\\three\\", '\\') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(pathutils::split("C:\\one\\two\\\\three\\", '\\') == std::vector<std::string>({"C:", "one", "two", "three"}));

    CHECK(pathutils::split("", '/') == std::vector<std::string>({}));
    CHECK(pathutils::split("one/two/three", '/') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(pathutils::split("one/two/three/", '/') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(pathutils::split("one/two//three/", '/') == std::vector<std::string>({"one", "two", "three"}));
    CHECK(pathutils::split("/one/two/three/", '/') == std::vector<std::string>({"/", "one", "two", "three"}));
}

TEST_CASE("pathutils::join") {
    CHECK(pathutils::join({""}, '/') == "");
    CHECK(pathutils::join({"hello"}, '/') == "hello");
    CHECK(pathutils::join({"hello", "world"}, '/') == "hello/world");
    CHECK(pathutils::join({"/hello", "world"}, '/') == "/hello/world");
    CHECK(pathutils::join({"/", "hello", "world"}, '/') == "/hello/world");
    CHECK(pathutils::join({"hello", "world"}, '\\') == "hello\\world");
    CHECK(pathutils::join({"C:\\hello", "world"}, '\\') == "C:\\hello\\world");
    CHECK(pathutils::join({"C:\\", "hello", "world"}, '\\') == "C:\\hello\\world");
    CHECK(pathutils::join({"C:", "hello", "world"}, '\\') == "C:\\hello\\world");
}

TEST_CASE("pathutils::resolve_path") {

    // Posix.
    CHECK(pathutils::resolve_path("/foo/bar/baz", "", '/') == "/foo/bar/baz");
    CHECK(pathutils::resolve_path("/foo/bar/baz", "/one/two", '/') == "/foo/bar/baz");
    CHECK(pathutils::resolve_path("baz", "/foo/bar", '/') == "/foo/bar/baz");
    CHECK(pathutils::resolve_path("../baz", "/foo/bar", '/') == "/foo/baz");

    // Windows (and EPOC).
    CHECK(pathutils::resolve_path("C:\\foo\\bar\\baz", "", '\\') == "C:\\foo\\bar\\baz");
    CHECK(pathutils::resolve_path("C:\\foo\\bar\\baz", "C:\\one\\two", '\\') == "C:\\foo\\bar\\baz");
    CHECK(pathutils::resolve_path("baz", "C:\\foo\\bar", '\\') == "C:\\foo\\bar\\baz");
    CHECK(pathutils::resolve_path("..\\baz", "D:\\foo\\bar", '\\') == "D:\\foo\\baz");
}
