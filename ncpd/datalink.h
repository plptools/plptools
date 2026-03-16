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
#pragma once

#include "config.h"

#include <stdio.h>
#include <pthread.h>

#include "bufferstore.h"
#include "bufferarray.h"

extern "C" {
    static void *data_pump_thread(void *);
}

class Link;

class DataLink
{
public:
    DataLink(const char *fname, int baud, Link *_link, unsigned short verbose, const int cancellationFd);
    ~DataLink();

    /**
     * Send a buffer out to serial line
     */
    void send(bufferStore &b, bool isEPOC);

    int getSpeed();
    bool linkFailed();
    void reset();

private:
    friend void * data_pump_thread(void *);

    inline void addToCrc(unsigned char a, unsigned short *crc) {
        *crc =  (*crc << 8) ^ crc_table[((*crc >> 8) ^ a) & 0xff];
    }

    /**
    * Reads the incoming data and processes data frames.
    *
    * @return true if the link is stable and more data can be consumed; false otherwise.
    */
    bool processInputData();

    void opByte(unsigned char a);

    /**
    * Signal the data pump thread that there is data to write and block until
    * there's space available.
    */
    void flushOutputBuffer();

    void internalReset();

    pthread_t dataPumpThreadId_;
    pthread_t ownerThreadId_;

    unsigned int crc_table[256];

    // The following sections represent groups of variables that comprise the state associated
    // with maintaining the underlying serial device, reading data from the serial device, and
    // writing data to the serial device. These should be treated as three distinct domains
    // wrt. concurrency and data within each group should be updated atomically.
    //
    // N.B. This thread-safety is not currently implemented.

    // Serial.

    int fd;
    int serialStatus = -1;
    int baudRateIndex_;
    int baudRate_;
    bool lastFatal = false;

    // Reading from serial.

    bool esc = false;
    bool justStarted = true;
    bufferStore rcv;
    int startPkt = -1;
    int lastSYN = -1;
    unsigned short crcIn = 0;
    unsigned short inCRCstate;
    unsigned short receivedCRC;
    unsigned char *inBuffer; int inWrite = 0; int inRead = 0;

    // Writing to serial.

    unsigned char *outBuffer; int outWrite = 0; int outRead = 0;

    // Initial configuration (const).

    Link * const link_;

    const std::string devname;

    /**
    * Requested baud rate; -1 indicates automatic.
    */
    const int requestedBaudRate_;

    /**
    * Used to signal cancellation.
    *
    * Should never be read.
    */
    const int cancellationFd_;
    const short int verbose_;
};
