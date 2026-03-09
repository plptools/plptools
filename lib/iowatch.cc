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

#include <mutex>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <memory.h>

IOWatch::IOWatch() {
    fileDescriptorCount_ = 0;
    fileDescriptors_ = new int [FD_SETSIZE];
    memset(fileDescriptors_, -1, FD_SETSIZE);
}

IOWatch::~IOWatch() {
    delete [] fileDescriptors_;
}

void IOWatch::addIO(const int fd) {
    std::lock_guard<std::mutex> lock(lock_);
    int pos;
    for (pos = 0; pos < fileDescriptorCount_ && fd < fileDescriptors_[pos]; pos++);
    if (fileDescriptors_[pos] == fd)
        return;
    for (int i = fileDescriptorCount_; i > pos; i--)
        fileDescriptors_[i] = fileDescriptors_[i-1];
    fileDescriptors_[pos] = fd;
    fileDescriptorCount_++;
}

void IOWatch::remIO(const int fd) {
    std::lock_guard<std::mutex> lock(lock_);
    int pos;
    for (pos = 0; pos < fileDescriptorCount_ && fd != fileDescriptors_[pos]; pos++);
    if (pos != fileDescriptorCount_) {
        fileDescriptorCount_--;
        for (int i = pos; i <fileDescriptorCount_; i++) fileDescriptors_[i] = fileDescriptors_[i+1];
    }
}

bool IOWatch::watch(const long secs, const long usecs) {
    int maxfd = 0;
    fd_set iop;
    FD_ZERO(&iop);
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (int i = 0; i < fileDescriptorCount_; i++) {
            FD_SET(fileDescriptors_[i], &iop);
            if (fileDescriptors_[i] > maxfd) {
                maxfd = fileDescriptors_[i];
            }
        }
    }
    struct timeval t;
    t.tv_usec = usecs;
    t.tv_sec = secs;
    return select(maxfd + 1, &iop, NULL, NULL, &t) > 0;
}
