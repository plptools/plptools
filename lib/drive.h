/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
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
#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <cstring>

#include "psitime.h"
#include "rfsv.h"

enum class MediaType: uint32_t {
    kNotPresent = 0,
    kUnknown = 1,
    kFloppy = 2,
    kDisk = 3,
    kCompactDisc = 4,
    kRAM = 5,
    kFlashDisk = 6,
    kROM = 7,
    kRemote = 8,
};

/**
 * A class representing information about
 * a Disk drive on the psion. An Object of this type
 * is used by @ref RFSV::devinfo for returning the
 * information of the probed drive.
 *
 * @author Fritz Elfert <felfert@to.com>
 */
class Drive {
    friend class RFSV32;
    friend class RFSV16;

public:
    /**
    * Default constructor.
    */
    Drive();

    Drive(MediaType mediaType,
          uint32_t driveAttributes,
          uint32_t mediaAttributes,
          uint32_t uid,
          uint64_t size,
          uint64_t space,
          char driveLetter,
          std::string name);

    /**
    * Copy constructor.
    */
    Drive(const Drive &other) = default;

    /**
    * Assignment operator.
    */
    Drive &operator=(const Drive &other) = default;

    /**
    * Retrieve the media type of the drive.
    *
    * @returns The media type of the probed drive.
    * <pre>
    * Media types are encoded by a number
    * in the range 0 .. 8 with the following
    * meaning:
    *
    *   0 = Not present
    *   1 = Unknown
    *   2 = Floppy
    *   3 = Disk
    *   4 = CD-ROM
    *   5 = RAM
    *   6 = Flash Disk
    *   7 = ROM
    *   8 = Remote
    * </pre>
    */
    MediaType getMediaType() const;

    /**
    * Retrieve the attributes of the drive.
    *
    * @returns The attributes of the probed drive.
    * <pre>
    * Drive attributes are encoded by a number
    * in the range 0 .. 63. The bits have the
    * the following meaning:
    *
    *   bit 0 = local
    *   bit 1 = ROM
    *   bit 2 = redirected
    *   bit 3 = substituted
    *   bit 4 = internal
    *   bit 5 = removable
    * </pre>
    */
    uint32_t getDriveAttributes() const;

    /**
    * Retrieve the attributes of the media.
    *
    * @returns The attributes of the probed media.
    * <pre>
    * Media attributes are encoded by a number
    * in the range 0 .. 15. The bits have the
    * following meaning:
    *
    *   bit 0 = variable size
    *   bit 1 = dual density
    *   bit 2 = formattable
    *   bit 3 = write protected
    * </pre>
    */
    uint32_t getMediaAttributes() const;

    /**
    * Retrieve the UID of the drive.
    * Each drive, except the ROM drive on a Psion has
    * a unique ID which can be retrieved here.
    *
    * @returns The UID of the probed drive.
    */
    uint32_t getUID() const;

    /**
    * Retrieve the total capacity of the drive.
    *
    * @returns The capacity of the probed drive in bytes.
    */
    uint64_t getSize() const;

    /**
    * Retrieve the free capacity on the drive.
    *
    * @returns The free space on the probed drive in bytes.
    */
    uint64_t getSpace() const;

    /**
    * Retrieve the volume name of the drive.
    *
    * returns The volume name of the drive.
    */
    std::string getName() const;

    /**
    * Retrieve the drive letter of the drive.
    *
    * returns The letter of the probed drive.
    */
    char getDriveLetter() const;

    /**
    * Get the file system path given by the current drive.
    *
    * @return the full path of the current drive (e.g., "C:\\" for the drive "C").
    */
    std::string getPath() const;

private:
    void setMediaType(MediaType type);
    void setDriveAttributes(uint32_t driveAttribute);
    void setMediaAttributes(uint32_t mediaAttribute);
    void setUID(uint32_t uid);
    void setSize(uint32_t sizeLo, uint32_t sizeHi);
    void setSpace(uint32_t spaceLo, uint32_t spaceHi);
    void setName(char drive, const char * const volname);

    MediaType mediaType_;
    uint32_t driveAttributes_;
    uint32_t mediaAttributes_;
    uint32_t uid_;
    uint64_t size_;
    uint64_t space_;
    char driveLetter_;
    std::string name_;
};
