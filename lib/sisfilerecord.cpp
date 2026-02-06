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

#include "sisfilerecord.h"
#include "sisfile.h"
#include "plpintl.h"

#include <stdio.h>

SisRC
SISFileRecord::fillFrom(uint8_t* buf, int* base, off_t len, SISFile* sisFile)
{
    if (*base + 28 + 4 * 2 > len)
        return SIS_TRUNCATED;

    SisRC rc = SIS_OK;
    m_buf = buf;
    m_len = len;
    uint8_t* p = buf + *base;
    int size = 0;
    m_flags = read32(p);
    if (logLevel >= 2)
        printf(_("Got flags %d\n"), m_flags);
    m_fileType = read32(p + 4);
    if (logLevel >= 2)
        printf(_("Got file type %d\n"), m_fileType);
    m_fileDetails = read32(p + 8);
    if (logLevel >= 2)
        printf(_("Got file details %d\n"), m_fileDetails);
    m_sourceLength = read32(p + 12);
    m_sourcePtr = read32(p + 16);
//    printf(_("Got source length = %d, source name ptr = %d\n"),
//           m_sourceLength, m_sourcePtr);
    if (logLevel >= 2)
        if (m_sourceLength > 0)
            printf(_("Got source name %.*s\n"), m_sourceLength, buf + m_sourcePtr);
    m_destLength = read32(p + 20);
    m_destPtr = read32(p + 24);
//    printf(_("Got dest length = %d, dest name ptr = %d\n"),
//           m_destLength, m_destPtr);
    if (logLevel >= 2)
        printf(_("Got destination name %.*s\n"), m_destLength, buf + m_destPtr);
    size = 28;
    switch (m_flags)
        {
        case 0: // Only one file.
            m_fileLengths = new uint32_t[1];
            m_filePtrs = new uint32_t[1];
            m_fileLengths[0] = read32(p + size);
            m_filePtrs[0] = read32(p + size + 4);
            size += 8;
            if (logLevel >= 2)
                printf(_("File is %d bytes long (at %d) (to %d)\n"),
                       m_fileLengths[0], m_filePtrs[0],
                       m_fileLengths[0] + m_filePtrs[0]);
            if (logLevel >= 1)
                printf(_("%d .. %d (%d bytes): Single file record type %d, %.*s\n"),
                       m_filePtrs[0],
                       m_filePtrs[0] + m_fileLengths[0],
                       m_fileLengths[0],
                       m_fileType,
                       m_destLength, buf + m_destPtr);
            break;

        case 1: // One file per language.
            {
            int n = sisFile->m_header.m_nlangs;
            m_fileLengths = new uint32_t[n];
            m_filePtrs = new uint32_t[n];
            if (*base + size + n * 8 > len)
                return SIS_TRUNCATED;
            for (int i = 0; i < n; ++i)
                {
                m_fileLengths[i] = read32(p + size);
//                if (m_fileLengths[i] > len)
//                    rc = SIS_TRUNCATEDDATA;
                size += 4;
                }
            for (int i = 0; i < n; ++i)
                {
                m_filePtrs[i] = read32(p + size);
                int fileLen = m_fileLengths[i];
//                if (m_filePtrs[i] + fileLen > len)
//                    rc = SIS_TRUNCATEDDATA;
                size += 4;
                if (logLevel >= 2)
                    printf(_("File %d (for %s) is %d bytes long (at %d)\n"),
                           i,
                           sisFile->getLanguage(i)->m_name,
                           fileLen,
                           m_filePtrs[i]);
                if (logLevel >= 1)
                    printf(_("%d .. %d (%d bytes): File record (%s) for %.*s\n"),
                           m_filePtrs[i],
                           m_filePtrs[i] + fileLen,
                           fileLen,
                           sisFile->getLanguage(i)->m_name,
                           m_destLength, buf + m_destPtr);
                }
            break;
            }

        default:
            if (logLevel >= 2)
                printf(_("Unknown file flags %d\n"), m_flags);
        }
    *base += size;
    return rc;
}

uint8_t*
SISFileRecord::getFilePtr(int fileNo)
{
    if (fileNo < 0)
        return 0;
    if (m_filePtrs[fileNo] >= m_len)
        return 0;
    return &m_buf[m_filePtrs[fileNo]];
}

void
SISFileRecord::setMainDrive(char drive)
{
    if (m_buf[m_destPtr] == '!')
        m_buf[m_destPtr] = drive;
}
