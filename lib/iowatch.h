/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
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
#ifndef _IOWATCH_H_
#define _IOWATCH_H_

/**
 * A simple wrapper for select()
 *
 * IOWatch is a simple wrapper for the select
 * system call. In particular, it takes care
 * of passing the maximum file descriptor
 * argument (arg 1) of select() correctly.
 * IOWatch handles select on read descriptors only.
 */
class IOWatch {
public:
    /**
    * Creates a new instance.
    */
    IOWatch();

    /**
    * Destroys an instance.
    */
    ~IOWatch();

    /**
    * Adds a file descriptor to
    * the set of descriptors.
    *
    * @param fd The file descriptor to add.
    */
    void addIO(const int fd);

    /**
    * Removes a file descriptor from the
    * set of descriptors.
    *
    * @param fd The file descriptor to remove.
    */
    void remIO(const int fd);

    /**
    * Performs a select() call.
    *
    * @param secs Number of seconds to wait.
    * @param usecs Number of microseconds to wait.
    *
    * @return true, if any of the descriptors is readable.
    */
    bool watch(const long secs, const long usecs);

private:
    int num;
    int *io;
};

#endif
