/*
 * This file is part of plptools.
 *
 *  Copyright (C) 1999 Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 2000, 2001 Fritz Elfert <felfert@to.com>
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
#include "config.h"

#include "iowatch.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <memory.h>

IOWatch::IOWatch() {
    num = 0;
    io = new int [FD_SETSIZE];
    memset(io, -1, FD_SETSIZE);
}

IOWatch::~IOWatch() {
    delete [] io;
}

void IOWatch::addIO(const int fd) {
    int pos;
    for (pos = 0; pos < num && fd < io[pos]; pos++);
    if (io[pos] == fd)
        return;
    for (int i = num; i > pos; i--)
        io[i] = io[i-1];
    io[pos] = fd;
    num++;
}

void IOWatch::remIO(const int fd) {
    int pos;
    for (pos = 0; pos < num && fd != io[pos]; pos++);
    if (pos != num) {
        num--;
        for (int i = pos; i <num; i++) io[i] = io[i+1];
    }
}

bool IOWatch::watch(const long secs, const long usecs) {
    if (num > 0) {
        int maxfd = 0;
        fd_set iop;
        FD_ZERO(&iop);
        for (int i = 0; i < num; i++) {
            FD_SET(io[i], &iop);
            if (io[i] > maxfd)
                maxfd = io[i];
        }
        struct timeval t;
        t.tv_usec = usecs;
        t.tv_sec = secs;
        return (select(maxfd+1, &iop, NULL, NULL, &t) > 0);
    }
    sleep(secs);
    usleep(usecs);
    return false;
}
