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

#ifndef _SISFILE_H
#define _SISFILE_H

#include "sistypes.h"
#include "sisfileheader.h"
#include "siscomponentrecord.h"

class SISLangRecord;
class SISFileRecord;
class SISReqRecord;

/**
 * The top level container of a SIS file.
 * Based on documentation by Alexander Thoukydides <alex@thouky.co.uk>.
 */
class SISFile
{
public:

    SISFile();

    virtual ~SISFile();

    /**
     * Compare uid and version number of this file, with another.
     *
     * @see SISFileHeader::compareApp()
     */
    SisRC compareApp(SISFile* other);

    /**
     * Populate the fields.
     *
     * @param buf The buffer to read from.
     * @param len The length of the buffer.
     */
    SisRC fillFrom(uint8_t* buf, off_t len);

    /**
     * Return the currently selected installation language.
     */
    int getLanguage();

    /**
     * Find a language entry, based on the sequence number in the SISLangRecord
     * part of the file.
     */
    LangTableEntry* getLanguage(int i);

    /**
     * Get the name of this component, in the selected language.
     */
    uint8_t* getName();

    /**
     * Get the number of bytes that should be copied to the residual sis
     * file on the psion.
     */
    uint32_t getResidualEnd()
        {
        return m_end;
        }

    void ownBuffer()
        {
        m_ownBuffer = true;
        }

    /**
     * Is this the same application?
     */
    bool sameApp(SISFile* other);

    /**
     * Set the installed drive.
     */
    void setDrive(char drive);

    /**
     * Set the number of installed files.
     */
    void setFiles(int nFiles);

    /**
     * Set the selected installation language.
     */
    void setLanguage(int lang);

    SISFileHeader m_header;
    SISLangRecord* m_langRecords;
    SISFileRecord* m_fileRecords;
    SISReqRecord* m_reqRecords;

private:

    SISComponentNameRecord m_componentRecord;

    bool m_ownBuffer;

    uint8_t* m_buf;

    uint32_t m_end;

    void updateEnd(uint32_t pos);

};

#endif
