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

    CHECK(is_absolute("/", PathFormat::kPOSIX) == true);
    CHECK(is_absolute("\\", PathFormat::kWindows) == false);

    CHECK(is_absolute("C:\\", PathFormat::kWindows) == true);
    CHECK(is_absolute("C:\\", PathFormat::kPOSIX) == false);

    CHECK(is_absolute("C:", PathFormat::kWindows) == false);
    CHECK(is_absolute("C:", PathFormat::kPOSIX) == false);
    CHECK(is_absolute("C:foo", PathFormat::kWindows) == false);
    CHECK(is_absolute("C:foo", PathFormat::kPOSIX) == false);

    CHECK(is_absolute("", PathFormat::kWindows) == false);
    CHECK(is_absolute("", PathFormat::kPOSIX) == false);

    CHECK(is_absolute("foo", PathFormat::kWindows) == false);
    CHECK(is_absolute("foo", PathFormat::kPOSIX) == false);
    CHECK(is_absolute("foo\\bar", PathFormat::kWindows) == false);
    CHECK(is_absolute("foo/bar", PathFormat::kPOSIX) == false);

    CHECK(is_absolute("\\C:\\", PathFormat::kWindows) == false);
    CHECK(is_absolute("/C:/", PathFormat::kPOSIX) == true);
}

TEST_CASE("pathutils::appending_components") {
    CHECK(appending_components("C:\\", {"Documents"}, PathFormat::kWindows) == "C:\\Documents");
    CHECK(appending_components("C:\\", {"Documents"}, PathFormat::kWindows) == "C:\\Documents");
    CHECK(appending_components("C:foo\\bar\\", {"baz"}, PathFormat::kWindows) == "C:foo\\bar\\baz");
}

TEST_CASE("pathutils::split") {
    CHECK(split("", PathFormat::kWindows) == std::vector<std::string>({}));
    CHECK(split(".", PathFormat::kWindows) == std::vector<std::string>({"."}));
    CHECK(split(".\\", PathFormat::kWindows) == std::vector<std::string>({".", ""}));
    CHECK(split("one\\two\\three", PathFormat::kWindows) == std::vector<std::string>({"one", "two", "three"}));
    CHECK(split("one\\two\\three\\", PathFormat::kWindows) == std::vector<std::string>({"one", "two", "three", ""}));
    CHECK(split("one\\two\\\\three\\", PathFormat::kWindows) == std::vector<std::string>({"one", "two", "three", ""}));
    CHECK(split("C:\\one\\two\\\\three\\", PathFormat::kWindows) == std::vector<std::string>({"C:", "\\", "one", "two", "three", ""}));
    CHECK(split("\\one\\two\\\\three\\", PathFormat::kWindows) == std::vector<std::string>({"\\", "one", "two", "three", ""}));
    CHECK(split("C:\\one\\two\\\\three\\", PathFormat::kWindows) == std::vector<std::string>({"C:", "\\", "one", "two", "three", ""}));
    CHECK(split("C:foo\\bar\\", PathFormat::kWindows) == std::vector<std::string>({"C:", "foo", "bar", ""}));
    CHECK(split("C:\\", PathFormat::kWindows) == std::vector<std::string>({"C:", "\\"}));

    CHECK(split("", PathFormat::kPOSIX) == std::vector<std::string>({}));
    CHECK(split(".", PathFormat::kPOSIX) == std::vector<std::string>({"."}));
    CHECK(split("./", PathFormat::kPOSIX) == std::vector<std::string>({".", ""}));
    CHECK(split("one/two/three", PathFormat::kPOSIX) == std::vector<std::string>({"one", "two", "three"}));
    CHECK(split("one/two/three/", PathFormat::kPOSIX) == std::vector<std::string>({"one", "two", "three", ""}));
    CHECK(split("/two//three/", PathFormat::kPOSIX) == std::vector<std::string>({"/", "two", "three", ""}));
    CHECK(split("/one/two/three/", PathFormat::kPOSIX) == std::vector<std::string>({"/", "one", "two", "three", ""}));
    CHECK(split("/", PathFormat::kPOSIX) == std::vector<std::string>({"/"}));
}

