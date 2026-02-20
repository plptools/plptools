/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *  Copyright (C) 2026 Jason Morley <hello@jbmorley.co.uk>
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
#ifndef _LOG_H_
#define _LOG_H_

#include <cstdio>
#include <iostream>

#include <syslog.h>

/**
 * A streambuffer, logging via syslog
 *
 * logbuf can be used, if you want to use syslog for
 * logging but don't want to change all your nice
 * C++-style output statements in your code.
 *
 * Here is an example showing the usage of logbuf:
 *
 * <PRE>
 *     openlog("myDaemon", LOG_CONS|LOG_PID, LOG_DAEMON);
 *     logbuf ebuf(LOG_ERR, 2);
 *     ostream lerr(&ebuf);
 *
 *     ... some code ...
 *
 *     lerr << "Whoops, got an error" << endl;
 * </PRE>
 *
 * Logging can be toggled between using syslog and writing to a file (see `useSyslog` and `useFileDescriptor`).
 */
class logbuf : public std::streambuf {
public:

    /**
    * Constructs a new instance.
    *
    * @param loglevel The log level for this instance.
    *   see syslog(3) for symbolic names to use.
    * @param fd An optional file descriptor to use
    *   if switched off.
    */
    logbuf(int loglevel, int fd);

    /**
     * Write logs using syslog.
     *
     * When writing to syslog is enabled, logs will not be written to the file descriptor passed in the constructor.
     */
    void useSyslog() { _use_syslog = true; }

    /**
     * Write logs to the file descriptor passed in the constructor.
     *
     * When writing to the file descriptor, logs will not be written using syslog. If a file descriptor of -1 was
     * specified, no logs will be written.
     */
    void useFileDescriptor() { _use_syslog = false; }

    /**
    * Modifies the loglevel of this instance.
    *
    * @param newlevel The new loglevel.
    */
    void setLevel(int newlevel) { _level = newlevel; }

    /**
    * Called by the associated
    * ostream to write a character.
    * Stores the character in a buffer
    * and calls syslog(level, buffer)
    * whenever a LF is seen.
    */
    int overflow(int c = EOF);

private:

    /**
    * Pointer to next char in buffer.
    */
    char *ptr;

    /**
    * Current length of buffer.
    */
    unsigned int len;

    /**
    * The log level to use with syslog.
    */
    int _level;

    /**
    * File descriptor to use when switched off.
    * If this is -1, don't output anything.
    */
    int _fd;

    /**
    * Log flag.
    */
    bool _use_syslog;

    /**
    * The internal buffer for holding
    * messages.
    */
    char buf[1024];
};

#endif
