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

#include "config.h"

#include "serialtransport.h"

#include <sys/ioctl.h>
#include <termios.h>

#include "mp_serial.h"

SerialTransport::SerialTransport(const char *dev, int speed, int debug)
: dev_(dev),
  speed_(speed),
  debug_(debug),
  fd_(-1) {
    printf("SerialTransport::SerialTransport");
}

SerialTransport::~SerialTransport() {
    if (fd_ >= -1) {
        ser_exit(fd_);
    }
}

int SerialTransport::open() {
    printf("SerialTransport::open");
    fd_ = init_serial(dev_.c_str(), speed_, debug_);
    return fd_;
}

void SerialTransport::close() {
    ser_exit(fd_);
    fd_ = -1;
}

bool SerialTransport::isConnected() const {
    return fd_ >= 0;
}

LinkStatus SerialTransport::refreshLink() {
    int arg;

    if (fd_ < 0) {
        return LinkStatus::FATAL;
    }

    // Get the status.
    if (ioctl(fd_, TIOCMGET, &arg) < 0) {
        return LinkStatus::FATAL;
    }

    // Set DTR and RTS if they're not set.
    if (!((arg & TIOCM_RTS) && (arg & TIOCM_DTR))) {
        arg |= (TIOCM_DTR | TIOCM_RTS);
        if (ioctl(fd_, TIOCMSET, &arg) < 0) {
            return LinkStatus::FATAL;
        }
    }

    // Check DSR to see if the device is still connected.
    // TODO: Check for a solution on Solaris.
    if ((arg & TIOCM_DSR) == 0) {
        return LinkStatus::FAILED;
    }

    return LinkStatus::OK;
}

int SerialTransport::getFd() const {
    return fd_;
}
