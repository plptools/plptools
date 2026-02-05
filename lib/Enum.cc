/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2000 Henner Zeller <hzeller@to.com>
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

#include <cstring>

#include "Enum.h"

using namespace std;

void EnumBase::i2sMapper::add(long i, const char* s) {
    stringMap.insert(pair<long, const char* const>(i, s));
}

string EnumBase::i2sMapper::lookup (long i) const {
    i2s_map_t::const_iterator searchPtr = stringMap.find(i);

    if (searchPtr == stringMap.end())
        return "[OUT-OF-RANGE]";
    /*
    * now combine the probably the multiple strings belonging to this
    * integer
    */
    string result;
    for (i = stringMap.count(i); i > 0 ; --i, ++searchPtr) {
        // this should be the case:
        assert(searchPtr != stringMap.end());
        if (result.length() != 0)
            result += string(",");
        result += string(searchPtr->second);
    }
    return result;
}


long EnumBase::i2sMapper::lookup (const char *s) const {
    /*
    * look up a specific string.
    * Since speed does not matter, we just do an exhaustive
    * search.
    * Otherwise we would have to maintain another map
    * mapping strings to ints .. but it's not worth the memory
    */
    i2s_map_t::const_iterator run = stringMap.begin();
    while (run != stringMap.end() && strcmp(s, run->second)) {
        ++run;
    }
    if (run == stringMap.end())
        return  -1;
    return run->first;
}

bool EnumBase::i2sMapper::inRange (long i) const {
    return (stringMap.find(i) != stringMap.end());
}
