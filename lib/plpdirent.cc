/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
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
#include "rfsv.h"

#include "plpdirent.h"

#include "pathutils.h"

#include <cstdint>
#include <iomanip>

using namespace std;

PlpUID::PlpUID() {
    memset(uid, 0, sizeof(uid));
}

PlpUID::PlpUID(const uint32_t u1, const uint32_t u2, const uint32_t u3) {
    uid[0] = u1; uid[1] = u2; uid[2] = u3;
}

uint32_t PlpUID::operator[](int idx) {
    assert ((idx > -1) && (idx < 3));
    return uid[idx];
}

PlpDirent::PlpDirent()
: size(0)
, attr(0)
, UID()
, time(time_t(0))
, attrstr("")
, dirname_("")
, name("") {
}

PlpDirent::PlpDirent(const uint32_t _size,
                     const uint32_t _attr,
                     const uint32_t tHi,
                     const uint32_t tLo,
                     const std::string &dirname,
                     const char * const _name)
: size(_size)
, attr(_attr)
, UID()
, time(tHi, tLo)
, attrstr("")
, dirname_(dirname)
, name(_name) {
}

uint32_t PlpDirent::getSize() const {
    return size;
}

uint32_t PlpDirent::getAttr() const {
    return attr;
}

bool PlpDirent::isDirectory() const {
    return (attr & RFSV::PSI_A_DIR) > 0;
}

uint32_t PlpDirent::getUID(int uididx) {
    if ((uididx >= 0) && (uididx < 4))
        return UID[uididx];
    return 0;
}

PlpUID &PlpDirent::getUID() {
    return UID;
}

std::string PlpDirent::getPath() const {
    std::string path = pathutils::ensuring_trailing_separator(dirname_, pathutils::kEPOCSeparator) + name;
    if (isDirectory()) {
        return pathutils::ensuring_trailing_separator(path, pathutils::kEPOCSeparator);
    } else {
        return path;
    }
}

const char *PlpDirent::getName() const {
    return name.c_str();
}

PsiTime PlpDirent::getPsiTime() {
    return time;
}

void PlpDirent::setName(const char *str) {
    name = str;
}

ostream &operator<<(ostream &o, const PlpDirent &e) {
    ostream::fmtflags old = o.flags();

    o << e.attrstr << " " << dec << setw(10)
      << setfill(' ') << e.size << " " << e.time
      << " " << e.name;
    o.flags(old);
    return o;
}
