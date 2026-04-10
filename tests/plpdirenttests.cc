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
#include "plpdirent.h"
#include "rfsv.h"

TEST_CASE("PlpDirent::getPath") {

    SUBCASE("Directory name with no trailing backslash") {
        PlpDirent dirent(0, 0, 0, 0, "C:\\Projects", "foo.txt");
        CHECK(dirent.isDirectory() == false);
        std::string name = dirent.getName();
        CHECK(name == "foo.txt");
        CHECK(dirent.getPath() == "C:\\Projects\\foo.txt");
    }

    SUBCASE("Directory name with trailing backslash") {
        PlpDirent dirent(0, 0, 0, 0, "C:\\Projects\\", "foo.txt");
        CHECK(dirent.isDirectory() == false);
        std::string name = dirent.getName();
        CHECK(name == "foo.txt");
        CHECK(dirent.getPath() == "C:\\Projects\\foo.txt");
    }

    SUBCASE("Empty directory name") {
        PlpDirent dirent(0, 0, 0, 0, "", "foo.txt");
        CHECK(dirent.isDirectory() == false);
        std::string name = dirent.getName();
        CHECK(name == "foo.txt");
        CHECK(dirent.getPath() == "foo.txt");
    }

    SUBCASE("Directory dirent must have trailing backslash") {
        PlpDirent dirent(0, RFSV::PSI_A_DIR, 0, 0, "M:\\", "FOO");
        CHECK(dirent.isDirectory() == true);
        std::string name = dirent.getName();
        CHECK(name == "FOO");
        CHECK(dirent.getPath() == "M:\\FOO\\");
    }

}
