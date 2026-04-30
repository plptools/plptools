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

TEST_CASE("DeviceConfiguration accessors") {
    std::string id = uuid::uuid4();
    DeviceConfiguration configuration(id, "My Psion");
    CHECK(configuration.id() == id);
    CHECK(configuration.name() == "My Psion");
}

TEST_CASE("DeviceConfiguration::serialize") {
    DeviceConfiguration configuration("12345", "My Psion");
    CHECK(configuration.serialize() == "id = 12345\r\nname = My Psion\r\n");
}

TEST_CASE("DeviceConfiguration::deserialize") {

    SUBCASE("non-empty name") {
        DeviceConfiguration configuration("123456", "My Psion");
        std::string stringRepresentation = configuration.serialize();
        auto result = DeviceConfiguration::deserialize(stringRepresentation);
        CHECK(result->id() == "123456");
        CHECK(result->name() == "My Psion");
    }

    SUBCASE("empty name") {
        DeviceConfiguration configuration("123456", "");
        std::string stringRepresentation = configuration.serialize();
        auto result = DeviceConfiguration::deserialize(stringRepresentation);
        CHECK(result->id() == "123456");
        CHECK(result->name() == "");
    }
}
