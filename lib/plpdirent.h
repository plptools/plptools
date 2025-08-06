/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
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
#ifndef _PLPDIRENT_H_
#define _PLPDIRENT_H_

#include <iostream>
#include <string>
#include <cstring>

#include "psitime.h"
#include "rfsv.h"

/**
 * A class, representing the UIDs of a file on the Psion.
 * Every File on the Psion has a unique UID for determining
 * the application-mapping. This class stores these UIDs.
 * An object of this class is contained in every @ref PlpDirent
 * object.
 *
 * @author Fritz Elfert <felfert@to.com>
 */
class PlpUID
{
    friend inline bool operator<(const PlpUID &u1, const PlpUID &u2);
public:
    /**
    * Default constructor.
    */
    PlpUID();

    /**
    * Constructor.
    * Create an instance, presetting all thre uid values.
    */
    PlpUID(const uint32_t u1, const uint32_t u2, const uint32_t u3);

    /**
    * Retrieve a UID value.
    *
    * @param idx The index of the desired UID. Range must be (0..2),
    *            otherwise an assertion is triggered.
    */
    uint32_t operator[](int idx);

private:
    long uid[3];
};

inline bool operator<(const PlpUID &u1, const PlpUID &u2) {
    return (memcmp(u1.uid, u2.uid, sizeof(u1.uid)) < 0);
}

/**
 * A class, representing a directory entry of the Psion.
 * Objects of this type are used by @ref rfsv::readdir ,
 * @ref rfsv::dir and @ref rfsv::fgeteattr for returning
 * the entries of a directory.
 *
 * @author Fritz Elfert <felfert@to.com>
 */
class PlpDirent {
    friend class rfsv32;
    friend class rfsv16;

public:
    /**
    * Default constructor
    */
    PlpDirent();

    /**
    * A copy constructor.
    * Mainly used by STL container classes.
    *
    * @param d The object to be used as initializer.
    */
    PlpDirent(const PlpDirent &d);

    /**
    * Initializing Constructor
    */
    PlpDirent(const uint32_t size, const uint32_t attr, const uint32_t tHi,
	      const uint32_t tLo, const char * const name);

    /**
    * Default destructor.
    */
    ~PlpDirent() {};

    /**
    * Retrieves the file size of a directory entry.
    *
    * @returns The file size in bytes.
    */
    uint32_t getSize();

    /**
    * Retrieves the file attributes of a directory entry.
    *
    * @returns The generic attributes ( @ref rfsv:file_attribs ).
    */
    uint32_t getAttr();

    /**
    * Retrieves the UIDs of a directory entry.
    * This method returns always 0 with a Series3.
    *
    * @param uididx The index of the UID to retrieve (0 .. 2).
    *
    * @returns The selected UID or 0 if the index is out of range.
    */
    uint32_t getUID(int uididx);

    /**
    * Retrieves the @ref PlpUID object of a directory entry.
    *
    * @returns The PlpUID object.
    */
    PlpUID &getUID();

    /**
    * Retrieve the file name of a directory entry.
    *
    * @returns The name of the file.
    */
    const char *getName();

    /**
    * Retrieve the modification time of a directory entry.
    *
    * @returns A @ref PsiTime object, representing the time.
    */
    PsiTime getPsiTime();

    /**
    * Set the file name of a directory entry.
    * This is currently unused. It does NOT
    * change the name of the corresponding file on
    * the Psion.
    *
    * @param str The new name of the file.
    */
    void setName(const char *str);

    /**
    * Assignment operator
    * Mainly used by STL container classes.
    *
    * @param e The new value to assign.
    *
    * @returns The modified object.
    */
    PlpDirent &operator=(const PlpDirent &e);

    /**
    * Prints the object contents.
    * The output is in human readable similar to the
    * output of a "ls" command.
    */
    friend std::ostream &operator<<(std::ostream &o, const PlpDirent &e);

private:
    uint32_t size;
    uint32_t attr;
    PlpUID  UID;
    PsiTime time;
    std::string  attrstr;
    std::string  name;
};

/**
 * A class representing information about
 * a Disk drive on the psion. An Object of this type
 * is used by @ref rfsv::devinfo for returning the
 * information of the probed drive.
 *
 * @author Fritz Elfert <felfert@to.com>
 */
class PlpDrive {
    friend class rfsv32;
    friend class rfsv16;

public:
    /**
    * Default constructor.
    */
    PlpDrive();

    /**
    * Copy constructor
    */
    PlpDrive(const PlpDrive &other);

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
    uint32_t getMediaType();

    /**
    * Retrieve the media type of the drive.
    * Just like the above function, but returns
    * the media type as human readable string.
    *
    * @param ret The string is returned here.
    */
    void getMediaType(std::string &ret);

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
    uint32_t getDriveAttribute();

    /**
    * Retrieve the attributes of the drive.
    * Just like the above function, but returns
    * the attributes as human readable string.
    *
    * @param ret The string is returned here.
    */
    void getDriveAttribute(std::string &ret);

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
    uint32_t getMediaAttribute();

    /**
    * Retrieve the attributes of the media.
    * Just like the above function, but returns
    * the attributes as human readable string.
    *
    * @param ret The string is returned here.
    */
    void getMediaAttribute(std::string &ret);

    /**
    * Retrieve the UID of the drive.
    * Each drive, except the ROM drive on a Psion has
    * a unique ID which can be retrieved here.
    *
    * @returns The UID of the probed drive.
    */
    uint32_t getUID();

    /**
    * Retrieve the total capacity of the drive.
    *
    * @returns The capacity of the probed drive in bytes.
    */
    uint64_t getSize();

    /**
    * Retrieve the free capacity on the drive.
    *
    * @returns The free space on the probed drive in bytes.
    */
    uint64_t getSpace();

    /**
    * Retrieve the volume name of the drive.
    *
    * returns The volume name of the drive.
    */
    std::string getName();

    /**
    * Retrieve the drive letter of the drive.
    *
    * returns The letter of the probed drive.
    */
    char getDrivechar();

private:
    void setMediaType(uint32_t type);
    void setDriveAttribute(uint32_t attr);
    void setMediaAttribute(uint32_t attr);
    void setUID(uint32_t uid);
    void setSize(uint32_t sizeLo, uint32_t sizeHi);
    void setSpace(uint32_t spaceLo, uint32_t spaceHi);
    void setName(char drive, const char * const volname);

    uint32_t mediatype;
    uint32_t driveattr;
    uint32_t mediaattr;
    uint32_t uid;
    uint64_t size;
    uint64_t space;
    char drivechar;
    std::string name;
};

#endif