TEST_CASE("pathutils::join") {
    CHECK(join({}, PathFormat::kPOSIX) == "");
    CHECK(join({"."}, PathFormat::kPOSIX) == ".");
    CHECK(join({".", ""}, PathFormat::kPOSIX) == "./");
    CHECK(join({""}, PathFormat::kPOSIX) == "./");
    CHECK(join({"hello"}, PathFormat::kPOSIX) == "hello");
    CHECK(join({"hello", "world"}, PathFormat::kPOSIX) == "hello/world");
    CHECK(join({"/hello", "world"}, PathFormat::kPOSIX) == "/hello/world");
    CHECK(join({"/hello/", "world"}, PathFormat::kPOSIX) == "/hello/world");
    CHECK(join({"/", "hello", "world"}, PathFormat::kPOSIX) == "/hello/world");
    CHECK(join({"..", ".."}, PathFormat::kPOSIX) == "../..");

    CHECK(join({}, PathFormat::kWindows) == "");
    CHECK(join({"."}, PathFormat::kWindows) == ".");
    CHECK(join({".", ""}, PathFormat::kWindows) == ".\\");
    CHECK(join({"C:", ""}, PathFormat::kWindows) == "C:.\\");
    CHECK(join({"C:", ".", ""}, PathFormat::kWindows) == "C:.\\");
    CHECK(join({"hello", "world"}, PathFormat::kWindows) == "hello\\world");
    CHECK(join({"C:\\hello", "world"}, PathFormat::kWindows) == "C:\\hello\\world");
    CHECK(join({"C:\\hello\\", "world"}, PathFormat::kWindows) == "C:\\hello\\world");
    CHECK(join({"C:\\", "hello", "world"}, PathFormat::kWindows) == "C:\\hello\\world");
    CHECK(join({"C:", "hello", "world"}, PathFormat::kWindows) == "C:hello\\world");
    CHECK(join({"C:", "\\", "hello", "world"}, PathFormat::kWindows) == "C:\\hello\\world");
    CHECK(join({"..", ".."}, PathFormat::kWindows) == "..\\..");

    // Trailing directory marker.
    CHECK(join({""}, PathFormat::kWindows) == ".\\");
    CHECK(join({"\\", ""}, PathFormat::kWindows) == "\\");
    CHECK(join({"C:", ""}, PathFormat::kWindows) == "C:.\\");
    CHECK(join({"C:", "\\", ""}, PathFormat::kWindows) == "C:\\");
    CHECK(join({"C:", "\\", "mnt\\", ""}, PathFormat::kWindows) == "C:\\mnt\\");
    CHECK(join({"C:", "\\", "mnt", ""}, PathFormat::kWindows) == "C:\\mnt\\");
    CHECK(join({""}, PathFormat::kPOSIX) == "./");
    CHECK(join({"/", ""}, PathFormat::kPOSIX) == "/");
    CHECK(join({"/", "mnt", ""}, PathFormat::kPOSIX) == "/mnt/");
    CHECK(join({"/", "mnt/", ""}, PathFormat::kPOSIX) == "/mnt/");
}

