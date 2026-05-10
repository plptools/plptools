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
#include "optional.h"

class Dummy {
public:
    Dummy(std::string value)
    : value_(value) {};

    std::string value() {
        return value_;
    }

private:
    std::string value_;
};

TEST_CASE("pathutils::epoc_basename") {
    std::string hello = "hello";
    auto foo = Optional<std::string>(hello);
    CHECK(foo.hasValue());

    Optional<std::string> bar("Hello, World");
    CHECK(bar.value() == "Hello, World");

    
    auto c = Optional<std::string>();
    auto d = Optional<uint8_t>(12);

    Optional<std::string> e;
    Optional<std::string> f{};
    Optional<uint8_t> g(8);
    Optional<uint8_t> h;

    Optional<std::string> hey;
    CHECK(!hey.hasValue());

    auto moveFrom = std::make_unique<std::string>("Hello");
    hey = std::move(*moveFrom);


    auto z = Optional<std::string>("Randodm!");
    hey = z;
    hey = Optional<std::string>("Bob");

    hey = Optional<std::string>("cheese it!");

    c = bar;

    auto optionalDummy = Optional<Dummy>(Dummy("cheese"));
    CHECK(optionalDummy->value() == "cheese");
    CHECK(optionalDummy.hasValue());


    SUBCASE("regular constructor") {
        Optional<std::string> a("Hello, World!");
        CHECK(*a == "Hello, World!");
        auto b = Optional<std::string>(a);
        CHECK(*b == "Hello, World!");
    }

    // TODO: Regular constructor?
}
