/*
 * This file is part of plptools.
 *
 *  Copyright (C) 2002 Daniel Brahneborg <basic@chello.se>
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

#ifndef _SISINSTALLER_H
#define _SISINSTALLER_H

#include "sistypes.h"

#include <sys/types.h>

class Psion;
class SISFile;
class SISFileLink;
class SISFileRecord;

/**
 * A minimal SIS installer.
 * Handles recursive sis files.
 */
class SISInstaller
{
public:

    SISInstaller();

    virtual ~SISInstaller();

    SisRC run(SISFile* file, uint8_t* buf, off_t len);

    SisRC run(SISFile* file, uint8_t* buf, off_t len, SISFile* parent);

    /**
     * Ask the user which drive to install to.
     */
    void selectDrive();

    /**
     * Set the base pointer to the list of already installed
     * applications, so we don't have to scan it for every sis
     * component.
     */
    void setInstalled(SISFileLink* installed)
        {
        m_installed = installed;
        }

    /**
     * Set the Psion manager.
     */
    void setPsion(Psion* psion);

private:

    char m_drive;

    int m_fileNo;

    Psion* m_psion;

    uint8_t* m_buf;

    SISFile* m_file;

    SISFileLink* m_installed;

    int m_lastSisFile;

    bool m_ownInstalled;

    enum {
        FILE_OK,
        FILE_SKIP,
        FILE_ABORT,
    };

    /**
     * Store the contents of a buffer in a file on the Psion.
     */
    void copyBuf(const uint8_t* buf, int len, char* name);

    /**
     * Copy a file to the Psion.
     */
    void copyFile(SISFileRecord* fileRecord);

    void createDirs(char* filename);

    int installFile(SISFileRecord* fileRecord);

    SisRC loadInstalled();

    void loadPsionSis(const char* name);

    void removeFile(SISFileRecord* fileRecord);

    void uninstall(SISFile* sisFile);

    void uninstallFile(SISFileRecord* fileRecord);

};

#endif

