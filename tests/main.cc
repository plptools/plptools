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

#include <cli_utils.h>

#include <stdlib.h>
#include <stdio.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("cli_utils::is_number") {

    CHECK(cli_utils::is_number("12987"));
    CHECK(!cli_utils::is_number(""));
    CHECK(!cli_utils::is_number("aslue"));
    CHECK(!cli_utils::is_number("12dsd"));
    CHECK(!cli_utils::is_number("dsd2323"));
    CHECK(!cli_utils::is_number("230xsd2039"));

}

TEST_CASE("cli_utils::lookup_default_port") {

    SUBCASE("default value is 7501") {
        CHECK(cli_utils::lookup_default_port() == DPORT);
    }

}

TEST_CASE("cli_utils::parse_port") {

    std::string host = "127.0.0.1";
    int port = 7501;

    SUBCASE("100") {
        CHECK(cli_utils::parse_port("100", &host, &port) == true);
        CHECK(host == "127.0.0.1");
        CHECK(port == 100);
    }

    SUBCASE("127.0.0.4") {
        CHECK(cli_utils::parse_port("127.0.0.4", &host, &port) == true);
        CHECK(host == "127.0.0.4");
        CHECK(port == 7501);
    }

    SUBCASE("192.168.0.1:3498") {
        CHECK(cli_utils::parse_port("192.168.0.1:3498", &host, &port) == true);
        CHECK(host == "192.168.0.1");
        CHECK(port == 3498);
    }

    SUBCASE("empty string") {
        CHECK(cli_utils::parse_port("", &host, &port) == true);
        CHECK(host == "127.0.0.1");
        CHECK(port == 7501);
    }

    SUBCASE("null result arguments") {
        CHECK(cli_utils::parse_port("127.0.0.1", nullptr, &port) == false);
        CHECK(cli_utils::parse_port("127.0.0.1", &host, nullptr) == false);
    }

    SUBCASE("missing host") {
        CHECK(cli_utils::parse_port(":12", &host, &port) == false);
    }

    SUBCASE("valid host and non-numeric port") {
        CHECK(cli_utils::parse_port("127.0.0.1:230a3", &host, &port) == false);
    }

    SUBCASE("unqualified hostname") {
        CHECK(cli_utils::parse_port("computer", &host, &port) == true);
        CHECK(host == "computer");
        CHECK(port == 7501);
    }

    SUBCASE("empty port") {
        CHECK(cli_utils::parse_port("127.0.0.1:", &host, &port) == false);
    }

}
