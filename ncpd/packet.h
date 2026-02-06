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

#define PKT_DEBUG_LOG       16
#define PKT_DEBUG_DUMP      32
#define PKT_DEBUG_HANDSHAKE 64

extern "C" {
    static void *pump_run(void *);
}

class Link;

class packet
{
public:
    packet(const char *fname, int baud, Link *_link, unsigned short verbose = 0);
    ~packet();

    /**
     * Send a buffer out to serial line
     */
    void send(bufferStore &b);

    void setEpoc(bool);
    void setVerbose(short int);
    short int getVerbose();
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

    Link *theLINK;
    pthread_t datapump;
    pthread_t thisThread;
    unsigned int   crc_table[256];

    unsigned short crcOut;
    unsigned short crcIn;
    unsigned short receivedCRC;
    unsigned short inCRCstate;

    unsigned char *inBuffer;
    int inWrite;
    int inRead;

    unsigned char *outBuffer;
    int outWrite;
    int outRead;

    int startPkt;
    int lastSYN;

    bufferArray inQueue;
    bufferStore rcv;
    int foundSync;
    int fd;
    int serialStatus;
    int baud_index;
    int realBaud;
    short int verbose;
    bool esc;
    bool lastFatal;
    bool isEPOC;
    bool justStarted;

    char *devname;
    int baud;
};

#endif
