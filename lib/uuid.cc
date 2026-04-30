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

#include "uuid.h"

#include <array>
#include <cstdint>
#include <cstdio>

constexpr size_t kUUID4Size = 16;

std::string uuid::uuid4() {
    std::array<uint8_t, kUUID4Size> b;

    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) {
        fprintf(stderr, "Failed to open /dev/urandom while generating UUID4.\n");
        abort();
    }

    int count = fread(b.data(), 1, kUUID4Size, f);
    fclose(f);
    if (count != kUUID4Size) {
        fprintf(stderr, "Failed to read from /dev/urandom while generating UUID4.\n");
        abort();
    }

    // Mask the version and variant into the random data.
    b[6] = (b[6] & 0x0F) | 0x40;  // 4
    b[8] = (b[8] & 0x3F) | 0x80;  // RFC 4122

    char buf[37];
    snprintf(buf, sizeof(buf),
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             b[0],  b[1],  b[2],  b[3],
             b[4],  b[5],  b[6],  b[7],
             b[8],  b[9],  b[10], b[11],
             b[12], b[13], b[14], b[15]);

    return std::string(buf);
}
