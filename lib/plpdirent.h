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
 * Objects of this type are used by @ref RFSV::readdir ,
 * @ref RFSV::dir and @ref RFSV::fgeteattr for returning
 * the entries of a directory.
 *
 * @author Fritz Elfert <felfert@to.com>
 */
class PlpDirent {
    friend class RFSV32;
    friend class RFSV16;

public:
    /**
    * Default constructor
    */
    PlpDirent();

    /**
    * A copy constructor.
    * Mainly used by STL container classes.
    *
    * @param e The object to be used as initializer.
    */
    PlpDirent(const PlpDirent &e) = default;

    /**
    * Initializing Constructor
    */
    PlpDirent(const uint32_t size,
              const uint32_t attr,
              const uint32_t tHi,
              const uint32_t tLo,
              const std::string &dirname,
              const char *const name);

    /**
    * Default destructor.
    */
    ~PlpDirent() {};

    /**
    * Retrieves the file size of a directory entry.
    *
    * @returns The file size in bytes.
    */
    uint32_t getSize() const;

    /**
    * Retrieves the file attributes of a directory entry.
    *
    * @returns The generic attributes ( @ref RFSV::file_attribs ).
    */
    uint32_t getAttr() const;

    /**
    * Determine if the directory entry represents a directory.
    *
    * @return true if the directory entry is itself a directory; false otherwise.
    */
    bool isDirectory() const;

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

    std::string getPath() const;

    /**
    * Retrieve the file name of a directory entry.
    *
    * @returns The name of the file.
    */
    const char *getName() const;

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
    PlpDirent &operator=(const PlpDirent &e) = default;

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
    std::string attrstr;
    std::string dirname_;
    std::string name;
};
