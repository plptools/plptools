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

#include "siscomponentrecord.h"
#include "sisfile.h"
#include "plpintl.h"

#include <stdio.h>
#include <string.h>

SISComponentNameRecord::SISComponentNameRecord()
{
    m_names = NULL;
}

SISComponentNameRecord::~SISComponentNameRecord()
{
    if (m_names)
        {
        for (int i = 0; i < m_nameCount; ++i)
            delete[] m_names[i];
        delete[] m_names;
        }
}

SisRC
SISComponentNameRecord::fillFrom(uint8_t* buf, int* basePos, off_t len,
                                 SISFile* sisFile)
{
    int n = sisFile->m_header.m_nlangs;
    int base = *basePos;
    int entrySize = 8 + n * 4 * 2;
    if (base + entrySize > len)
        return SIS_TRUNCATED;
    *basePos += entrySize;

    uint8_t* p = buf + base;
    int size = 0;

    m_nameLengths = new uint32_t[n];
    m_namePtrs = new uint32_t[n];

    // First read lengths.
    //
    for (int i = 0; i < n; ++i)
        {
        m_nameLengths[i] = read32(p + size);
        if (m_nameLengths[i] > len)
            {
            printf(_("Length too large for name record %d.\n"), i);
            return SIS_TRUNCATED;
            }
        size += 4;
        }

    // Then read ptrs.
    //
    m_names = new uint8_t*[n];
    m_nameCount = n;
    for (int i = 0; i < n; ++i)
        {
        m_namePtrs[i] = read32(p + size);
        if (m_namePtrs[i] + m_nameLengths[i] > len)
            {
            printf(_("Position/length too large for name record %d.\n"), i);
            return SIS_TRUNCATED;
            }
        size += 4;
        if (logLevel >= 2)
            printf(_("Name %d (for %s) is %.*s\n"),
                   i,
                   sisFile->getLanguage(i)->m_name,
                   m_nameLengths[i],
                   buf + m_namePtrs[i]);
        int nlen = m_nameLengths[i];
        m_names[i] = new uint8_t[nlen + 1];
        memcpy(m_names[i], buf + m_namePtrs[i], nlen);
        m_names[i][nlen] = 0;
        }
    if (logLevel >= 1)
        printf(_("%d .. %d (%d bytes): Name records\n"), base, base + size, size);
    return SIS_OK;
}

uint32_t
SISComponentNameRecord::getLastEnd()
{
    uint32_t last = 0;
    for (int i = 0; i < m_nameCount; ++i)
        {
        uint32_t pos = m_namePtrs[i] + m_nameLengths[i];
        if (last < pos)
            last = pos;
        }
    return last;
}

uint8_t*
SISComponentNameRecord::getName(int no)
{
    return m_names[no];
}