TEST_CASE("pathutils::resolve_path") {

    // Posix.

    CHECK(resolve_path("/foo/bar/baz", "", PathFormat::kPOSIX) == "/foo/bar/baz");
    CHECK(resolve_path("/foo/bar/baz", "/one/two", PathFormat::kPOSIX) == "/foo/bar/baz");

    CHECK(resolve_path("", "/foo/bar/", PathFormat::kPOSIX) == "/foo/bar/");

    CHECK(resolve_path("baz", "/foo/bar", PathFormat::kPOSIX) == "/foo/bar/baz");
    CHECK(resolve_path("../baz", "/foo/bar", PathFormat::kPOSIX) == "/foo/baz");
    CHECK(resolve_path("../..", "/foo", PathFormat::kPOSIX) == "/..");
    CHECK(resolve_path("../../..", "/foo", PathFormat::kPOSIX) == "/../..");

    CHECK(resolve_path("../../..", "foo", PathFormat::kPOSIX) == "../..");
    CHECK(resolve_path("../../bar", "foo", PathFormat::kPOSIX) == "../bar");
    CHECK(resolve_path("..", "bob", PathFormat::kPOSIX) == ".");
    CHECK(resolve_path("../foo/../bar/../baz", "bob", PathFormat::kPOSIX) == "baz");

    CHECK(resolve_path("", "", PathFormat::kPOSIX) == ".");
    CHECK(resolve_path("./", "", PathFormat::kPOSIX) == "./");
    CHECK(resolve_path("../", "foo", PathFormat::kPOSIX) == "./");

    // Windows.

    CHECK(resolve_path("C:\\foo\\bar\\baz", "", PathFormat::kWindows) == "C:\\foo\\bar\\baz");
    CHECK(resolve_path("C:\\foo\\bar\\baz", "C:\\one\\two", PathFormat::kWindows) == "C:\\foo\\bar\\baz");

    CHECK(resolve_path("baz", "C:\\foo\\bar", PathFormat::kWindows) == "C:\\foo\\bar\\baz");
    CHECK(resolve_path("..\\baz", "D:\\foo\\bar", PathFormat::kWindows) == "D:\\foo\\baz");
    CHECK(resolve_path("..\\..", "C:\\foo", PathFormat::kWindows) == "C:\\..");
    CHECK(resolve_path("..\\..\\..", "C:\\foo", PathFormat::kWindows) == "C:\\..\\..");

    CHECK(resolve_path("baz", "\\foo\\bar", PathFormat::kWindows) == "\\foo\\bar\\baz");
    CHECK(resolve_path("..\\baz", "\\foo\\bar", PathFormat::kWindows) == "\\foo\\baz");
    CHECK(resolve_path("..\\..", "\\foo", PathFormat::kWindows) == "\\..");
    CHECK(resolve_path("..\\..\\..", "\\foo", PathFormat::kWindows) == "\\..\\..");

    CHECK(resolve_path("baz", "\\foo\\bar\\", PathFormat::kWindows) == "\\foo\\bar\\baz");
    CHECK(resolve_path("..\\baz", "\\foo\\bar\\", PathFormat::kWindows) == "\\foo\\baz");
    CHECK(resolve_path("..\\..", "\\foo\\", PathFormat::kWindows) == "\\..");
    CHECK(resolve_path("..\\..\\..", "\\foo\\", PathFormat::kWindows) == "\\..\\..");

    CHECK(resolve_path("baz", "C:foo\\bar", PathFormat::kWindows) == "C:foo\\bar\\baz");
    CHECK(resolve_path("..\\baz", "C:foo\\bar", PathFormat::kWindows) == "C:foo\\baz");
    CHECK(resolve_path("..\\..", "C:foo", PathFormat::kWindows) == "C:..");
    CHECK(resolve_path("..\\..\\..", "C:foo", PathFormat::kWindows) == "C:..\\..");

    CHECK(resolve_path("..\\..\\..", "foo", PathFormat::kWindows) == "..\\..");
    CHECK(resolve_path("..\\..\\bar", "foo", PathFormat::kWindows) == "..\\bar");
    CHECK(resolve_path("..", "bob", PathFormat::kWindows) == ".");

    CHECK(resolve_path("", "", PathFormat::kWindows) == ".");
    CHECK(resolve_path(".\\", "", PathFormat::kWindows) == ".\\");
    CHECK(resolve_path("..\\", "", PathFormat::kWindows) == "..\\");
    CHECK(resolve_path("..\\", "foo", PathFormat::kWindows) == ".\\");

    CHECK(resolve_path("C:\\foo\\bar", "D:\\baz", PathFormat::kWindows) == "C:\\foo\\bar");
    CHECK(resolve_path("C:", "D:", PathFormat::kWindows) == "C:");
    CHECK(resolve_path("\\foo\\bar", "D:\\baz", PathFormat::kWindows) == "D:\\foo\\bar");
    CHECK(resolve_path("\\foo\\bar", "D:", PathFormat::kWindows) == "D:\\foo\\bar");
    CHECK(resolve_path("C:\\foo\\bar", "D:\\baz", PathFormat::kWindows) == "C:\\foo\\bar");
    CHECK(resolve_path("C:\\baz", "D:", PathFormat::kWindows) == "C:\\baz");
}
