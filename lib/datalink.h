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

#include <condition_variable>
#include <stdio.h>
#include <pthread.h>
#include <mutex>

#include "bufferstore.h"
#include "bufferarray.h"

extern "C" {
    static void *data_pump_thread(void *);
}

class Link;

/**
* Thread-safe class responsible for managing the underlying serial device and data link framing.
*/
class DataLink
{
public:
    DataLink(const char *fname, int baud, Link *_link, bool noDSRCheck, unsigned short verbose, const int cancellationFd);
    ~DataLink();

    /**
    * Send a buffer out to serial line.
    *
    * This blocks until there's enough space in the output buffer to write the whole message atomically (to ensure
    * messages can't get interleaved), suspending the current thread until signaled by the data pump thread if
    * there's insufficient space.
    *
    * Drops messages on the floor when shutting down.
    *
    * @param b buffer to send
    * @param isEPOC flag indicating if additional EPOC32 byte-stuffing should be used
    */
    void send(BufferStore &b, bool isEPOC);

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
    bool processInputData(std::vector<BufferStore> &receivedData);

    void sendReceivedData(std::vector<BufferStore> &receivedData);

    /**
    * Store a flag that we're shutting down and signal any waiting @ref send calls.
    */
    void shutdown();

    void internalReset(bool resetBaudRateIndex);

    pthread_t dataPumpThreadId_;

    unsigned int crc_table[256];

    // The following sections represent groups of variables that comprise the state associated
    // with maintaining the underlying serial device, reading data from the serial device, and
    // writing data to the serial device. These should be treated as three distinct domains
    // wrt. concurrency and data within each group should be updated atomically.

    // When acquiring locks, the order _must_ be:
    // 1) serial
    // 2) input
    // 3) output

    // Serial.

    std::mutex serialMutex_;
    int fd;
    int serialStatus = -1;
    int baudRateIndex_ = 0;
    int baudRate_;
    bool lastFatal = false;

    // Reading from serial.

    std::mutex inputMutex_;
    bool esc = false;
    bool justStarted = true;
    BufferStore rcv;
    int startPkt = -1;
    int lastSYN = -1;
    unsigned short crcIn = 0;
    unsigned short inCRCstate;
    unsigned short receivedCRC;
    unsigned char *inBuffer; int inWrite = 0; int inRead = 0;

    // Writing to serial.

    std::mutex outputMutex_;
    bool isCancelled_ = false;
    unsigned char *outBuffer; int outWrite = 0; int outRead = 0;
    bool noDSRCheck_ = false;

    // Signaling.

    std::condition_variable outputCondition_;

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
