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
#include "drive.h"
#include "rfsv.h"

TEST_CASE("Drive::getPath") {

    SUBCASE("M") {
        Drive drive(MediaType::kRAM, 0, 0, 0, 0, 0, 'M', "Drive");
        CHECK(drive.getPath() == "M:\\");
    }

    SUBCASE("C") {
        Drive drive(MediaType::kRAM, 0, 0, 0, 0, 0, 'C', "Drive");
        CHECK(drive.getPath() == "C:\\");
    }

}
