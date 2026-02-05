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

#include "sisfileheader.h"
#include "plpintl.h"

#include <stdio.h>
#include <stdlib.h>

const int OFF_NUMBER_OF_FILES = 26;
const int OFF_INSTALLATION_DRIVE = 28;

SisRC
SISFileHeader::compareApp(SISFileHeader* other)
{
        if (m_uid1 != other->m_uid1)
                return SIS_DIFFERENT_APP;
        if ((m_major < other->m_major) ||
                ((m_major == other->m_major) &&
                 (m_minor < other->m_minor)))
                return SIS_VER_EARLIER;
        if ((m_major == other->m_major) &&
                (m_minor == other->m_minor) &&
                (m_variant != other->m_variant))
                return SIS_OTHER_VARIANT;
        return SIS_SAME_OR_LATER;
}

SisRC
SISFileHeader::fillFrom(uint8_t* buf, int* base, off_t len)
{
        if (*base + 68 > len)
                return SIS_TRUNCATED;
        uint8_t* start = buf + *base;
        m_buf = buf;
        m_uid1 = read32(start);
        if (logLevel >= 1)
                printf(_("Got uid1 = %08x\n"), m_uid1);
        m_uid2 = read32(start + 4);
        if (m_uid2 != 0x1000006d)
                {
                printf("%s", _("Got bad uid2.\n"));
                return SIS_CORRUPTED;
                }
        if (logLevel >= 2)
                printf(_("Got uid2 = %08x\n"), m_uid2);
        m_uid3 = read32(start + 8);
        if (m_uid3 != 0x10000419)
                {
                printf("%s", _("Got bad uid3.\n"));
                return SIS_CORRUPTED;
                }
        if (logLevel >= 2)
                printf(_("Got uid3 = %08x\n"), m_uid3);
        m_uid4 = read32(start + 12);
        if (logLevel >= 2)
                printf(_("Got uid4 = %08x\n"), m_uid4);
        uint16_t crc1 = 0;
        for (int i = 0; i < 12; i += 2)
                crc1 = updateCrc(crc1, buf[*base + i]);
        uint16_t crc2 = 0;
        for (int i = 0; i < 12; i += 2)
                crc2 = updateCrc(crc2, buf[*base + i + 1]);
        if (logLevel >= 2)
                printf(_("Got first crc = %08x, wanted %08x\n"),
                           crc2 << 16 | crc1, m_uid4);
        if ((crc2 << 16 | crc1) != m_uid4)
                {
                printf("%s", _("Got bad crc.\n"));
                return SIS_CORRUPTED;
                }
        m_crc = read16(start + 16);
        m_nlangs = read16(start + 18);
        if (logLevel >= 2)
                printf(_("Got %d languages\n"), m_nlangs);
        m_nfiles = read16(start + 20);
        if (logLevel >= 2)
                printf(_("Got %d files\n"), m_nfiles);
        m_nreqs = read16(start + 22);
        if (logLevel >= 2)
                printf(_("Got %d reqs\n"), m_nreqs);
        m_installationLanguage = read16(start + 24);
        if (logLevel >= 2)
                printf(_("Selected language is %d\n"), m_installationLanguage);
        m_installationFiles = read16(start + OFF_NUMBER_OF_FILES);
        if (logLevel >= 2)
                printf(_("Installed files: %d / %d\n"), m_installationFiles, m_nfiles);
        m_installationDrive = read32(start + OFF_INSTALLATION_DRIVE);
        if (logLevel >= 2)
                printf(_("Installed on drive %c\n"), m_installationDrive);
        m_installerVersion = read32(start + 32);
        if (logLevel >= 2)
                printf(_("Got installer version: %08x\n"), m_installerVersion);
        m_options = read16(start + 36);
        if (logLevel >= 2)
                printf(_("Got options: %04x\n"), m_options);
        m_type = read16(start + 38);
        if (logLevel >= 2)
                printf(_("Got type: %04x\n"), m_type);
        m_major = read16(start + 40);
        if (logLevel >= 2)
                printf(_("Got major: %d\n"), m_major);
        m_minor = read16(start + 42);
        if (logLevel >= 2)
                printf(_("Got minor: %d\n"), m_minor);
        m_variant = read32(start + 44);
        if (logLevel >= 2)
                printf(_("Got variant: %d\n"), m_variant);
        m_languagePtr = read32(start + 48);
        if (logLevel >= 2)
                printf(_("Languages begin at %d\n"), m_languagePtr);
//    if (m_languagePtr >= len)
//        return SIS_TRUNCATED;
        m_filesPtr = read32(start + 52);
        if (logLevel >= 2)
                printf(_("Files begin at %d\n"), m_filesPtr);
//    if (m_filesPtr >= len)
//        return SIS_TRUNCATED;
        m_reqPtr = read32(start + 56);
        if (logLevel >= 2)
                printf(_("Requisites begin at %d\n"), m_reqPtr);
//    if (m_reqPtr >= len)
//        return SIS_TRUNCATED;
        m_unknown = read32(start + 60);
        m_componentPtr = read32(start + 64);
        if (logLevel >= 2)
                printf(_("Components begin at %d\n"), m_componentPtr);
//    if (m_componentPtr >= len)
//        return SIS_TRUNCATED;
        *base += 68;
        return SIS_OK;
}

void
SISFileHeader::setDrive(char drive)
{
        m_installationDrive = drive;
        m_buf[OFF_INSTALLATION_DRIVE] = drive;
        m_buf[OFF_INSTALLATION_DRIVE + 1] =
                m_buf[OFF_INSTALLATION_DRIVE + 2] =
                m_buf[OFF_INSTALLATION_DRIVE + 3] = 0;
}

void
SISFileHeader::setFiles(int nFiles)
{
        m_installationFiles = nFiles;
        write16(m_buf + OFF_NUMBER_OF_FILES, nFiles);
}
