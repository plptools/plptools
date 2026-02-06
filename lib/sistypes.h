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

#ifndef _SISTYPES_H
#define _SISTYPES_H

#include <stdint.h>
#include <sys/types.h>

/**
 * Return Codes.
 */
enum SisRC {
    SIS_OK = 0,
    SIS_TRUNCATED,
    SIS_TRUNCATEDDATA,
    SIS_CORRUPTED,
    SIS_FAILED,
    SIS_ABORTED,
    SIS_DIFFERENT_APP,
    SIS_VER_EARLIER,
    SIS_SAME_OR_LATER,
    SIS_OTHER_VARIANT
};

extern uint16_t read16(uint8_t* p);

extern uint32_t read32(uint8_t* p);

extern void write16(uint8_t* p, int val);

extern void createCRCTable();

extern uint16_t updateCrc(uint16_t crc, uint8_t value);

extern int logLevel;

/**
 * Holder of a language entry, translating from language numbers to
 * names.
 */
struct LangTableEntry
{
    uint16_t m_no;
    char   m_code[3];
    const char*  m_name;
};

extern LangTableEntry langTable[];

#endif
