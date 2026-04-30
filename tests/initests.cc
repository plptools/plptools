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

#include <iostream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>

#include "doctest.h"
#include "deviceconfiguration.h"
#include "pathutils.h"
#include "uuid.h"
#include "ini.h"

TEST_CASE("parse_ini") {

    SUBCASE("empty string") {
        auto contents = ini::deserialize("");
        REQUIRE(contents != nullptr);
        CHECK(contents->empty());
    }

    SUBCASE("multiple new lines") {
        auto contents = ini::deserialize("\r\n\r\n\r\n");
        REQUIRE(contents != nullptr);
        CHECK(contents->empty());
    }

    SUBCASE("corrupt fails") {
        REQUIRE(ini::deserialize("broken") == nullptr);
        REQUIRE(ini::deserialize("a=b\nbroken") == nullptr);
    }

    SUBCASE("missing key fails") {
        REQUIRE(ini::deserialize("=value\n") == nullptr);
    }

    SUBCASE("non-alpha key fails") {
        REQUIRE(ini::deserialize("some2=value\n") == nullptr);
    }

    SUBCASE("single value") {
        auto contents = ini::deserialize("key = bar\r\n");
        REQUIRE(contents != nullptr);
        CHECK(*contents == std::unordered_map<std::string, std::string>{
            {"key", "bar"}
        });
    }

    SUBCASE("empty value") {
        auto contents = ini::deserialize("key=\r\n");
        REQUIRE(contents != nullptr);
        CHECK(*contents == std::unordered_map<std::string, std::string>{
            {"key", ""}
        });
    }

    SUBCASE("missing newline") {
        auto contents = ini::deserialize("key = bar");
        REQUIRE(contents != nullptr);
        CHECK(*contents == std::unordered_map<std::string, std::string>{
            {"key", "bar"}
        });
    }

    SUBCASE("ignore trailing whitespace with missing newline") {
        auto contents = ini::deserialize("key = bar  ");
        REQUIRE(contents != nullptr);
        CHECK(*contents == std::unordered_map<std::string, std::string>{
            {"key", "bar"}
        });
    }

    SUBCASE("ignore trailing whitespace") {
        auto contents = ini::deserialize("key = bar  \n");
        REQUIRE(contents != nullptr);
        CHECK(*contents == std::unordered_map<std::string, std::string>{
            {"key", "bar"}
        });
    }

    SUBCASE("ignore trailing whitespace (windows line ending)") {
        auto contents = ini::deserialize("key = bar  \r\n");
        REQUIRE(contents != nullptr);
        CHECK(*contents == std::unordered_map<std::string, std::string>{
            {"key", "bar"}
        });
    }
}
