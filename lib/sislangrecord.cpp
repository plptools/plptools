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

#include "sislangrecord.h"
#include "plpintl.h"

#include <stdio.h>

SisRC
SISLangRecord::fillFrom(uint8_t* buf, int* base, off_t len)
{
    if (*base + 2 > len)
        return SIS_TRUNCATED;
    m_lang = read16(buf + *base);
    if (m_lang > 33)  // Thai, last language
        return SIS_CORRUPTED;
    if (logLevel >= 2)
        printf(_("Got language %d (%s)\n"), m_lang, langTable[m_lang].m_name);
    if (logLevel >= 1)
        printf(_("%d .. %d (%d bytes): Language record for %s\n"),
               *base, *base + 2, 2, langTable[m_lang].m_name);
    *base += 2;
    return SIS_OK;
}
