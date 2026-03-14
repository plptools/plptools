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
#ifndef _packet_h
#define _packet_h

#include "config.h"
#include <stdio.h>
#include <pthread.h>

#include "bufferstore.h"
#include "bufferarray.h"

extern "C" {
    static void *pump_run(void *);
}

class Link;

class packet
{
public:
    packet(const char *fname, int baud, Link *_link, unsigned short verbose, const int cancellationFd);
    ~packet();

    /**
     * Send a buffer out to serial line
     */
    void send(bufferStore &b);

    void setEpoc(bool);
    int getSpeed();
    bool linkFailed();
    void reset();

private:
    friend void * pump_run(void *);

    inline void addToCrc(unsigned char a, unsigned short *crc) {
        *crc =  (*crc << 8) ^ crc_table[((*crc >> 8) ^ a) & 0xff];
    }

    void findSync();
    void opByte(unsigned char a);
    void opCByte(unsigned char a, unsigned short *crc);
    void realWrite();
    void internalReset();

    Link *link_;
    pthread_t dataPumpThreadId_;
    pthread_t ownerThreadId_;
    unsigned int crc_table[256];

    unsigned short crcOut = 0;
    unsigned short crcIn = 0;
    unsigned short receivedCRC;
    unsigned short inCRCstate;

    unsigned char *inBuffer;
    int inWrite = 0;
    int inRead = 0;

    unsigned char *outBuffer;
    int outWrite = 0;
    int outRead = 0;

    int startPkt = -1;
    int lastSYN = -1;

    bufferArray inQueue;
    bufferStore rcv;
    int fd;
    int serialStatus = -1;
    int baud_index;
    int realBaud;
    short int verbose;
    bool esc = false;
    bool lastFatal = false;
    bool isEPOC = false;
    bool justStarted = true;

    char *devname;
    int baud;

    const int cancellationFd;
};

#endif
