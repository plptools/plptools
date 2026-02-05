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

#ifndef _SISFILEHEADER_H
#define _SISFILEHEADER_H

#include "sistypes.h"

/**
 * The first part of a SISFile.
 *
 * This file header is referenced from most other parts of the sis file,
 * mainly since it contains the list of languages.
 */
class SISFileHeader
{
public:

    /**
     * Compare uid and version number of this file, with another.
     */
    SisRC compareApp(SISFileHeader* other);

    /**
     * Populate the fields.
     *
     * @param buf The buffer to read from.
     * @param base The index to start reading from, which is updated
     *   when the header is successfully read.
     * @param len The length of the buffer.
     */
    SisRC fillFrom(uint8_t* buf, int* base, off_t len);

    /**
     * Update the drive letter, and patch the parsed buffer.
     */
    void setDrive(char drive);

    /**
     * Update the number of installed files, and patch the parsed buffer.
     */
    void setFiles(int nFiles);

    enum FileOptions {
        op_isUnicode = 1,
        op_isDistributable = 2
#ifdef EPOC6
        ,
        op_noCompress = 8,
        op_shutdownApps = 16
#endif
    };

    enum FileType {
        FT_App = 0
#ifdef EPOC6
        ,
        FT_System = 1,
        FT_Option = 2,
        FT_Config = 3,
        FT_Patch = 4,
        FT_Upgrade = 5
#endif
    };

    uint32_t m_uid1;
    uint32_t m_uid2;
    uint32_t m_uid3;
    uint32_t m_uid4;
    uint16_t m_crc;
    uint16_t m_nlangs;
    uint16_t m_nfiles;
    uint16_t m_nreqs;
    uint16_t m_installationLanguage;
    uint16_t m_installationFiles;
    uint32_t m_installationDrive;
    uint32_t m_installerVersion;
    uint16_t m_options;
    uint16_t m_type;
    uint16_t m_major;
    uint16_t m_minor;
    uint32_t m_variant;
    uint32_t m_languagePtr;
    uint32_t m_filesPtr;
    uint32_t m_reqPtr;
    uint32_t m_unknown;
    uint32_t m_componentPtr;

private:

    uint8_t* m_buf;

};

#endif
