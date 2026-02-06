/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2002 Daniel Brahneborg <basic.chello@se>
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
 */
#include "config.h"

#include "sisreqrecord.h"
#include "sisfile.h"
#include "plpintl.h"

#include <stdio.h>

SisRC
SISReqRecord::fillFrom(uint8_t* buf, int* base, off_t len, SISFile* sisFile)
{
    int n = sisFile->m_header.m_nlangs;
    if (*base + 12 + n * 4 * 2 > len)
        return SIS_TRUNCATED;

    uint8_t* p = buf + *base;
    int size = 0;

    m_uid = read32(p);
    m_major = read16(p + 4);
    m_minor = read16(p + 6);
    m_variant = read32(p + 8);
    m_nameLengths = new uint32_t[n];
    m_namePtrs = new uint32_t[n];
    if (logLevel >= 2)
        printf(_("Requisite: uid=%08x, version=%d.%d-%d.\n"),
               m_uid, m_major, m_minor, m_variant);

    // First read lengths.
    //
    size = 12;
    for (int i = 0; i < n; ++i)
        {
        m_nameLengths[i] = read32(p + size);
        if (logLevel >= 2)
            printf(_("Got namelength %d\n"), m_nameLengths[i]);
        size += 4;
        }

    // Then read ptrs.
    //
    for (int i = 0; i < n; ++i)
        {
        m_namePtrs[i] = read32(p + size);
        if (logLevel >= 2)
            printf(_("Got namepos %d\n"), m_namePtrs[i]);
        if (m_namePtrs[i] + m_nameLengths[i] > len)
            {
            printf(_("Position/length too large for req record %d.\n"), i);
            return SIS_CORRUPTED;
            }
        size += 4;
        if (logLevel >= 2)
            printf(_("Name of requisite for %s is %.*s\n"),
                   sisFile->getLanguage(i)->m_name,
                   m_nameLengths[i],
                   buf + m_namePtrs[i]);
        }
    if (logLevel >= 1)
        printf(_("%d .. %d (%d bytes): Req record for uid %08x\n"),
               *base, *base + size, size, m_uid);
    *base += size;
    return SIS_OK;
}
